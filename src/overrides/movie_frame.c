#include "api/title_guest_movie_frame.h"

/* Keep the native decode and timing. Only hand completed video images to the
 * host window; audio and guest scheduling retain their original call order. */
void sub_00152FEC(void)
{
    title_guest_movie_step();
}

void sub_001000B0(void)
{
    title_guest_movie_lock_surface();
}
