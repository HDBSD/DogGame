#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "title_guest_movie_frame.h"
#include <xbox/movie_frame.h>
#if defined(_WIN32)
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#endif

/* The recompiler's manual scanner recognizes extern sub_ADDRESS_gen as a
 * wrapped body to retain. A custom delegate name would make clean setup
 * generation omit the original routine while retaining only our observer. */
extern void sub_00152FEC_gen(void);
extern void sub_001000B0_gen(void);

/* SD100F: 0015413D locks the destination at 0015414F, then converts
 * decoder +40/+44 pixels into it. 001534FE reports status 1 only after
 * that copy returns. 00108D20 writes pitch and pixel pointer to LockRect's
 * output. These are translated-instruction observations, not a new decoder. */
static _Thread_local int observing_movie;
static _Thread_local uint32_t locked_pixels, locked_pitch;

void title_guest_movie_lock_surface(void)
{
    uint32_t output = MEM32(esp + 8);
    int movie_lock = observing_movie && MEM32(esp) == 0x0015415Fu;
    sub_001000B0_gen();
    if (movie_lock && output >= 0x10000u && output <= 0x03FFFFF8u)
    {
        locked_pitch = MEM32(output);
        locked_pixels = MEM32(output + 4);
    }
}

void title_guest_movie_step(void)
{
    uint32_t decoder = MEM32(esp + 4);
    uint32_t status = MEM32(esp + 12);
#if defined(_WIN32)
    uint64_t started = GetTickCount64();
    static _Thread_local uint64_t previous_frame, previous_call;
    static _Thread_local unsigned polls;
    uint64_t call_gap = previous_call ? started - previous_call : 0;
    previous_call = started;
    polls++;
#endif
    locked_pixels = 0;
    locked_pitch = 0;
    observing_movie = 1;
    sub_00152FEC_gen();
    observing_movie = 0;
#if defined(_WIN32)
    uint64_t elapsed = GetTickCount64() - started;
    if (elapsed > 50 && getenv("RECOMP_OVERLAY_STATS"))
    {
        fprintf(stderr, "[MOVIE-STEP] native-call=%llu ms\n", (unsigned long long)elapsed);
    }
#endif

    if (decoder < 0x10000u || decoder > 0x03FFFF00u ||
        status < 0x10000u || status > 0x03FFFFFCu || MEM32(status) != 1 ||
        !locked_pixels)
    {
        return;
    }

    uint32_t width = MEM32(decoder + 0x40);
    uint32_t height = MEM32(decoder + 0x44);
    uint32_t physical = locked_pixels & 0x0FFFFFFFu;
#if defined(_WIN32)
    if (previous_frame && started - previous_frame > 60 && getenv("RECOMP_OVERLAY_STATS"))
    {
        fprintf(stderr, "[MOVIE-GAP] frame-gap=%llu call-gap=%llu polls=%u timestamp=%u field=%u base=%u divisor=%u\n",
                (unsigned long long)(started - previous_frame), (unsigned long long)call_gap, polls,
                MEM32(decoder + 0xE4), MEM32(MEM32(0x10E648) + 0x1DD8),
                MEM32(decoder + 0xB8), MEM32(decoder + 0xBC));
    }
    previous_frame = started;
    polls = 0;
#endif
    if (width && height && width <= 2048 && height <= 2048 &&
        locked_pitch >= width * 2 && locked_pitch <= 8192 &&
        (uint64_t)physical + (uint64_t)locked_pitch * height <= 0x04000000u)
    {
        xbox_MovieFramePublish((const void *)XBOX_PTR(physical | 0x80000000u),
            physical, locked_pitch, width, height);
    }
}
