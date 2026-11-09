/*
 * Sound effects: waveOut on SDL audio streams, and the mmio RIFF reader the
 * game loads its WAV files with. Every waveOut handle is an SDL audio stream
 * in the wave's own format, bound to one shared playback device; SDL converts.
 * The position is the bytes written minus the bytes still queued.
 *
 * LEMMINGS_SOUND=0 (the default under the test harness, as tools/wine/run.sh
 * mutes Wine) reports no wave device.
 */
#include "port_internal.h"
#include "port_res.h"

static SDL_AudioDeviceID g_dev;
static int g_audioOk = -1;

typedef struct WaveOut {
    SDL_AudioStream *stream;
    WAVEFORMATEX fmt;
    DWORD written;
    WAVEHDR *pending[16];
    DWORD ends[16];
    int npending;
    HWND notify;
    DWORD_PTR instance;
} WaveOut;

static WAVEHDR *g_prepared[256];
static int g_nprepared;

static int known(WAVEHDR *h)
{
    int i;

    for (i = 0; i < g_nprepared; i++)
        if (g_prepared[i] == h)
            return 1;
    return 0;
}

void wave_init(void)
{
    const char *e = SDL_getenv("LEMMINGS_SOUND");

    if (e ? !SDL_atoi(e) : port_harness) {
        g_audioOk = 0;
        return;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        port_trace("audio: %s", SDL_GetError());
        g_audioOk = 0;
        return;
    }
    g_dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    g_audioOk = g_dev != 0;
    if (!g_audioOk)
        port_trace("audio device: %s", SDL_GetError());
}

void wave_quit(void)
{
    if (g_dev)
        SDL_CloseAudioDevice(g_dev);
    g_dev = 0;
}

int port_audio_available(void)
{
    return g_audioOk > 0;
}

SDL_AudioDeviceID port_audio_device(void)
{
    return g_dev;
}

UINT WINAPI waveOutGetNumDevs(void)
{
    return g_audioOk > 0 ? 1 : 0;
}

MMRESULT WINAPI waveOutGetDevCapsA(UINT_PTR dev, LPWAVEOUTCAPS caps, UINT size)
{
    WAVEOUTCAPS c;

    if (g_audioOk <= 0 || (dev != 0 && dev != WAVE_MAPPER))
        return MMSYSERR_BADDEVICEID;
    memset(&c, 0, sizeof c);
    c.wMid = 1;
    c.vDriverVersion = 0x100;
    SDL_strlcpy(c.szPname, "SDL audio output", sizeof c.szPname);
    c.dwFormats = 0xfff;
    c.wChannels = 2;
    c.dwSupport = WAVECAPS_VOLUME | WAVECAPS_LRVOLUME;
    memcpy(caps, &c, size < sizeof c ? size : sizeof c);
    return MMSYSERR_NOERROR;
}

static WaveOut *wo_get(HWAVEOUT h)
{
    return (WaveOut *)port_handle_get(h, H_WAVEOUT);
}

MMRESULT WINAPI waveOutOpen(HWAVEOUT *out, UINT dev, LPCWAVEFORMATEX f, DWORD_PTR callback, DWORD_PTR instance,
                            DWORD flags)
{
    SDL_AudioSpec spec;
    WaveOut *w;

    if (g_audioOk <= 0)
        return MMSYSERR_NODRIVER;
    if (dev != 0 && dev != WAVE_MAPPER)
        return MMSYSERR_BADDEVICEID;
    if (!f || f->wFormatTag != WAVE_FORMAT_PCM || (f->wBitsPerSample != 8 && f->wBitsPerSample != 16) ||
        f->nChannels < 1 || f->nChannels > 2 || f->nSamplesPerSec < 1000)
        return WAVERR_BADFORMAT;
    if (flags & WAVE_FORMAT_QUERY)
        return MMSYSERR_NOERROR;
    spec.format = f->wBitsPerSample == 8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE;
    spec.channels = f->nChannels;
    spec.freq = (int)f->nSamplesPerSec;
    w = calloc(1, sizeof *w);
    w->stream = SDL_CreateAudioStream(&spec, NULL);
    if (!w->stream || !SDL_BindAudioStream(g_dev, w->stream)) {
        port_trace("audio stream: %s", SDL_GetError());
        if (w->stream)
            SDL_DestroyAudioStream(w->stream);
        free(w);
        return MMSYSERR_ERROR;
    }
    memcpy(&w->fmt, f, sizeof(PCMWAVEFORMAT));
    if ((flags & 0x70000) == CALLBACK_WINDOW)
        w->notify = (HWND)callback;
    w->instance = instance;
    *out = (HWAVEOUT)port_handle_new(H_WAVEOUT, w);
    if (w->notify)
        PostMessageA(w->notify, MM_WOM_OPEN, (WPARAM)*out, 0);
    return MMSYSERR_NOERROR;
}

