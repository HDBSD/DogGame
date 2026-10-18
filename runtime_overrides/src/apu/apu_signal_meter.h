/* Observation-only meters owned by the APU thread. They never alter samples.
 * Values outside [-1, 1] identify lost headroom, not necessarily bad source
 * data. Report windows run from initialization/the previous report, so the
 * first window can be longer than the WAV's retained history. */
#ifndef APU_SIGNAL_METER_H
#define APU_SIGNAL_METER_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ApuSignalMeter
{
    uint64_t samples;
    uint64_t over_range;
    uint64_t non_finite;
    float peak;
} ApuSignalMeter;

static inline int apu_signal_meter_enabled(void)
{
    static int enabled = -1;
    if (enabled < 0)
    {
        const char *value = getenv("RECOMP_DEBUG");
        enabled = value && *value && atoi(value) != 0;
    }
    return enabled;
}

static inline void apu_signal_meter_sample(ApuSignalMeter *meter, float value)
{
    meter->samples++;
    if (!isfinite(value))
    {
        meter->non_finite++;
        return;
    }
    float magnitude = fabsf(value);
    if (magnitude > meter->peak)
    {
        meter->peak = magnitude;
    }
    if (magnitude > 1.0f)
    {
        meter->over_range++;
    }
}

static inline void apu_signal_meter_write(FILE *file, const char *stage,
                                         ApuSignalMeter *meter)
{
    fprintf(file, " %s_samples=%llu %s_peak=%g %s_over=%llu %s_nonfinite=%llu",
            stage, (unsigned long long)meter->samples, stage, (double)meter->peak,
            stage, (unsigned long long)meter->over_range,
            stage, (unsigned long long)meter->non_finite);
    memset(meter, 0, sizeof(*meter));
}

#endif
