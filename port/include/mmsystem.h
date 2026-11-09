/*
 * The part of MMSYSTEM (WINMM) that the game uses: waveOut for the sound
 * effects, mmio for reading WAV files, MCI for the MIDI music. See windows.h.
 */
#ifndef PORT_MMSYSTEM_H
#define PORT_MMSYSTEM_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef UINT MMRESULT;
typedef DWORD FOURCC;
typedef DWORD MCIERROR;
typedef UINT MCIDEVICEID;

#define mmioFOURCC(a, b, c, d) \
    ((DWORD)(BYTE)(a) | ((DWORD)(BYTE)(b) << 8) | ((DWORD)(BYTE)(c) << 16) | ((DWORD)(BYTE)(d) << 24))

#define MMSYSERR_NOERROR     0
#define MMSYSERR_ERROR       1
#define MMSYSERR_BADDEVICEID 2
#define MMSYSERR_NOTENABLED  3
#define MMSYSERR_ALLOCATED   4
#define MMSYSERR_INVALHANDLE 5
#define MMSYSERR_NODRIVER    6
#define MMSYSERR_NOMEM       7
#define MMSYSERR_NOTSUPPORTED 8
#define MMSYSERR_INVALFLAG   10
#define MMSYSERR_INVALPARAM  11
#define WAVERR_BADFORMAT     32
#define WAVERR_STILLPLAYING  33
#define WAVERR_UNPREPARED    34

#define MM_MCINOTIFY 0x3B9
#define MM_WOM_OPEN  0x3BB
#define MM_WOM_CLOSE 0x3BC
#define MM_WOM_DONE  0x3BD

/* Wave output */
#define WAVE_MAPPER       ((UINT)-1)
#define WAVE_FORMAT_QUERY 0x0001
#define WAVE_FORMAT_PCM   1
#define CALLBACK_NULL     0x00000000L
#define CALLBACK_WINDOW   0x00010000L
#define WHDR_DONE      0x00000001
#define WHDR_PREPARED  0x00000002
#define WHDR_BEGINLOOP 0x00000004
#define WHDR_ENDLOOP   0x00000008
#define WHDR_INQUEUE   0x00000010
#define WAVECAPS_PITCH        0x0001
#define WAVECAPS_PLAYBACKRATE 0x0002
#define WAVECAPS_VOLUME       0x0004
#define WAVECAPS_LRVOLUME     0x0008
#define WAVECAPS_SYNC         0x0010
#define TIME_MS      0x0001
#define TIME_SAMPLES 0x0002
#define TIME_BYTES   0x0004

#pragma pack(push, 1)
typedef struct tWAVEFORMATEX {  /* 18 bytes, read from WAV files */
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX *LPCWAVEFORMATEX;
typedef struct pcmwaveformat_tag {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
} PCMWAVEFORMAT;
#pragma pack(pop)

typedef struct wavehdr_tag {
    LPSTR lpData;
    DWORD dwBufferLength;
    DWORD dwBytesRecorded;
    DWORD_PTR dwUser;
    DWORD dwFlags;
    DWORD dwLoops;
    struct wavehdr_tag *lpNext;
    DWORD_PTR reserved;
} WAVEHDR, *LPWAVEHDR;

typedef struct tagWAVEOUTCAPSA {
    WORD wMid;
    WORD wPid;
    UINT vDriverVersion;
    CHAR szPname[32];
    DWORD dwFormats;
    WORD wChannels;
    WORD wReserved1;
    DWORD dwSupport;
} WAVEOUTCAPSA, WAVEOUTCAPS, *LPWAVEOUTCAPS;

typedef struct mmtime_tag {
    UINT wType;
    union {
        DWORD ms;
        DWORD sample;
        DWORD cb;
        DWORD ticks;
        struct {
            BYTE hour, min, sec, frame, fps, dummy, pad[2];
        } smpte;
        struct {
            DWORD songptrpos;
        } midi;
    } u;
} MMTIME, *LPMMTIME;

UINT WINAPI waveOutGetNumDevs(void);
MMRESULT WINAPI waveOutGetDevCapsA(UINT_PTR dev, LPWAVEOUTCAPS caps, UINT size);
MMRESULT WINAPI waveOutOpen(HWAVEOUT *out, UINT dev, LPCWAVEFORMATEX format, DWORD_PTR callback,
                            DWORD_PTR instance, DWORD flags);
MMRESULT WINAPI waveOutClose(HWAVEOUT out);
MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT out, LPWAVEHDR hdr, UINT size);
MMRESULT WINAPI waveOutUnprepareHeader(HWAVEOUT out, LPWAVEHDR hdr, UINT size);
MMRESULT WINAPI waveOutWrite(HWAVEOUT out, LPWAVEHDR hdr, UINT size);
MMRESULT WINAPI waveOutReset(HWAVEOUT out);
MMRESULT WINAPI waveOutGetPosition(HWAVEOUT out, LPMMTIME time, UINT size);
MMRESULT WINAPI waveOutSetVolume(HWAVEOUT out, DWORD volume);
#define waveOutGetDevCaps waveOutGetDevCapsA

