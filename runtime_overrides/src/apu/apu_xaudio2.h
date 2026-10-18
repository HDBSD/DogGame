/**
 * XAudio2 Audio Output Backend
 */
#ifndef APU_XAUDIO2_H
#define APU_XAUDIO2_H

#include <stdint.h>
#include <stdio.h>

/* Initialize XAudio2. Returns 1 on success, 0 on failure. */
int xa2_init(void);

/* Shut down XAudio2 and release all resources. */
void xa2_shutdown(void);

/* Returns 1 if XAudio2 is active. */
int xa2_is_active(void);

/* Submit interleaved stereo 16-bit samples. Returns 1 if accepted. */
int xa2_submit_samples(const int16_t *samples, int num_samples);

/* Get the preferred buffer size in samples. */
int xa2_get_buffer_size(void);

/* APU-thread diagnostic only, serialized with submission/shutdown. Writes a
 * backend snapshot without flushing, resetting or changing playback. Sample
 * counts and queue depth are observations of the asynchronous output engine.
 * The caller owns the FILE and handles write errors. NULL is ignored. */
void xa2_write_diagnostics(FILE *destination);

#endif /* APU_XAUDIO2_H */
