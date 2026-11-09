/*
 * The MCI sequencer, as far as the game uses it (midi.cpp): one device that
 * opens a .MID file, seeks, plays with MM_MCINOTIFY to a window when the tune
 * ends, reports its position and stops. The tune itself is played by the
 * platform's MIDI backend (midi_backend.h).
 */
#include "port_internal.h"
#include "midi_backend.h"

#define DEVICE_ID 1
#define MCI_MODE_STOP 525
#define MCI_MODE_PLAY 526

static struct {
    int open;
    void *tune;
    uint32_t pos;               /* where it stopped or was sought to */
    int playing;
    HWND notify;
} g_seq;

static int music_wanted(void)
{
    const char *e = getenv("LEMMINGS_MUSIC");

    return e ? atoi(e) != 0 : !port_harness;
}

static void notify_end(UINT how)
{
    if (g_seq.notify) {
        PostMessageA(g_seq.notify, MM_MCINOTIFY, how, DEVICE_ID);
        g_seq.notify = NULL;
    }
}

void mci_poll(void)
{
    if (g_seq.playing && !midi_backend_playing(g_seq.tune)) {
        g_seq.playing = 0;
        g_seq.pos = midi_backend_length(g_seq.tune);
        notify_end(MCI_NOTIFY_SUCCESSFUL);
    }
}

void mci_quit(void)
{
    if (g_seq.tune)
        midi_backend_close(g_seq.tune);
    memset(&g_seq, 0, sizeof g_seq);
}

UINT WINAPI midiOutGetNumDevs(void)
{
    return music_wanted() && midi_backend_available() ? 1 : 0;
}

MMRESULT WINAPI midiOutGetDevCapsA(UINT_PTR dev, LPMIDIOUTCAPS caps, UINT size)
{
    MIDIOUTCAPS c;

    (void)dev;
    if (!midiOutGetNumDevs())
        return MMSYSERR_BADDEVICEID;
    memset(&c, 0, sizeof c);
    c.vDriverVersion = 0x100;
    port_strlcpy(c.szPname, midi_backend_name(), sizeof c.szPname);
    c.wTechnology = MOD_SYNTH;
    c.wVoices = 64;
    c.wNotes = 64;
    c.wChannelMask = 0xffff;
    memcpy(caps, &c, size < sizeof c ? size : sizeof c);
    return MMSYSERR_NOERROR;
}