/* MIDI output capabilities (the About box) */
#define MOD_MIDIPORT 1
#define MOD_SYNTH    2
#define MOD_SQSYNTH  3
#define MOD_FMSYNTH  4
#define MOD_MAPPER   5
#define MIDI_MAPPER  ((UINT)-1)
typedef struct tagMIDIOUTCAPSA {
    WORD wMid;
    WORD wPid;
    UINT vDriverVersion;
    CHAR szPname[32];
    WORD wTechnology;
    WORD wVoices;
    WORD wNotes;
    WORD wChannelMask;
    DWORD dwSupport;
} MIDIOUTCAPSA, MIDIOUTCAPS, *LPMIDIOUTCAPS;
UINT WINAPI midiOutGetNumDevs(void);
MMRESULT WINAPI midiOutGetDevCapsA(UINT_PTR dev, LPMIDIOUTCAPS caps, UINT size);
#define midiOutGetDevCaps midiOutGetDevCapsA

/* RIFF files */
#define MMIO_READ      0x00000000
#define MMIO_WRITE     0x00000001
#define MMIO_READWRITE 0x00000002
#define MMIO_ALLOCBUF  0x00010000
#define MMIO_FINDCHUNK 0x0010
#define MMIO_FINDRIFF  0x0020
#define MMIO_FINDLIST  0x0040
#define MMIOERR_CHUNKNOTFOUND 265
typedef struct _MMCKINFO {
    FOURCC ckid;
    DWORD cksize;
    FOURCC fccType;
    DWORD dwDataOffset;
    DWORD dwFlags;
} MMCKINFO, *LPMMCKINFO;
typedef struct _MMIOINFO MMIOINFO, *LPMMIOINFO;
HMMIO WINAPI mmioOpenA(LPSTR name, LPMMIOINFO info, DWORD flags);
MMRESULT WINAPI mmioClose(HMMIO mmio, UINT flags);
LONG WINAPI mmioRead(HMMIO mmio, HPSTR buf, LONG size);
LONG WINAPI mmioSeek(HMMIO mmio, LONG offset, int origin);
MMRESULT WINAPI mmioDescend(HMMIO mmio, LPMMCKINFO ck, const MMCKINFO *parent, UINT flags);
MMRESULT WINAPI mmioAscend(HMMIO mmio, LPMMCKINFO ck, UINT flags);
#define mmioOpen mmioOpenA

/* Timer */
DWORD WINAPI timeGetTime(void);

