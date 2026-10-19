#ifndef XBOX_MOVIE_FRAME_H
#define XBOX_MOVIE_FRAME_H
#include <stdint.h>

/* Decoder thread only, after the native image copy returns. Copies pixels;
 * ownership stays with the caller. Invalid dimensions are ignored. The window
 * consumes host snapshots, never the decoder's writable guest surface. */
void xbox_MovieFramePublish(const void *pixels, uint32_t physical,
                            uint32_t pitch, uint32_t width, uint32_t height);
#endif
