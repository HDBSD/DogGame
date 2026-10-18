"""Check focus-event ownership, persistence and title-thread pause input.

Uses the actual setting/request and input-hook functions, synthetic decoded
guest input, and a temporary INI. Does not simulate native menu acceptance.
"""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/focus-loss-tests"
out.mkdir(parents=True, exist_ok=True)
video = (root / "runtime_overrides/src/video/fb_present.c").read_text()
display = (root / "src/title_display.c").read_text()
guest = (root / "src/api/title_guest_menu.c").read_text()

def function(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    level = 1
    end = brace + 1
    while level:
        level += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"

code = r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
static wchar_t s_display_ini[MAX_PATH];
static volatile LONG s_pause_on_focus_loss = 1, s_focus_loss_pending;
'''
for name in ("int xbox_DisplayPauseOnFocusLoss(void)",
             "void xbox_DisplayPauseOnFocusLossSet(int enabled)",
             "int xbox_DisplayConsumeFocusLoss(void)", "static void queue_focus_loss(void)"):
    code += function(video, name)
code += r'''
enum {
    GUEST_ADDRESS_MINIMUM = 16, GUEST_MEMORY_END = 512,
    CONTROLLER_TABLE_ADDRESS = 0, CONTROLLER_BYTES_REQUIRED = 0x34,
    CONTROLLER_PRESSED_OFFSET = 0x30, BUTTON_START = 1
};
static uint32_t memory[128], mode;
#define MEM32(address) memory[(address) / 4]
static uint32_t title_menu_current_mode(void) { return mode; }
static void trace_native_menu_input(const char *boundary) {}
static void remap_start_to_back_if_needed(uint32_t value) {}
'''
code += function(guest, "static int guest_object_readable(")
code += function(guest, "int title_menu_request_pause(void)")
code += function(display, "void title_display_input_frame(void)")
code += r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    GetFullPathNameW(L"focus-test.ini", MAX_PATH, s_display_ini, NULL);
    DeleteFileW(s_display_ini);
    CHECK(xbox_DisplayPauseOnFocusLoss());
    queue_focus_loss();
    CHECK(xbox_DisplayConsumeFocusLoss());
    CHECK(!xbox_DisplayConsumeFocusLoss());
    queue_focus_loss();
    xbox_DisplayPauseOnFocusLossSet(0);
    CHECK(!xbox_DisplayConsumeFocusLoss());
    queue_focus_loss();
    CHECK(!xbox_DisplayConsumeFocusLoss());
    CHECK(GetPrivateProfileIntW(L"Display", L"PauseOnFocusLoss", 1, s_display_ini) == 0);
    xbox_DisplayPauseOnFocusLossSet(1);
    CHECK(GetPrivateProfileIntW(L"Display", L"PauseOnFocusLoss", 0, s_display_ini) == 1);
    CHECK(!xbox_DisplayConsumeFocusLoss());

    memory[0] = 64;
    MEM32(64 + 0x2C) = 0x80;
    MEM32(64 + CONTROLLER_PRESSED_OFFSET) = 0x200;
    mode = 11;
    queue_focus_loss();
    title_display_input_frame();
    CHECK(MEM32(64 + CONTROLLER_PRESSED_OFFSET) == 0x201);
    CHECK(MEM32(64 + 0x2C) == 0x80);
    MEM32(64 + CONTROLLER_PRESSED_OFFSET) = 0;
    title_display_input_frame();
    CHECK(!MEM32(64 + CONTROLLER_PRESSED_OFFSET));

    /* Already-paused/menu requests cannot unpause or survive into gameplay. */
    mode = 7;
    queue_focus_loss();
    title_display_input_frame();
    mode = 11;
    title_display_input_frame();
    CHECK(!MEM32(64 + CONTROLLER_PRESSED_OFFSET));
    mode = 2;
    queue_focus_loss();
    title_display_input_frame();
    CHECK(!MEM32(64 + CONTROLLER_PRESSED_OFFSET));

    memory[0] = 0;
    CHECK(!title_menu_request_pause());
    memory[0] = GUEST_MEMORY_END - CONTROLLER_BYTES_REQUIRED;
    CHECK(!title_menu_request_pause());
    CHECK(!MEM32(64 + CONTROLLER_PRESSED_OFFSET));
    DeleteFileW(s_display_ini);
    puts("Focus-loss defaults, INI persistence, one-shot delivery, menu gating and pause edges passed");
    return 0;
}
'''
source = out / "focus_loss.c"
source.write_text(code)
cache = (root / "build/portable/compile/CMakeCache.txt").read_text()
compiler = re.search(r"^CMAKE_C_COMPILER:[^=]+=(.+)$", cache, re.M).group(1)
exe = out / "focus_loss.exe"
subprocess.run([compiler, str(source), "-o", str(exe)], check=True)
subprocess.run([str(exe)], cwd=out, check=True)
