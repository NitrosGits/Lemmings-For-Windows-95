/*
 * What the MCI sequencer (mci.c) needs from a platform's MIDI synthesizer:
 * play a Standard MIDI File from a position, stop, and say where it is. One
 * backend is linked in: port/macos/midi_coreaudio.c (Core Audio's built-in
 * General MIDI synthesizer) on macOS, port/src/midi_none.c (no music)
 * elsewhere until those platforms get theirs (PORTING.md, section 3).
 */
#ifndef MIDI_BACKEND_H
#define MIDI_BACKEND_H

#include <stdint.h>

int midi_backend_available(void);
const char *midi_backend_name(void);
unsigned midi_backend_version(void);    /* as MIDIOUTCAPS has it: 0x0100 is 1.0 */
void *midi_backend_open(const char *hostPath);      /* NULL if it can't be played */
void midi_backend_close(void *m);
int midi_backend_play(void *m, uint32_t fromMs);    /* 0 when it plays */
void midi_backend_stop(void *m);
uint32_t midi_backend_position(void *m);            /* milliseconds */
uint32_t midi_backend_length(void *m);
int midi_backend_playing(void *m);                  /* 0 once stopped or at the end */
/* the harness: seconds of synthesizer output so far, seconds with sound, the peak (0..1) */
void midi_backend_level(double *rendered, double *heard, float *peak);

#endif
