"""Exercise production renderer settings against an isolated temporary INI."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/vulkan-settings-tests"
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
#include <stdio.h>
#include <wchar.h>
static int vulkan;
static int xbox_ExperimentalVulkanSelected(void) { return vulkan; }
static wchar_t s_display_ini[MAX_PATH];
static unsigned s_resolution;
static volatile LONG s_internal_scaling, s_internal_resolution, s_resolution_request;
static volatile LONG s_fps_cap, s_widescreen, s_mouse_interaction, s_dynamic_prompts, s_pause_on_focus_loss, s_borderless;
'''
code += source[source.index("static const unsigned s_resolutions"):source.index("static unsigned s_resolution;")]
for name in ("static const wchar_t *display_renderer_section(", "unsigned xbox_DisplayResolutionCount(",
             "unsigned xbox_DisplayResolutionSelected(", "void xbox_DisplayInternalScalingSet(",
             "void xbox_DisplayFpsCapSet(", "void xbox_DisplayResolutionSelect(", "static void display_load("):
    code += function(name)
code += r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    display_load(); DeleteFileW(s_display_ini);
    WritePrivateProfileStringW(L"Display", L"InternalScaling", L"0", s_display_ini);
    WritePrivateProfileStringW(L"Display", L"InternalResolution", L"2", s_display_ini);
    WritePrivateProfileStringW(L"Display", L"FpsCap30", L"0", s_display_ini);
    display_load();
    CHECK(!s_internal_scaling && !s_fps_cap && s_internal_resolution == 2);
    CHECK(xbox_DisplayResolutionCount() == 8);
    vulkan = 1; display_load();
    CHECK(s_internal_scaling && s_fps_cap && xbox_DisplayResolutionSelected() == 0);
    CHECK(s_resolutions[0][0] == 640 && s_resolutions[0][1] == 480);
    CHECK(xbox_DisplayResolutionCount() == 9 && s_resolutions[8][0] == 3840 && s_resolutions[8][1] == 2160);
    xbox_DisplayResolutionSelect(6);
    xbox_DisplayInternalScalingSet(0);
    xbox_DisplayFpsCapSet(0);
    CHECK(GetPrivateProfileIntW(L"Vulkan", L"InternalResolution", 0, s_display_ini) == 6);
    CHECK(GetPrivateProfileIntW(L"Vulkan", L"InternalScaling", 1, s_display_ini) == 0);
    CHECK(GetPrivateProfileIntW(L"Vulkan", L"FpsCap30", 1, s_display_ini) == 0);
    CHECK(GetPrivateProfileIntW(L"Display", L"InternalResolution", 0, s_display_ini) == 2);
    vulkan = 0; display_load();
    CHECK(!s_internal_scaling && !s_fps_cap && s_internal_resolution == 2);
    vulkan = 1; display_load();
    CHECK(!s_internal_scaling && !s_fps_cap && s_internal_resolution == 6);
    DeleteFileW(s_display_ini);
    vulkan = 0; display_load();
    CHECK(!s_internal_scaling && !s_fps_cap && s_internal_resolution == 0);
    puts("Vulkan native-resolution/30 defaults, saved preferences and DirectX settings isolation passed");
    return 0;
}
'''
path = out / "settings.c"
path.write_text(code)
cache = (root / "build/portable/compile/CMakeCache.txt").read_text()
compiler = re.search(r"^CMAKE_C_COMPILER:[^=]+=(.+)$", cache, re.M).group(1)
exe = out / "settings.exe"
subprocess.run([compiler, str(path), "-o", str(exe)], check=True)
subprocess.run([str(exe)], cwd=out, check=True)
