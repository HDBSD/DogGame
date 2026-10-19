"""Check the actual guest refresh scheduler against fractional and stalled clocks."""
from pathlib import Path
import subprocess
from test_support import msbuild

root = Path(__file__).resolve().parents[1]
out = root / "tests/irq-test-build"
header = root / "runtime_overrides/src/kernel/video_refresh_clock.h"
fixture = f'#include "{header.as_posix()}"\n#include <stdio.h>\n' + r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    XboxVideoRefreshClock clock = {0};
    unsigned fields = 0, maximum_gap = 0, previous = 0;
    for (unsigned milliseconds = 0; milliseconds < 10000; milliseconds++)
    {
        if (xbox_video_refresh_due(&clock, (uint64_t)milliseconds * 10000, 10000000))
        {
            unsigned gap = milliseconds - previous;
            if (gap > maximum_gap) maximum_gap = gap;
            previous = milliseconds;
            fields++;
            CHECK(!xbox_video_refresh_due(&clock, (uint64_t)milliseconds * 10000, 10000000));
        }
    }
    CHECK(fields == 600 && maximum_gap <= 17);
    CHECK(clock.deadline == 100000000 && clock.phase == 0);
    // The previous scheduler sees uptime quantized to 15.625 ms. Its rate
    // averages near 60 Hz but field gaps repeatedly exceed a movie frame half.
    uint64_t next_ms = 0;
    previous = maximum_gap = fields = 0;
    for (unsigned milliseconds = 0; milliseconds < 10000; milliseconds++)
    {
        uint64_t coarse = ((uint64_t)milliseconds * 1000 / 15625) * 15625 / 1000;
        if (coarse >= next_ms)
        {
            unsigned gap = milliseconds - previous;
            if (gap > maximum_gap) maximum_gap = gap;
            previous = milliseconds;
            fields++;
            next_ms = (((coarse * 60 / 1000) + 1) * 1000 + 59) / 60;
        }
    }
    CHECK(fields >= 590 && fields <= 610 && maximum_gap >= 30);
    // A long scheduling stall emits one field, not a burst of old interrupts.
    CHECK(xbox_video_refresh_due(&clock, 105000000, 10000000));
    CHECK(clock.deadline > 105000000 && clock.deadline <= 105166667);
    CHECK(!xbox_video_refresh_due(&clock, 105000000, 10000000));
    CHECK(!xbox_video_refresh_due(&clock, 106000000, 0));
    // A failed QPC fallback can change clock domains without a stale deadline.
    CHECK(xbox_video_refresh_due(&clock, 42, 1000));
    CHECK(clock.deadline > 42 && clock.deadline <= 59);
    puts("Passed: precise 60-Hz fields, coarse-clock regression, stalls and fallback.");
    return 0;
}
'''
(out / "video_refresh_clock.c").write_text(fixture)
project = (out / "apu_irq_harness.vcxproj").read_text()
project = project.replace("apu_irq_harness.c", "video_refresh_clock.c").replace("obj\\apu_irq\\", "obj\\video_refresh_clock\\")
(out / "video_refresh_clock.vcxproj").write_text(project)
subprocess.run([msbuild, str(out / "video_refresh_clock.vcxproj"), "/p:Configuration=Release", "/p:Platform=x64", "/verbosity:minimal"], check=True)
subprocess.run([str(out / "bin/video_refresh_clock.exe")], check=True)
