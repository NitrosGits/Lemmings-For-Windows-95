/* The sound effects' waves, played one at a time through the wave mapper. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17;

#include <windows.h>
#include <mmsystem.h>
#include "lemming.h"

struct WAVE {
    char *name;
    WAVEFORMATEX *format;
    WAVEHDR *header;            /* followed by the samples */
};

int g_waveEnabled = 1;
int g_wavePlaying = 0;
HWAVEOUT g_waveDevice = 0;
WAVE g_waves[23] = {
    {"chink.wav"}, {"letsgo.wav"}, {"yippee.wav"}, {"splat.wav"}, {"ohno.wav"}, {"glug.wav"},
    {"scrape.wav"}, {"bang.wav"}, {"chain.wav"}, {"changeop.wav"}, {"die.wav"}, {"door.wav"},
    {"electric.wav"}, {"explode.wav"}, {"fire.wav"}, {"mantrap.wav"}, {"mousepre.wav"},
    {"oing.wav"}, {"slicer.wav"}, {"tenton.wav"}, {"thud.wav"}, {"thunk.wav"}, {"ting.wav"},
};
int g_waveToggle = 0;
WAVEHDR *g_waveCurrent = NULL;  /* the one playing */

int g_waveReady;
HWAVEOUT g_waveOut;
HWND g_waveWindow;
int g_volumeDevice;

/* 0x10005fd0: loads sound\name, prepared for playing; returns -1 to -7 on errors */
static WAVEHDR *LoadWave(char *name, WAVEFORMATEX **format)
{
    DWORD size;
    HWAVEOUT hwo;
    BYTE *data;
    MMCKINFO child;
    MMCKINFO parent;
    char path[128];
    HMMIO mmio;
    WAVEHDR *header;

    lstrcpy(path, "sound\\");
    lstrcat(path, name);
    mmio = mmioOpen(path, NULL, MMIO_ALLOCBUF | MMIO_READ);
    if (!mmio)
        return (WAVEHDR *)-1;
    parent.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    if (mmioDescend(mmio, &parent, NULL, MMIO_FINDRIFF)) {
        mmioClose(mmio, 0);
        return (WAVEHDR *)-2;
    }
    child.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(mmio, &child, &parent, MMIO_FINDCHUNK)) {
        mmioClose(mmio, 0);
        return (WAVEHDR *)-3;
    }
    size = child.cksize;
    *format = (WAVEFORMATEX *)VSMEM_Allocate(size, "wave format chunk");
    if (mmioRead(mmio, (HPSTR)*format, size) != (LONG)size) {
        mmioClose(mmio, 0);
        return (WAVEHDR *)-4;
    }
    mmioAscend(mmio, &child, 0);
    child.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(mmio, &child, &parent, MMIO_FINDCHUNK)) {
        mmioClose(mmio, 0);
        return (WAVEHDR *)-5;
    }
    size = child.cksize;
    header = (WAVEHDR *)VSMEM_Allocate(size + sizeof(WAVEHDR), "wave data chunk");
    data = (BYTE *)(header + 1);
    if (mmioRead(mmio, (HPSTR)data, size) != (LONG)size) {
        mmioClose(mmio, 0);
        return (WAVEHDR *)-6;
    }
    mmioClose(mmio, 0);
    header->lpData = (LPSTR)data;
    header->dwBufferLength = size;
    header->dwFlags = 0;
    header->dwLoops = 0;
    if ((*format)->wFormatTag != WAVE_FORMAT_PCM)
        return (WAVEHDR *)-7;
    if (waveOutOpen(&hwo, WAVE_MAPPER, *format, 0, 0, WAVE_FORMAT_QUERY))
        return (WAVEHDR *)-7;
    if (waveOutOpen(&hwo, WAVE_MAPPER, *format, 0, 0, 0))
        return (WAVEHDR *)-7;
    waveOutPrepareHeader(hwo, header, sizeof(WAVEHDR));
    waveOutClose(hwo);
    return header;
}

/* 0x10006200 (not used) */
void WaveOpenDevice(WAVEFORMATEX *format)
{
    if (!waveOutOpen(&g_waveDevice, g_volumeDevice, format, 0, 0, 0))
        g_wavePlaying = 0;
}

/* 0x10006230 (not used) */
void WaveCloseDevice(void)
{
    waveOutClose(g_waveDevice);
}