MCIERROR WINAPI mciSendCommandA(MCIDEVICEID dev, UINT msg, DWORD_PTR flags, DWORD_PTR parms)
{
    port_trace("mciSendCommand dev %u msg %#x flags %#lx", dev, msg, (unsigned long)flags);
    if (msg == MCI_SYSINFO) {
        MCI_SYSINFO_PARMS *p = (MCI_SYSINFO_PARMS *)parms;

        if ((flags & MCI_SYSINFO_QUANTITY) && p->lpstrReturn && p->dwRetSize >= 4) {
            *(DWORD *)p->lpstrReturn = p->wDeviceType == MCI_DEVTYPE_SEQUENCER ? midiOutGetNumDevs() : 0;
            return 0;
        }
        return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (msg == MCI_OPEN) {
        MCI_OPEN_PARMS *p = (MCI_OPEN_PARMS *)parms;
        char host[1100];

        if (!midiOutGetNumDevs())
            return MCIERR_CANNOT_LOAD_DRIVER;
        if (g_seq.open)
            return MCIERR_HARDWARE;
        if (!(flags & MCI_OPEN_ELEMENT) || !file_host_path(p->lpstrElementName, host, sizeof host, 0))
            return MCIERR_FILE_NOT_FOUND;
        g_seq.tune = midi_backend_open(host);
        if (!g_seq.tune) {
            port_trace("MIDI: can't play %s", host);
            return MCIERR_INVALID_FILE;
        }
        g_seq.open = 1;
        g_seq.pos = 0;
        g_seq.playing = 0;
        g_seq.notify = NULL;
        p->wDeviceID = DEVICE_ID;
        return 0;
    }
    if (dev != DEVICE_ID || !g_seq.open)
        return MCIERR_INVALID_DEVICE_ID;
    switch (msg) {
    case MCI_CLOSE:
        if (g_seq.playing)
            midi_backend_stop(g_seq.tune);
        notify_end(MCI_NOTIFY_ABORTED);
        midi_backend_close(g_seq.tune);
        memset(&g_seq, 0, sizeof g_seq);
        return 0;
    case MCI_SEEK:
        {
            MCI_SEEK_PARMS *p = (MCI_SEEK_PARMS *)parms;

            if (g_seq.playing) {
                midi_backend_stop(g_seq.tune);
                g_seq.playing = 0;
                notify_end(MCI_NOTIFY_ABORTED);
            }
            if (flags & MCI_SEEK_TO_START)
                g_seq.pos = 0;
            else if (flags & MCI_SEEK_TO_END)
                g_seq.pos = midi_backend_length(g_seq.tune);
            else if ((flags & MCI_TO) && p)
                g_seq.pos = p->dwTo;
        }
        return 0;
    case MCI_SET:
        return 0;               /* milliseconds is all the game uses */
    case MCI_STATUS:
        {
            MCI_STATUS_PARMS *p = (MCI_STATUS_PARMS *)parms;

            if (!(flags & MCI_STATUS_ITEM) || !p)
                return MCIERR_UNSUPPORTED_FUNCTION;
            switch (p->dwItem) {
            case MCI_STATUS_POSITION:
                p->dwReturn = g_seq.playing ? midi_backend_position(g_seq.tune) : g_seq.pos;
                return 0;
            case MCI_STATUS_LENGTH:
                p->dwReturn = midi_backend_length(g_seq.tune);
                return 0;
            case MCI_STATUS_MODE:
                p->dwReturn = g_seq.playing ? MCI_MODE_PLAY : MCI_MODE_STOP;
                return 0;
            }
            return MCIERR_UNSUPPORTED_FUNCTION;
        }
    case MCI_PLAY:
        {
            MCI_PLAY_PARMS *p = (MCI_PLAY_PARMS *)parms;

            if (flags & MCI_FROM)
                g_seq.pos = p->dwFrom;
            if (g_seq.playing) {
                midi_backend_stop(g_seq.tune);
                g_seq.playing = 0;
            }
            notify_end(MCI_NOTIFY_SUPERSEDED);
            if (midi_backend_play(g_seq.tune, g_seq.pos))
                return MCIERR_HARDWARE;
            g_seq.playing = 1;
            if ((flags & MCI_NOTIFY) && p)
                g_seq.notify = (HWND)p->dwCallback;
        }
        return 0;
    case MCI_STOP:
    case MCI_PAUSE:
        if (g_seq.playing) {
            g_seq.pos = midi_backend_position(g_seq.tune);
            midi_backend_stop(g_seq.tune);
            g_seq.playing = 0;
        }
        notify_end(MCI_NOTIFY_ABORTED);
        return 0;
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}

BOOL WINAPI mciGetErrorStringA(MCIERROR err, LPSTR buf, UINT size)
{
    const char *s;

    switch (err) {
    case MCIERR_INVALID_DEVICE_ID:
        s = "Invalid device ID. Use the ID given to the device when the device was opened.";
        break;
    case MCIERR_CANNOT_LOAD_DRIVER:
        s = "There is no MIDI synthesizer on this system.";
        break;
    case MCIERR_FILE_NOT_FOUND:
        s = "The specified file cannot be found. Make sure the path and filename are correct.";
        break;
    case MCIERR_INVALID_FILE:
        s = "The MIDI file cannot be played.";
        break;
    case MCIERR_HARDWARE:
        s = "The MIDI synthesizer is busy or not working.";
        break;
    default:
        s = "The MCI command failed.";
        break;
    }
    if (size)
        port_strlcpy(buf, s, size);
    return TRUE;
}
