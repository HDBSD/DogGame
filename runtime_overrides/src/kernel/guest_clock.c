#include "guest_clock.h"

#if defined(_WIN32)
#include <windows.h>

static INIT_ONCE clock_initialization = INIT_ONCE_STATIC_INIT;
static LARGE_INTEGER clock_frequency, clock_origin;
static uint64_t clock_uptime;

static BOOL CALLBACK initialize_guest_clock(PINIT_ONCE once, PVOID parameter, PVOID *context)
{
    (void)once;
    (void)parameter;
    (void)context;
    clock_uptime = GetTickCount64();
    if (!QueryPerformanceFrequency(&clock_frequency) || clock_frequency.QuadPart <= 0 ||
        !QueryPerformanceCounter(&clock_origin))
    {
        clock_frequency.QuadPart = 0;
    }
    return TRUE;
}

uint64_t xbox_GuestMilliseconds(void)
{
    LARGE_INTEGER now;
    InitOnceExecuteOnce(&clock_initialization, initialize_guest_clock, NULL, NULL);
    if (!clock_frequency.QuadPart)
    {
        return GetTickCount64();
    }
    QueryPerformanceCounter(&now);
    uint64_t elapsed = (uint64_t)(now.QuadPart - clock_origin.QuadPart);
    uint64_t frequency = (uint64_t)clock_frequency.QuadPart;
    /* Split before multiplication so the host's uptime does not overflow
     * the conversion. All users share this origin and rate. */
    return clock_uptime + (elapsed / frequency) * 1000 +
           ((elapsed % frequency) * 1000) / frequency;
}

#else
#include "../platform/win32_compat.h"

uint64_t xbox_GuestMilliseconds(void)
{
    return GetTickCount64();
}
#endif
