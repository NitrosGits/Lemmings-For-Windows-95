/*
 * macOS: the sound streams of plat.h on Core Audio. One default-output
 * AudioUnit plays 32-bit float stereo; its render callback mixes every stream,
 * resampling each one linearly from its own rate. A stream keeps the bytes it
 * has not played, so plat_stream_queued() falls to exactly 0 when a sound has
 * ended (waveOut's WOM_DONE depends on that).
 */
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "plat.h"

#define OUT_RATE 44100.0

struct PlatStream {
    struct PlatStream *next;
    int rate, bits, channels, frameBytes;
    unsigned char *buf;
    size_t len, cap;        /* bytes in buf */
    size_t start;           /* the first frame not played, in bytes */
    double frac;            /* how far between that frame and the next */
    float gain;
};

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static AudioUnit g_unit;
static PlatStream *g_streams;

static float sample(const PlatStream *s, size_t frame, int ch)
{
    const unsigned char *p = s->buf + s->start + frame * (size_t)s->frameBytes;

    if (s->channels == 1)
        ch = 0;
    if (s->bits == 8)
        return ((int)p[ch] - 128) / 128.0f;
    return (int16_t)(p[ch * 2] | p[ch * 2 + 1] << 8) / 32768.0f;
}

static void mix(PlatStream *s, float *out, UInt32 frames)
{
    double step = s->rate / OUT_RATE;
    size_t have = (s->len - s->start) / (size_t)s->frameBytes;
    double pos = s->frac;
    UInt32 i;

    for (i = 0; i < frames; i++) {
        size_t f = (size_t)pos;
        double t = pos - (double)f;
        int ch;

        if (f >= have)
            break;
        for (ch = 0; ch < 2; ch++) {
            float a = sample(s, f, ch), b = f + 1 < have ? sample(s, f + 1, ch) : a;

            out[i * 2 + ch] += (a + (float)t * (b - a)) * s->gain;
        }
        pos += step;
    }
    {
        size_t used = (size_t)pos;

        if (used >= have) {         /* all played */
            s->start = s->len = 0;
            s->frac = 0;
        } else {
            s->start += used * (size_t)s->frameBytes;
            s->frac = pos - (double)used;
        }
    }
}

static OSStatus render(void *ref, AudioUnitRenderActionFlags *flags, const AudioTimeStamp *ts, UInt32 bus,
                       UInt32 frames, AudioBufferList *data)
{
    float *out = data->mBuffers[0].mData;
    PlatStream *s;

    (void)ref;
    (void)flags;
    (void)ts;
    (void)bus;
    memset(out, 0, data->mBuffers[0].mDataByteSize);
    pthread_mutex_lock(&g_lock);
    for (s = g_streams; s; s = s->next)
        if (s->len > s->start)
            mix(s, out, frames);
    pthread_mutex_unlock(&g_lock);
    for (UInt32 i = 0; i < frames * 2; i++)
        out[i] = out[i] > 1.0f ? 1.0f : out[i] < -1.0f ? -1.0f : out[i];
    return noErr;
}

int plat_audio_open(void)
{
    AudioComponentDescription d = {kAudioUnitType_Output, kAudioUnitSubType_DefaultOutput,
                                   kAudioUnitManufacturer_Apple, 0, 0};
    AudioComponent c = AudioComponentFindNext(NULL, &d);
    AudioStreamBasicDescription f;
    AURenderCallbackStruct cb = {render, NULL};

    if (!c || AudioComponentInstanceNew(c, &g_unit) != noErr)
        return 0;
    memset(&f, 0, sizeof f);
    f.mSampleRate = OUT_RATE;
    f.mFormatID = kAudioFormatLinearPCM;
    f.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    f.mChannelsPerFrame = 2;
    f.mBitsPerChannel = 32;
    f.mFramesPerPacket = 1;
    f.mBytesPerFrame = f.mBytesPerPacket = 8;
    if (AudioUnitSetProperty(g_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &f, sizeof f) ||
        AudioUnitSetProperty(g_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cb,
                             sizeof cb) ||
        AudioUnitInitialize(g_unit) || AudioOutputUnitStart(g_unit)) {
        AudioComponentInstanceDispose(g_unit);
        g_unit = NULL;
        return 0;
    }
    return 1;
}

void plat_audio_close(void)
{
    if (!g_unit)
        return;
    AudioOutputUnitStop(g_unit);
    AudioUnitUninitialize(g_unit);
    AudioComponentInstanceDispose(g_unit);
    g_unit = NULL;
}

PlatStream *plat_stream_create(int rate, int bits, int channels)
{
    PlatStream *s;

    if (rate <= 0 || (bits != 8 && bits != 16) || channels < 1 || channels > 2)
        return NULL;
    s = calloc(1, sizeof *s);
    s->rate = rate;
    s->bits = bits;
    s->channels = channels;
    s->frameBytes = bits / 8 * channels;
    s->gain = 1.0f;
    pthread_mutex_lock(&g_lock);
    s->next = g_streams;
    g_streams = s;
    pthread_mutex_unlock(&g_lock);
    return s;
}

int plat_stream_put(PlatStream *s, const void *data, int len)
{
    if (!s || len < 0)
        return 0;
    pthread_mutex_lock(&g_lock);
    if (s->start) {                 /* drop what has been played */
        memmove(s->buf, s->buf + s->start, s->len - s->start);
        s->len -= s->start;
        s->start = 0;
    }
    if (s->len + (size_t)len > s->cap) {
        size_t cap = (s->len + (size_t)len) * 2;
        unsigned char *b = realloc(s->buf, cap);

        if (!b) {
            pthread_mutex_unlock(&g_lock);
            return 0;
        }
        s->buf = b;
        s->cap = cap;
    }
    memcpy(s->buf + s->len, data, (size_t)len);
    s->len += (size_t)len;
    pthread_mutex_unlock(&g_lock);
    return 1;
}

int plat_stream_queued(PlatStream *s)
{
    int q;

    if (!s)
        return 0;
    pthread_mutex_lock(&g_lock);
    q = (int)(s->len - s->start);
    pthread_mutex_unlock(&g_lock);
    return q;
}

void plat_stream_clear(PlatStream *s)
{
    if (!s)
        return;
    pthread_mutex_lock(&g_lock);
    s->start = s->len = 0;
    s->frac = 0;
    pthread_mutex_unlock(&g_lock);
}

void plat_stream_gain(PlatStream *s, float gain)
{
    if (s)
        s->gain = gain;
}

void plat_stream_destroy(PlatStream *s)
{
    PlatStream **pp;

    if (!s)
        return;
    pthread_mutex_lock(&g_lock);
    for (pp = &g_streams; *pp; pp = &(*pp)->next)
        if (*pp == s) {
            *pp = s->next;
            break;
        }
    pthread_mutex_unlock(&g_lock);
    free(s->buf);
    free(s);
}
