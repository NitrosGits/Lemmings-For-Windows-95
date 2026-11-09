/* No MIDI synthesizer: the game sees no sequencer and plays no music. */
#include <stddef.h>

#include "midi_backend.h"

int midi_backend_available(void)
{
    return 0;
}

const char *midi_backend_name(void)
{
    return "none";
}

void *midi_backend_open(const char *hostPath)
{
    (void)hostPath;
    return NULL;
}

void midi_backend_close(void *m)
{
    (void)m;
}

int midi_backend_play(void *m, uint32_t fromMs)
{
    (void)m;
    (void)fromMs;
    return -1;
}

void midi_backend_stop(void *m)
{
    (void)m;
}

uint32_t midi_backend_position(void *m)
{
    (void)m;
    return 0;
}

uint32_t midi_backend_length(void *m)
{
    (void)m;
    return 0;
}

int midi_backend_playing(void *m)
{
    (void)m;
    return 0;
}

void midi_backend_level(double *rendered, double *heard, float *peak)
{
    *rendered = *heard = 0;
    *peak = 0;
}