static DWORD played(WaveOut *w)
{
    int q = SDL_GetAudioStreamQueued(w->stream);

    return q < 0 || (DWORD)q > w->written ? 0 : w->written - (DWORD)q;
}

/* marks the buffers that have been played */
static void retire(HWAVEOUT h, WaveOut *w)
{
    DWORD pos = played(w);
    int i, n = 0;

    for (i = 0; i < w->npending; i++) {
        if (pos >= w->ends[i]) {
            w->pending[i]->dwFlags = (w->pending[i]->dwFlags & ~WHDR_INQUEUE) | WHDR_DONE;
            if (w->notify)
                PostMessageA(w->notify, MM_WOM_DONE, (WPARAM)h, (LPARAM)w->pending[i]);
        } else {
            w->pending[n] = w->pending[i];
            w->ends[n++] = w->ends[i];
        }
    }
    w->npending = n;
}

MMRESULT WINAPI waveOutClose(HWAVEOUT h)
{
    WaveOut *w = wo_get(h);

    if (!w)
        return MMSYSERR_INVALHANDLE;
    SDL_DestroyAudioStream(w->stream);
    if (w->notify)
        PostMessageA(w->notify, MM_WOM_CLOSE, (WPARAM)h, 0);
    port_handle_free(h);
    free(w);
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT h, LPWAVEHDR hdr, UINT size)
{
    (void)h;
    (void)size;
    if (!hdr || (uintptr_t)hdr < 0x10000)
        return MMSYSERR_INVALPARAM;
    if (!known(hdr) && g_nprepared < 256)
        g_prepared[g_nprepared++] = hdr;
    hdr->dwFlags |= WHDR_PREPARED;
    hdr->dwFlags &= ~WHDR_DONE;
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI waveOutUnprepareHeader(HWAVEOUT h, LPWAVEHDR hdr, UINT size)
{
    int i;

    (void)h;
    (void)size;
    /* the game passes back the error codes LoadWave returned as pointers */
    if (!known(hdr))
        return MMSYSERR_INVALPARAM;
    for (i = 0; i < g_nprepared; i++)
        if (g_prepared[i] == hdr)
            g_prepared[i] = g_prepared[--g_nprepared];
    hdr->dwFlags &= ~WHDR_PREPARED;
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI waveOutWrite(HWAVEOUT h, LPWAVEHDR hdr, UINT size)
{
    WaveOut *w = wo_get(h);

    (void)size;
    if (!w)
        return MMSYSERR_INVALHANDLE;
    if (!known(hdr) || !(hdr->dwFlags & WHDR_PREPARED))
        return WAVERR_UNPREPARED;
    retire(h, w);
    if (!SDL_PutAudioStreamData(w->stream, hdr->lpData, (int)hdr->dwBufferLength))
        return MMSYSERR_ERROR;
    w->written += hdr->dwBufferLength;
    hdr->dwFlags = (hdr->dwFlags & ~WHDR_DONE) | WHDR_INQUEUE;
    if (w->npending < 16) {
        w->pending[w->npending] = hdr;
        w->ends[w->npending++] = w->written;
    }
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI waveOutReset(HWAVEOUT h)
{
    WaveOut *w = wo_get(h);
    int i;

    if (!w)
        return MMSYSERR_INVALHANDLE;
    SDL_ClearAudioStream(w->stream);
    w->written = 0;
    for (i = 0; i < w->npending; i++)
        w->pending[i]->dwFlags = (w->pending[i]->dwFlags & ~WHDR_INQUEUE) | WHDR_DONE;
    w->npending = 0;
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI waveOutGetPosition(HWAVEOUT h, LPMMTIME t, UINT size)
{
    WaveOut *w = wo_get(h);
    DWORD pos;

    (void)size;
    if (!w)
        return MMSYSERR_INVALHANDLE;
    retire(h, w);
    pos = played(w);
    switch (t->wType) {
    case TIME_SAMPLES:
        t->u.sample = w->fmt.nBlockAlign ? pos / w->fmt.nBlockAlign : pos;
        break;
    case TIME_MS:
        t->u.ms = w->fmt.nAvgBytesPerSec ? (DWORD)((uint64_t)pos * 1000 / w->fmt.nAvgBytesPerSec) : 0;
        break;
    default:
        t->wType = TIME_BYTES;
        t->u.cb = pos;
        break;
    }
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI waveOutSetVolume(HWAVEOUT h, DWORD volume)
{
    WaveOut *w = wo_get(h);

    if (!w)
        return MMSYSERR_INVALHANDLE;
    SDL_SetAudioStreamGain(w->stream, (float)(volume & 0xffff) / 65535.0f);
    return MMSYSERR_NOERROR;
}

/* ---- mmio: reading RIFF files ---- */

typedef struct Mmio {
    FILE *f;
} Mmio;

static Mmio *mm_get(HMMIO h)
{
    return (Mmio *)port_handle_get(h, H_MMIO);
}

HMMIO WINAPI mmioOpenA(LPSTR name, LPMMIOINFO info, DWORD flags)
{
    char host[1100];
    Mmio *m;
    FILE *f;

    (void)info;
    if ((flags & 3) != MMIO_READ || !file_host_path(name, host, sizeof host, 0))
        return NULL;
    f = fopen(host, "rb");
    if (!f) {
        port_trace("mmioOpen(%s): not found (%s)", name, host);
        return NULL;
    }
    m = calloc(1, sizeof *m);
    m->f = f;
    return (HMMIO)port_handle_new(H_MMIO, m);
}

MMRESULT WINAPI mmioClose(HMMIO h, UINT flags)
{
    Mmio *m = mm_get(h);

    (void)flags;
    if (!m)
        return MMSYSERR_INVALHANDLE;
    fclose(m->f);
    port_handle_free(h);
    free(m);
    return 0;
}

LONG WINAPI mmioRead(HMMIO h, HPSTR buf, LONG size)
{
    Mmio *m = mm_get(h);

    if (!m || size < 0)
        return -1;
    return (LONG)fread(buf, 1, (size_t)size, m->f);
}

LONG WINAPI mmioSeek(HMMIO h, LONG offset, int origin)
{
    Mmio *m = mm_get(h);

    if (!m || fseek(m->f, offset, origin == 2 ? SEEK_END : origin == 1 ? SEEK_CUR : SEEK_SET))
        return -1;
    return (LONG)ftell(m->f);
}

static int read_u32(FILE *f, DWORD *v)
{
    BYTE b[4];

    if (fread(b, 1, 4, f) != 4)
        return 0;
    *v = (DWORD)b[0] | ((DWORD)b[1] << 8) | ((DWORD)b[2] << 16) | ((DWORD)b[3] << 24);
    return 1;
}

MMRESULT WINAPI mmioDescend(HMMIO h, LPMMCKINFO ck, const MMCKINFO *parent, UINT flags)
{
    Mmio *m = mm_get(h);
    long end = parent ? (long)(parent->dwDataOffset + parent->cksize) : 0x7fffffff;
    FOURCC want = flags & MMIO_FINDCHUNK ? ck->ckid : 0;
    FOURCC wantType = flags & (MMIO_FINDRIFF | MMIO_FINDLIST) ? ck->fccType : 0;

    if (!m)
        return MMSYSERR_INVALHANDLE;
    for (;;) {
        long at = ftell(m->f);
        DWORD id, size, type = 0;

        if (at + 8 > end || !read_u32(m->f, &id) || !read_u32(m->f, &size))
            return MMIOERR_CHUNKNOTFOUND;
        ck->ckid = id;
        ck->cksize = size;
        ck->dwFlags = 0;
        ck->dwDataOffset = (DWORD)(at + 8);
        if (id == mmioFOURCC('R', 'I', 'F', 'F') || id == mmioFOURCC('L', 'I', 'S', 'T')) {
            if (!read_u32(m->f, &type))
                return MMIOERR_CHUNKNOTFOUND;
            ck->fccType = type;
        }
        if ((flags & MMIO_FINDRIFF) && id == mmioFOURCC('R', 'I', 'F', 'F') && type == wantType)
            return 0;
        if ((flags & MMIO_FINDLIST) && id == mmioFOURCC('L', 'I', 'S', 'T') && type == wantType)
            return 0;
        if ((flags & MMIO_FINDCHUNK) && id == want) {
            fseek(m->f, at + 8, SEEK_SET);
            return 0;
        }
        if (!(flags & (MMIO_FINDCHUNK | MMIO_FINDRIFF | MMIO_FINDLIST))) {
            if (!type)
                fseek(m->f, at + 8, SEEK_SET);
            return 0;
        }
        if (fseek(m->f, at + 8 + (long)size + (long)(size & 1), SEEK_SET))
            return MMIOERR_CHUNKNOTFOUND;
    }
}

MMRESULT WINAPI mmioAscend(HMMIO h, LPMMCKINFO ck, UINT flags)
{
    Mmio *m = mm_get(h);

    (void)flags;
    if (!m)
        return MMSYSERR_INVALHANDLE;
    fseek(m->f, (long)(ck->dwDataOffset + ck->cksize + (ck->cksize & 1)), SEEK_SET);
    return 0;
}