/* MCI */
#define MCI_OPEN    0x0803
#define MCI_CLOSE   0x0804
#define MCI_PLAY    0x0806
#define MCI_SEEK    0x0807
#define MCI_STOP    0x0808
#define MCI_PAUSE   0x0809
#define MCI_SET     0x080D
#define MCI_SYSINFO 0x0810
#define MCI_STATUS  0x0814
#define MCI_NOTIFY  0x00000001L
#define MCI_WAIT    0x00000002L
#define MCI_FROM    0x00000004L
#define MCI_TO      0x00000008L
#define MCI_TRACK   0x00000010L
#define MCI_SEEK_TO_START 0x00000100L
#define MCI_SEEK_TO_END   0x00000200L
#define MCI_OPEN_SHAREABLE  0x00000100L
#define MCI_OPEN_ELEMENT    0x00000200L
#define MCI_OPEN_ALIAS      0x00000400L
#define MCI_OPEN_ELEMENT_ID 0x00000800L
#define MCI_OPEN_TYPE_ID    0x00001000L
#define MCI_OPEN_TYPE       0x00002000L
#define MCI_SET_TIME_FORMAT 0x00000400L
#define MCI_STATUS_ITEM     0x00000100L
#define MCI_STATUS_LENGTH   0x00000001L
#define MCI_STATUS_POSITION 0x00000002L
#define MCI_STATUS_MODE     0x00000004L
#define MCI_SYSINFO_QUANTITY 0x00000100L
#define MCI_SYSINFO_OPEN     0x00000200L
#define MCI_SYSINFO_NAME     0x00000400L
#define MCI_FORMAT_MILLISECONDS 0
#define MCI_DEVTYPE_SEQUENCER 523
#define MCI_NOTIFY_SUCCESSFUL 0x0001
#define MCI_NOTIFY_SUPERSEDED 0x0002
#define MCI_NOTIFY_ABORTED    0x0004
#define MCI_NOTIFY_FAILURE    0x0008
#define MCIERR_BASE                 256
#define MCIERR_INVALID_DEVICE_ID    (MCIERR_BASE + 1)
#define MCIERR_UNRECOGNIZED_COMMAND (MCIERR_BASE + 5)
#define MCIERR_HARDWARE             (MCIERR_BASE + 6)
#define MCIERR_CANNOT_LOAD_DRIVER   (MCIERR_BASE + 10)
#define MCIERR_UNSUPPORTED_FUNCTION (MCIERR_BASE + 18)
#define MCIERR_FILE_NOT_FOUND       (MCIERR_BASE + 19)
#define MCIERR_DEVICE_NOT_READY     (MCIERR_BASE + 20)
#define MCIERR_INVALID_FILE         (MCIERR_BASE + 40)

typedef struct tagMCI_GENERIC_PARMS {
    DWORD_PTR dwCallback;
} MCI_GENERIC_PARMS;
typedef struct tagMCI_OPEN_PARMSA {
    DWORD_PTR dwCallback;
    MCIDEVICEID wDeviceID;
    LPCSTR lpstrDeviceType;
    LPCSTR lpstrElementName;
    LPCSTR lpstrAlias;
} MCI_OPEN_PARMSA, MCI_OPEN_PARMS;
typedef struct tagMCI_PLAY_PARMS {
    DWORD_PTR dwCallback;
    DWORD dwFrom;
    DWORD dwTo;
} MCI_PLAY_PARMS;
typedef struct tagMCI_SEEK_PARMS {
    DWORD_PTR dwCallback;
    DWORD dwTo;
} MCI_SEEK_PARMS;
typedef struct tagMCI_SET_PARMS {
    DWORD_PTR dwCallback;
    DWORD dwTimeFormat;
    DWORD dwAudio;
} MCI_SET_PARMS;
typedef struct tagMCI_STATUS_PARMS {
    DWORD_PTR dwCallback;
    DWORD_PTR dwReturn;
    DWORD dwItem;
    DWORD dwTrack;
} MCI_STATUS_PARMS;
typedef struct tagMCI_SYSINFO_PARMSA {
    DWORD_PTR dwCallback;
    LPSTR lpstrReturn;
    DWORD dwRetSize;
    DWORD dwNumber;
    UINT wDeviceType;
} MCI_SYSINFO_PARMSA, MCI_SYSINFO_PARMS;

MCIERROR WINAPI mciSendCommandA(MCIDEVICEID dev, UINT msg, DWORD_PTR flags, DWORD_PTR parms);
BOOL WINAPI mciGetErrorStringA(MCIERROR err, LPSTR buf, UINT size);
#define mciSendCommand mciSendCommandA
#define mciGetErrorString mciGetErrorStringA

#ifdef __cplusplus
}
#endif

#endif
