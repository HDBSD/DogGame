#ifndef XBOX_VIDEO_REFRESH_CLOCK_H
#define XBOX_VIDEO_REFRESH_CLOCK_H

#include <stdint.h>

/* Owned by the interrupt-dispatch thread. Counter units come from QPC, not
 * coarse uptime milliseconds. Preserve fractional periods and skip missed
 * deadlines after a stall; never replay a burst of guest interrupts. */
typedef struct
{
    uint64_t frequency;
    uint64_t deadline;
    uint64_t phase;
} XboxVideoRefreshClock;

static int xbox_video_refresh_due(XboxVideoRefreshClock *clock, uint64_t now, uint64_t frequency)
{
    uint64_t periods, fraction;
    if (frequency < 60)
    {
        return 0;
    }
    if (clock->frequency != frequency)
    {
        clock->frequency = frequency;
        clock->deadline = now;
        clock->phase = 0;
    }
    if (now < clock->deadline)
    {
        return 0;
    }

    periods = (now - clock->deadline) / frequency * 60 +
        ((now - clock->deadline) % frequency) * 60 / frequency + 1;
    fraction = clock->phase + periods * (frequency % 60);
    clock->deadline += periods * (frequency / 60) + fraction / 60;
    clock->phase = fraction % 60;
    if (clock->deadline <= now)
    {
        clock->phase += frequency % 60;
        clock->deadline += frequency / 60 + clock->phase / 60;
        clock->phase %= 60;
    }
    return 1;
}

#endif