/* 0x10006240 (not used) */
LRESULT CALLBACK WaveWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case MM_WOM_CLOSE:
        g_waveToggle ^= 1;
        SetWindowText(hwnd, "MM_WOM_CLOSE received");
        return 0;
    case MM_WOM_OPEN:
        g_waveToggle ^= 1;
        SetWindowText(hwnd, "MM_WOM_OPEN received");
        return 0;
    case MM_WOM_DONE:
        SetWindowText(hwnd, "MM_WOM_DONE received");
        waveOutClose(g_waveOut);
        g_wavePlaying = 0;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* 0x100062f0: plays a wave unless the last one is still playing */
static void PlayWave(WAVEFORMATEX *format, WAVEHDR *header)
{
    MMTIME time;

    if (g_waveCurrent) {
        time.wType = TIME_BYTES;
        waveOutGetPosition(g_waveOut, &time, sizeof(time));
        if (g_waveCurrent->dwBufferLength > time.u.cb)
            return;
        waveOutReset(g_waveOut);
        waveOutClose(g_waveOut);
        g_waveCurrent = NULL;
    }
    if (!waveOutOpen(&g_waveOut, WAVE_MAPPER, format, 0, 0, 0)) {
        waveOutWrite(g_waveOut, header, sizeof(WAVEHDR));
        g_wavePlaying = 1;
        g_waveCurrent = header;
    }
}

/* 0x10006390 */
static void FreeWave(WAVEFORMATEX *format, WAVEHDR *header)
{
    HWAVEOUT hwo;

    if (!waveOutOpen(&hwo, WAVE_MAPPER, format, 0, 0, 0)) {
        waveOutUnprepareHeader(hwo, header, sizeof(WAVEHDR));
        waveOutClose(hwo);
        VSMEM_Free(format);
        VSMEM_Free(header);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_findvolumedevice0, pad_findvolumedevice1;

/* 0x100063e0: the first output device with volume control, or -1 */
static int FindVolumeDevice(void)
{
    WAVEOUTCAPS caps;
    int n, i;

    n = waveOutGetNumDevs();
    for (i = 0; i < n; i++)
        if (!waveOutGetDevCaps(i, &caps, sizeof(caps)) && (caps.dwSupport & WAVECAPS_VOLUME))
            return i;
    return -1;
}

/* 0x10006430 */
void WaveInit(void)
{
    WAVE *w;

    g_wavePlaying = 0;
    g_waveReady = 0;
    if (!waveOutGetNumDevs()) {
        g_waveReady = 0;
        return;
    }
    for (w = g_waves; w < g_waves + 23; w++)
        w->header = LoadWave(w->name, &w->format);
    g_waveReady = 1;
    g_volumeDevice = FindVolumeDevice();
}

/* 0x10006490 */
void WaveQuit(void)
{
    WAVE *w;

    if (g_waveReady) {
        for (w = g_waves; w < g_waves + 23; w++)
            FreeWave(w->format, w->header);
        if (g_wavePlaying)
            waveOutReset(g_waveOut);
        g_wavePlaying = 0;
        DestroyWindow(g_waveWindow);
    }
}

/* 0x100064f0 */
void WaveFX(PNT at, int sound)
{
    if (g_waveEnabled && g_waveReady)
        PlayWave(g_waves[sound].format, g_waves[sound].header);
}

/* 0x10006530 */
int WINAPI WaveIsEnabled(void)
{
    return waveOutGetNumDevs() != 0;
}

/* 0x10006540 */
int WINAPI WaveIsVolumeEnabled(void)
{
    return FindVolumeDevice() != -1;
}

/* 0x10006550 */
void WINAPI WaveSetVolume(int volume)
{
}

/* 0x10006560: plays a sample */
void WINAPI WaveTestVolume(int volume)
{
    WAVEFORMATEX *format;
    WAVEHDR *header;
    int devs, n;

    if (WaveIsVolumeEnabled()) {
        devs = waveOutGetNumDevs();
        /* The original clamps the device count to 100 and keeps it (in esi) until the end, so
           it used it in something that generates no code; its volume code was stripped. These
           two empty switches stand in for those uses: the first makes the clamp an expression
           (computed in eax, stored once), the second keeps n live across the calls. */
        switch (n = devs >= 100 ? 100 : devs) {
        case 1:
            break;
        }
        header = LoadWave("chink.wav", &format);
        PlayWave(format, header);
        FreeWave(format, header);
        switch (n) {
        case 1:
            break;
        }
    }
}
