"""Exercise real framebuffer publication/leases with a stalled reader.

Vulkan must publish coherent newer frames without overwriting the reader's
borrowed image. DirectX retains its existing global-lock ownership contract.
"""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/vulkan-presentation-tests"
out.mkdir(parents=True, exist_ok=True)
source = (root / "runtime_overrides/src/video/fb_present.c").read_text()

def function(signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"

code = r'''
#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static int vulkan = 1;
static int xbox_ExperimentalVulkanSelected(void) { return vulkan; }
static int xbox_DisplayInternalScaling(void) { return 1; }
static uint32_t native_pixels[64];
static ptrdiff_t xbox_GetMemoryOffset(void) { return (ptrdiff_t)((uintptr_t)native_pixels - 0x10000); }
static void display_limit_frame(void) {}
static int s_fb_running = 1;
static uint32_t s_fb_width = 8, s_fb_height = 8;
static uint32_t *s_present[3], *s_high_present[3];
static volatile LONG s_present_idx = -1;
static SRWLOCK s_present_lock = SRWLOCK_INIT;
static SRWLOCK s_snapshot_slots[3] = {SRWLOCK_INIT, SRWLOCK_INIT, SRWLOCK_INIT};
static LONG s_borrowed_slot = -1;
static const uint32_t *s_high_rgb;
static size_t s_high_capacity[3];
static unsigned s_high_width[3], s_high_height[3], s_high_draw_width, s_high_draw_height;
static void (*s_world_capture)(void);
'''
for name in ("static int acquire_present_snapshot(", "static void release_present_snapshot(",
             "static void borrow_high_snapshot(", "static void release_high_snapshot(",
             "void xbox_FramebufferWindowPresentHigh("):
    code += function(name)
code += r'''
static DWORD WINAPI publish_frames(void *unused)
{
    uint32_t high[256];
    for (unsigned frame = 1; frame <= 1000; frame++)
    {
        for (unsigned i = 0; i < 64; i++) native_pixels[i] = frame;
        for (unsigned i = 0; i < 256; i++) high[i] = frame;
        xbox_FramebufferWindowPresentHigh(0x10000, 32, high, 16, 16);
    }
    return 0;
}
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    _putenv_s("RECOMP_FB_VA", "");
    uint32_t high[256] = {0};
    xbox_FramebufferWindowPresentHigh(0x10000, 32, high, 16, 16);
    CHECK(acquire_present_snapshot()); borrow_high_snapshot();
    CHECK(s_borrowed_slot >= 0 && s_high_rgb && s_high_draw_width == 16);
    CHECK(!acquire_present_snapshot());
    const uint32_t *borrowed = s_high_rgb;
    HANDLE writer = CreateThread(NULL, 0, publish_frames, NULL, 0, NULL);
    CHECK(writer && WaitForSingleObject(writer, 2000) == WAIT_OBJECT_0);
    for (unsigned i = 0; i < 256; i++) CHECK(borrowed[i] == 0);
    release_high_snapshot(); release_present_snapshot(); CloseHandle(writer);
    CHECK(acquire_present_snapshot()); borrow_high_snapshot();
    CHECK(s_high_rgb && s_high_rgb[0] == 1000 && s_high_rgb[255] == 1000);
    CHECK(s_present[s_borrowed_slot][0] == 1000);
    release_high_snapshot(); release_present_snapshot();

    /* DirectX still holds the global reader lock until drawing completes. */
    vulkan = 0;
    CHECK(acquire_present_snapshot()); borrow_high_snapshot();
    CHECK(s_borrowed_slot == -1);
    writer = CreateThread(NULL, 0, publish_frames, NULL, 0, NULL);
    CHECK(writer && WaitForSingleObject(writer, 20) == WAIT_TIMEOUT);
    release_high_snapshot(); release_present_snapshot();
    CHECK(WaitForSingleObject(writer, 2000) == WAIT_OBJECT_0); CloseHandle(writer);
    for (unsigned i = 0; i < 3; i++) { free(s_present[i]); free(s_high_present[i]); }
    puts("Vulkan stalled-reader publication, immutable leases, coherent frames and unchanged DirectX locking passed");
    return 0;
}
'''
path = out / "presentation.c"
path.write_text(code)
cache = (root / "build/portable/compile/CMakeCache.txt").read_text()
compiler = re.search(r"^CMAKE_C_COMPILER:[^=]+=(.+)$", cache, re.M).group(1)
exe = out / "presentation.exe"
subprocess.run([compiler, str(path), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
