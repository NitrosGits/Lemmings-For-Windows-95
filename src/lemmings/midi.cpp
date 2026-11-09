/*
 * The music: a MIDI file per level played through the MCI sequencer,
 * restarted when it ends.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13;

#include <windows.h>
#include <stdlib.h>
#include "winlemm.h"

int g_musicOn = 1;
UINT g_midiDevice = (UINT)-1;
int g_midiPlaying = 0;
DWORD g_midiPos = 0;
HWND g_midiNotify = 0;
int g_tune = 0;
int g_tuneCount = 15;

/* 0x40ca50 */
void SetMusic(int on)
{
    g_musicOn = on;
    if (g_midiPlaying && !on) {
        PauseMidi();
        return;
    }
    if (!g_midiPlaying && on) {
        if (g_midiDevice != (UINT)-1) {
            RestartMidi();
            return;
        }
        if (g_hwndView) {
            OpenMidi(GetLevelMusic());
            PlayMidi(g_hwndView);
        }
    }
}

/* 0x40cab0: is there a MIDI sequencer */
int HaveMidi(void)
{
    DWORD count;
    MCI_SYSINFO_PARMS info;

    info.lpstrReturn = (LPSTR)&count;
    info.wDeviceType = MCI_DEVTYPE_SEQUENCER;
    info.dwRetSize = sizeof(count);
    mciSendCommand(0, MCI_SYSINFO, MCI_SYSINFO_QUANTITY, (PORT_DWORD)&info);
    return count != 0;
}

/* 0x40caf0 */
DWORD OpenMidi(char *file)
{
    MCI_SEEK_PARMS seek;
    MCI_SET_PARMS set;
    MCI_OPEN_PARMS open;
    char text[128];
    char path[1256];
    DWORD err;

    if (!g_musicOn)
        return 0;
    if (!HaveMidi())
        return 0;
    if (g_midiDevice == (UINT)-1) {
        wsprintf(path, "%s\\%s", g_curDir, file);
        open.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_SEQUENCER;
        open.lpstrElementName = path;
        if ((err = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_ELEMENT,
                                  (PORT_DWORD)&open)) != 0) {
            g_midiDevice = (UINT)-1;
            mciGetErrorString(err, text, sizeof(text));
            MessageBox(0, text, path, 0);
            return err;
        }
        g_midiDevice = open.wDeviceID;
    }
    seek.dwTo = 0;
    if ((err = mciSendCommand(g_midiDevice, MCI_SEEK, MCI_SEEK_TO_START, (PORT_DWORD)&seek)) != 0) {
        mciSendCommand(g_midiDevice, MCI_CLOSE, 0, 0);
        mciGetErrorString(err, path, 128);
        MessageBox(0, path, "Midi Error", 0);
        return err;
    }
    set.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
    if ((err = mciSendCommand(g_midiDevice, MCI_SET, MCI_SET_TIME_FORMAT | MCI_WAIT, (PORT_DWORD)&set)) != 0) {
        mciSendCommand(g_midiDevice, MCI_CLOSE, 0, 0);
        mciGetErrorString(err, path, 128);
        MessageBox(0, path, "Midi Error", 0);
        return err;
    }
    return err;
}

/* 0x40ccb0 */
DWORD CloseMidi(void)
{
    DWORD r;

    if (g_midiDevice == (UINT)-1)
        return (DWORD)-1;
    mciSendCommand(g_midiDevice, MCI_STOP, 0, 0);
    r = mciSendCommand(g_midiDevice, MCI_CLOSE, 0, 0);
    g_midiDevice = (UINT)-1;
    g_midiPlaying = 0;
    g_midiPos = 0;
    return r;
}

/* 0x40cd10 */
DWORD PauseMidi(void)
{
    MCI_STATUS_PARMS status;
    DWORD err;

    if (g_midiDevice != (UINT)-1) {
        status.dwItem = MCI_STATUS_POSITION;
        if ((err = mciSendCommand(g_midiDevice, MCI_STATUS, MCI_STATUS_ITEM, (PORT_DWORD)&status)) != 0) {
            char text[128];

            mciGetErrorString(err, text, sizeof(text));
            MessageBox(0, text, "Error MIDI", 0);
        } else {
            g_midiPos = status.dwReturn;
            if ((err = mciSendCommand(g_midiDevice, MCI_STOP, 0, 0)) != 0) {
                char text[128];

                mciGetErrorString(err, text, sizeof(text));
                MessageBox(0, text, "Error MIDI", 0);
            }
        }
        g_midiPlaying = 0;
        return err;
    }
    return (DWORD)-1;
}

/* 0x40cde0: play it again from the start */
DWORD RestartMidi(void)
{
    MCI_SEEK_PARMS seek;
    MCI_PLAY_PARMS play;
    char text[128];
    DWORD err;

    if (g_midiDevice == (UINT)-1 || !g_musicOn || !g_midiNotify)
        return (DWORD)-1;
    seek.dwTo = 0;
    if ((err = mciSendCommand(g_midiDevice, MCI_SEEK, MCI_TO | MCI_WAIT, (PORT_DWORD)&seek)) != 0) {
        mciSendCommand(g_midiDevice, MCI_CLOSE, 0, 0);
        mciGetErrorString(err, text, sizeof(text));
        MessageBox(0, text, "Midi Error", 0);
        return err;
    }
    play.dwCallback = (PORT_DWORD)g_midiNotify;
    if ((err = mciSendCommand(g_midiDevice, MCI_PLAY, MCI_NOTIFY, (PORT_DWORD)&play)) != 0) {
        mciGetErrorString(err, text, sizeof(text));
        MessageBox(0, text, "Midi Error", 0);
        CloseMidi();
        return err;
    }
    g_midiPlaying = 1;
    g_midiPos = 0;
    return 0;
}

/* 0x40cf00 */
DWORD PlayMidi(HWND hwnd)
{
    MCI_PLAY_PARMS play;
    char text[128];
    DWORD err;

    if (g_midiDevice == (UINT)-1 || !g_musicOn)
        return (DWORD)-1;
    play.dwCallback = (PORT_DWORD)hwnd;
    g_midiNotify = hwnd;
    if ((err = mciSendCommand(g_midiDevice, MCI_PLAY, MCI_NOTIFY, (PORT_DWORD)&play)) != 0) {
        CloseMidi();
        mciGetErrorString(err, text, sizeof(text));
        MessageBox(0, text, "Midi Error", 0);
        return err;
    }
    g_midiPlaying = 1;
    g_midiPos = 0;
    return 0;
}

/* 0x40cfb0 */
char *GetLevelMusic(void)
{
    static char *s_tunes[15] = {
        "music\\CANCANP.MID",
        "music\\LEMMIN1P.MID",
        "music\\LEMMIN2P.MID",
        "music\\LEMMIN3P.MID",
        "music\\MOUNTAIP.MID",
        "music\\TENLEMSP.MID",
        "music\\TIM1P.MID",
        "music\\TIM2P.MID",
        "music\\TIM3P.MID",
        "music\\TIM4P.MID",
        "music\\TIM5P.MID",
        "music\\TIM6P.MID",
        "music\\TIM7P.MID",
        "music\\TIM8P.MID",
        "music\\TIM9P.MID",
    };

    return s_tunes[g_tune];
}

/* 0x40cfc0 */
void NextTune(void)
{
    if (++g_tune >= g_tuneCount)
        g_tune = 0;
}

/* 0x40cfe0 */
void RandomTune(void)
{
    srand(timeGetTime());
    g_tune = rand() % g_tuneCount;
}
