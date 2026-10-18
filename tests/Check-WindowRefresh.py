"""Exercise the actual window refresh clock, including time spent drawing."""
from pathlib import Path
import subprocess

from test_support import msbuild

root = Path(__file__).resolve().parents[1]
output = root / "tests/irq-test-build"
source = (root / "runtime_overrides/src/video/fb_present.c").read_text()
clock = source[source.index("typedef struct WindowRefreshClock"):source.index("static DWORD WINAPI fb_thread(")]
fixture = "#include <windows.h>\n#include <stdio.h>\n" + clock + r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    WindowRefreshClock clock;
    LARGE_INTEGER started, now, work_started;
    window_refresh_initialize(&clock);
    QueryPerformanceCounter(&started);
    for (unsigned frame = 0; frame < 30; frame++)
    {
        /* Simulate conversion/drawing cost without relying on Sleep's
         * resolution. This work belongs inside the refresh interval. */
        QueryPerformanceCounter(&work_started);
        do
        {
            QueryPerformanceCounter(&now);
        } while (now.QuadPart - work_started.QuadPart < clock.frequency / 250);
        window_refresh_wait(&clock);
    }
    QueryPerformanceCounter(&now);
    double milliseconds = (now.QuadPart - started.QuadPart) * 1000.0 / clock.frequency;
    CHECK(milliseconds >= 480 && milliseconds < 700);
    Sleep(80);
    window_refresh_wait(&clock);
    QueryPerformanceCounter(&started);
    window_refresh_wait(&clock);
    QueryPerformanceCounter(&now);
    CHECK((now.QuadPart - started.QuadPart) * 1000.0 / clock.frequency >= 12);
    if (clock.timer) CloseHandle(clock.timer);
    clock.timer = NULL;
    window_refresh_wait(&clock);
    printf("Passed: drawing-inclusive 60 Hz refresh, stall recovery and timer fallback (30 refreshes %.2f ms).\n", milliseconds);
    return 0;
}
'''
(output / "window_refresh.c").write_text(fixture)
project = (output / "apu_irq_harness.vcxproj").read_text()
project = project.replace("apu_irq_harness.c", "window_refresh.c")
project = project.replace("obj\\apu_irq\\", "obj\\window_refresh\\")
(output / "window_refresh.vcxproj").write_text(project)
subprocess.run([msbuild, str(output / "window_refresh.vcxproj"),
                "/p:Configuration=Release", "/p:Platform=x64", "/verbosity:minimal"], check=True)
subprocess.run([str(output / "bin/window_refresh.exe")], check=True)
