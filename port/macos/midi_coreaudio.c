/*
 * macOS: the game's MIDI music through Core Audio. A MusicSequence loads the
 * .MID file, and its MusicPlayer plays it on the AUGraph that AudioToolbox
 * builds for it (Apple's built-in General MIDI synthesizer into the default
 * output). See port/src/midi_backend.h.
 */
#include <AudioToolbox/AudioToolbox.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "midi_backend.h"

typedef struct Tune {
    MusicSequence seq;
    MusicPlayer player;
    MusicTimeStamp lengthBeats;
    int playing;
} Tune;

/* what the synthesizer sent to the output (the harness's "levels"), from a
   render notification on the graph's output unit: render thread only */
static volatile double g_rendered, g_heard;
static volatile float g_peak;

static OSStatus metered(void *ref, AudioUnitRenderActionFlags *flags, const AudioTimeStamp *ts, UInt32 bus,
                        UInt32 frames, AudioBufferList *data)
{
    double rate = *(double *)ref;
    UInt32 i, b;

    (void)ts;
    (void)bus;
    if (!(*flags & kAudioUnitRenderAction_PostRender) || !data)
        return noErr;
    for (i = 0; i < frames; i++) {
        float m = 0;

        for (b = 0; b < data->mNumberBuffers; b++) {
            const float *x = data->mBuffers[b].mData;
            UInt32 ch = data->mBuffers[b].mNumberChannels, n = data->mBuffers[b].mDataByteSize / sizeof(float);
            UInt32 c;

            for (c = 0; c < ch && i * ch + c < n; c++)
                if (fabsf(x[i * ch + c]) > m)
                    m = fabsf(x[i * ch + c]);
        }
        if (m > 0.001f)
            g_heard += 1.0 / rate;
        if (m > g_peak)
            g_peak = m;
    }
    g_rendered += frames / rate;
    return noErr;
}

void midi_backend_level(double *rendered, double *heard, float *peak)
{
    *rendered = g_rendered;
    *heard = g_heard;
    *peak = g_peak;
}

/* meters the graph's output unit, if it renders 32-bit floats */
static void meter(Tune *t)
{
    static double rate;
    AUGraph graph;
    UInt32 n = 0, i;

    if (MusicSequenceGetAUGraph(t->seq, &graph) != noErr || AUGraphGetNodeCount(graph, &n) != noErr)
        return;
    for (i = 0; i < n; i++) {
        AUNode node;
        AudioComponentDescription d;
        AudioUnit unit;
        AudioStreamBasicDescription f;
        UInt32 size = sizeof f;

        if (AUGraphGetIndNode(graph, i, &node) != noErr || AUGraphNodeInfo(graph, node, &d, &unit) != noErr ||
            d.componentType != kAudioUnitType_Output)
            continue;
        if (AudioUnitGetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &f, &size) !=
                noErr ||
            f.mFormatID != kAudioFormatLinearPCM || !(f.mFormatFlags & kAudioFormatFlagIsFloat) ||
            f.mBitsPerChannel != 32 || f.mSampleRate <= 0)
            continue;
        rate = f.mSampleRate;
        AudioUnitAddRenderNotify(unit, metered, &rate);
    }
}

int midi_backend_available(void)
{
    return 1;
}

/* the synthesizer the music plays on, as the system names it */
static AudioComponent synth(void)
{
    AudioComponentDescription d = {kAudioUnitType_MusicDevice, kAudioUnitSubType_DLSSynth,
                                   kAudioUnitManufacturer_Apple, 0, 0};

    return AudioComponentFindNext(NULL, &d);
}

const char *midi_backend_name(void)
{
    static char name[64];
    AudioComponent c = synth();
    CFStringRef s = NULL;

    if (!name[0]) {
        snprintf(name, sizeof name, "Apple DLS Synth");
        if (c && AudioComponentCopyName(c, &s) == noErr && s) {
            char full[64], *p;

            /* "Apple: DLSMusicDevice" */
            if (CFStringGetCString(s, full, sizeof full, kCFStringEncodingASCII)) {
                while ((p = strchr(full, ':')))
                    memmove(p, p + 1, strlen(p));
                snprintf(name, sizeof name, "%s", full);
            }
            CFRelease(s);
        }
    }
    return name;
}

/* its version as a driver's: major in the high byte, minor in the low */
unsigned midi_backend_version(void)
{
    AudioComponent c = synth();
    UInt32 v = 0;

    if (!c || AudioComponentGetVersion(c, &v) != noErr || !v)
        return 0x100;
    return (v >> 16 & 0xff) << 8 | (v >> 8 & 0xff);
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
    meter(t);
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
