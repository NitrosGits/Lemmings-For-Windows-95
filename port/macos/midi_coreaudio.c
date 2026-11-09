/*
 * macOS: the game's MIDI music through Core Audio. A MusicSequence loads the
 * .MID file, and its MusicPlayer plays it on the AUGraph that AudioToolbox
 * builds for it (Apple's built-in General MIDI synthesizer into the default
 * output). See port/src/midi_backend.h.
 */
#include <AudioToolbox/AudioToolbox.h>
#include <stdlib.h>
#include <string.h>

#include "midi_backend.h"

typedef struct Tune {
    MusicSequence seq;
    MusicPlayer player;
    MusicTimeStamp lengthBeats;
    int playing;
} Tune;

int midi_backend_available(void)
{
    return 1;
}

const char *midi_backend_name(void)
{
    return "Apple DLS Synth";
}

void *midi_backend_open(const char *path)
{
    Tune *t = calloc(1, sizeof *t);
    CFURLRef url;
    UInt32 n = 0, i;

    if (NewMusicSequence(&t->seq) != noErr) {
        free(t);
        return NULL;
    }
    url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)path, (CFIndex)strlen(path), false);
    if (!url || MusicSequenceFileLoad(t->seq, url, kMusicSequenceFile_MIDIType, 0) != noErr) {
        if (url)
            CFRelease(url);
        DisposeMusicSequence(t->seq);
        free(t);
        return NULL;
    }
    CFRelease(url);
    MusicSequenceGetTrackCount(t->seq, &n);
    for (i = 0; i < n; i++) {
        MusicTrack track;
        MusicTimeStamp len = 0;
        UInt32 size = sizeof len;

        if (MusicSequenceGetIndTrack(t->seq, i, &track) == noErr &&
            MusicTrackGetProperty(track, kSequenceTrackProperty_TrackLength, &len, &size) == noErr &&
            len > t->lengthBeats)
            t->lengthBeats = len;
    }
    if (NewMusicPlayer(&t->player) != noErr || MusicPlayerSetSequence(t->player, t->seq) != noErr) {
        DisposeMusicSequence(t->seq);
        free(t);
        return NULL;
    }
    MusicPlayerPreroll(t->player);
    return t;
}

/* silences the synthesizer: a stopped MusicPlayer leaves notes sounding */
static void all_notes_off(Tune *t)
{
    AUGraph graph;
    UInt32 n = 0, i;

    if (MusicSequenceGetAUGraph(t->seq, &graph) != noErr || AUGraphGetNodeCount(graph, &n) != noErr)
        return;
    for (i = 0; i < n; i++) {
        AUNode node;
        AudioComponentDescription d;
        AudioUnit unit;
        int ch;

        if (AUGraphGetIndNode(graph, i, &node) != noErr || AUGraphNodeInfo(graph, node, &d, &unit) != noErr)
            continue;
        if (d.componentType != kAudioUnitType_MusicDevice)
            continue;
        for (ch = 0; ch < 16; ch++) {
            MusicDeviceMIDIEvent(unit, 0xB0 | ch, 123, 0, 0);   /* all notes off */
            MusicDeviceMIDIEvent(unit, 0xB0 | ch, 120, 0, 0);   /* all sound off */
        }
    }
}

void midi_backend_close(void *m)
{
    Tune *t = m;

    if (!t)
        return;
    MusicPlayerStop(t->player);
    all_notes_off(t);
    DisposeMusicPlayer(t->player);
    DisposeMusicSequence(t->seq);
    free(t);
}

int midi_backend_play(void *m, uint32_t fromMs)
{
    Tune *t = m;
    MusicTimeStamp beats = 0;

    if (!t)
        return -1;
    MusicSequenceGetBeatsForSeconds(t->seq, fromMs / 1000.0, &beats);
    MusicPlayerSetTime(t->player, beats);
    if (MusicPlayerStart(t->player) != noErr)
        return -1;
    t->playing = 1;
    return 0;
}

void midi_backend_stop(void *m)
{
    Tune *t = m;

    if (!t)
        return;
    MusicPlayerStop(t->player);
    all_notes_off(t);
    t->playing = 0;
}

uint32_t midi_backend_position(void *m)
{
    Tune *t = m;
    MusicTimeStamp beats = 0;
    Float64 secs = 0;

    if (!t || MusicPlayerGetTime(t->player, &beats) != noErr)
        return 0;
    MusicSequenceGetSecondsForBeats(t->seq, beats, &secs);
    return (uint32_t)(secs * 1000.0);
}

uint32_t midi_backend_length(void *m)
{
    Tune *t = m;
    Float64 secs = 0;

    if (!t)
        return 0;
    MusicSequenceGetSecondsForBeats(t->seq, t->lengthBeats, &secs);
    return (uint32_t)(secs * 1000.0);
}

int midi_backend_playing(void *m)
{
    Tune *t = m;
    MusicTimeStamp beats = 0;

    if (!t || !t->playing)
        return 0;
    if (MusicPlayerGetTime(t->player, &beats) == noErr && beats >= t->lengthBeats) {
        midi_backend_stop(t);
        return 0;
    }
    return 1;
}
