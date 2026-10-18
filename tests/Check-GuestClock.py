"""Check the production guest clock with deterministic host clock inputs."""
from pathlib import Path
import subprocess

from test_support import msbuild

root = Path(__file__).resolve().parents[1]
output = root / "tests/irq-test-build"
source = root / "runtime_overrides/src/kernel/guest_clock.c"
fixture = r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static LONGLONG counter = 1000000;
static ULONGLONG uptime = 0xFFFFFFF0ull;
static int unavailable;
static BOOL WINAPI host_frequency(LARGE_INTEGER *value)
{
    value->QuadPart = 10000000;
    return !unavailable;
}
static BOOL WINAPI host_counter(LARGE_INTEGER *value)
{
    value->QuadPart = counter;
    return TRUE;
}
static ULONGLONG WINAPI host_uptime(void)
{
    return uptime;
}
#define QueryPerformanceFrequency host_frequency
#define QueryPerformanceCounter host_counter
#define GetTickCount64 host_uptime
''' + f'#include "{source.as_posix()}"\n' + r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static DWORD WINAPI initialize_reader(void *unused)
{
    (void)unused;
    return xbox_GuestMilliseconds() == uptime ? 0 : 1;
}

int main(int argc, char **argv)
{
    unavailable = argc > 1 && !strcmp(argv[1], "fallback");
    if (unavailable)
    {
        CHECK(xbox_GuestMilliseconds() == uptime);
        uptime += 25;
        CHECK(xbox_GuestMilliseconds() == uptime);
        puts("Passed: unavailable-QPC fallback.");
        return 0;
    }
    HANDLE readers[8];
    for (unsigned i = 0; i < 8; i++)
    {
        readers[i] = CreateThread(NULL, 0, initialize_reader, NULL, 0, NULL);
        CHECK(readers[i]);
    }
    CHECK(WaitForMultipleObjects(8, readers, TRUE, 1000) == WAIT_OBJECT_0);
    for (unsigned i = 0; i < 8; i++)
    {
        DWORD result;
        CHECK(GetExitCodeThread(readers[i], &result) && !result);
        CloseHandle(readers[i]);
    }
    /* The coarse host uptime stays frozen across these reads. The guest
     * must nevertheless advance each elapsed millisecond, including wrap. */
    for (unsigned millisecond = 1; millisecond <= 20; millisecond++)
    {
        counter += 10000;
        uint64_t now = xbox_GuestMilliseconds();
        CHECK(now == uptime + millisecond);
        CHECK((uint32_t)now == (uint32_t)(uptime + millisecond));
    }
    counter = 1000000 + 10000000ll * 4294967300ll + 30000;
    CHECK(xbox_GuestMilliseconds() == uptime + 4294967300003ull);
    puts("Passed: concurrent initialization, epoch, millisecond advances, 32-bit wrap and long-run conversion.");
    return 0;
}
'''
(output / "guest_clock.c").write_text(fixture)
project = (output / "apu_irq_harness.vcxproj").read_text()
project = project.replace("apu_irq_harness.c", "guest_clock.c")
project = project.replace("obj\\apu_irq\\", "obj\\guest_clock\\")
(output / "guest_clock.vcxproj").write_text(project)
subprocess.run([msbuild, str(output / "guest_clock.vcxproj"),
                "/p:Configuration=Release", "/p:Platform=x64", "/verbosity:minimal"], check=True)
for arguments in [[], ["fallback"]]:
    subprocess.run([str(output / "bin/guest_clock.exe"), *arguments], check=True)
