#ifndef TITLE_GUEST_MOVIE_FRAME_H
#define TITLE_GUEST_MOVIE_FRAME_H

/* Delegate the original routines once, retaining their guest ABI. Observation
 * and publication happen on the decoder thread and never mutate guest state. */
void title_guest_movie_step(void);
void title_guest_movie_lock_surface(void);
#endif
