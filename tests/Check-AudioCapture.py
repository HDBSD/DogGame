"""Exercise the real capture lifecycle and deferred menu/status helpers.

Audio mixing/voice diagnostics are stubbed; the recorder writes real WAV/TXT
files in the generated fixture directory without requiring game assets.
"""
from pathlib import Path
import subprocess
from test_support import msbuild

root = Path(__file__).resolve().parents[1]
out = root / "tests/irq-test-build"
capture = (root / "runtime_overrides/src/apu/apu_core.c").read_text()
debug = (root / "src/title_debug.c").read_text()

def section(source, begin, end):
    return source[source.index(begin):source.index(end, source.index(begin))]

code = r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
'''
code += f'#include "{(root / "runtime_overrides/include/xbox/audio_capture.h").as_posix()}"\n'
code += r'''
static int fail_open;
static char wave_path[256];
static FILE *capture_fopen(const char *path, const char *mode)
{
    if (fail_open == 1 || (fail_open == 2 && strstr(path, ".txt"))) return NULL;
    if (strstr(path, ".wav")) snprintf(wave_path, sizeof wave_path, "%s", path);
    return fopen(path, mode);
}
#define fopen capture_fopen
typedef struct MCPXAPUState
{
    struct { int16_t frame_buf[256][2]; } monitor;
} MCPXAPUState;
'''
code += section(capture, "#define AUDIO_HISTORY_FRAMES", "static void audio_capture_header")
code += section(capture, "static void audio_capture_header", "static void audio_capture_snapshot")
code += 'static void audio_capture_snapshot(MCPXAPUState *state, const char *phase) { if (audio_capture_state) fprintf(audio_capture_state, "%s\\n", phase); }\n'
code += section(capture, "static void audio_capture_close", "/* ============================================================\n * Debug frame markers")
code += r'''
static volatile LONG s_pending_capture;
static DWORD s_profile_until, s_trace_until;
enum { DEBUG_CAPTURE_PROFILE = 1, DEBUG_CAPTURE_RENDER_TRACE = 2, DEBUG_CAPTURE_AUDIO = 3 };
static unsigned menu_mode, closed, profiles, traces;
static unsigned title_state_menu_mode(void) { return menu_mode; }
static void close_debug(void) { closed++; }
static void nv2a_debug_profile_request(void) { profiles++; }
static void nv2a_debug_trace_seconds_request(unsigned seconds) { if (seconds == 10) traces++; }
static char drawn[160];
static void draw_debug_text(HDC dc, int x, int y, const char *text) { snprintf(drawn, sizeof drawn, "%s", text); }
'''
code += section(debug, "static void arm_gameplay_capture", "void title_debug_frame")
code += section(debug, "static void draw_audio_capture_status", "static void debug_draw")
code += r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    _putenv_s("RECOMP_DEBUG", "1");
    MCPXAPUState state = {0};
    for (unsigned i = 0; i < 256; i++) state.monitor.frame_buf[i][0] = 0x1234;
    for (unsigned frame = 0; frame < 1875; frame++) audio_capture_frame(&state);
    CHECK(audio_history_count == AUDIO_HISTORY_FRAMES);
    CHECK(apu_debug_audio_status(NULL) == AUDIO_CAPTURE_IDLE);
    menu_mode = 7;
    arm_gameplay_capture(DEBUG_CAPTURE_AUDIO);
    CHECK(closed == 1);
    start_pending_gameplay_capture();
    CHECK(s_pending_capture == DEBUG_CAPTURE_AUDIO && !audio_capture_request);
    menu_mode = 2; start_pending_gameplay_capture(); CHECK(!audio_capture_request);
    menu_mode = 11; start_pending_gameplay_capture();
    CHECK(!s_pending_capture && apu_debug_audio_status(NULL) == AUDIO_CAPTURE_REQUESTED);
    apu_debug_audio_request(); CHECK(audio_capture_request == 1);
    start_pending_gameplay_capture(); CHECK(audio_capture_request == 1);
    audio_capture_frame(&state);
    unsigned remaining, previous = 10000;
    CHECK(apu_debug_audio_status(&remaining) == AUDIO_CAPTURE_RECORDING && remaining < previous);
    while (audio_post_frames)
    {
        previous = remaining;
        apu_debug_audio_request(); // repeated F8 must not restart/queue another recording
        CHECK(apu_debug_audio_status(NULL) == AUDIO_CAPTURE_RECORDING);
        audio_capture_frame(&state);
        apu_debug_audio_status(&remaining);
        CHECK(remaining <= previous);
    }
    CHECK(apu_debug_audio_status(&remaining) == AUDIO_CAPTURE_COMPLETE && !remaining && !audio_capture_request);
    CHECK(audio_capture_bytes == AUDIO_HISTORY_FRAMES * 8);
    FILE *wave = fopen(wave_path, "rb"); CHECK(wave);
    char riff[4]; uint32_t bytes;
    CHECK(fread(riff, 1, 4, wave) == 4 && !memcmp(riff, "RIFF", 4));
    fseek(wave, 40, SEEK_SET); CHECK(fread(&bytes, 4, 1, wave) == 1 && bytes == audio_capture_bytes);
    fseek(wave, 0, SEEK_END); CHECK(ftell(wave) == (long)audio_capture_bytes + 44); fclose(wave);
    fail_open = 1;
    apu_debug_audio_request(); audio_capture_frame(&state);
    CHECK(apu_debug_audio_status(NULL) == AUDIO_CAPTURE_FAILED);
    fail_open = 2;
    apu_debug_audio_request(); audio_capture_frame(&state);
    while (audio_post_frames) audio_capture_frame(&state);
    CHECK(apu_debug_audio_status(NULL) == AUDIO_CAPTURE_FAILED);
    fail_open = 0;
    apu_debug_audio_request(); audio_capture_frame(&state);
    audio_capture_close(); // shutdown before ten following seconds completed
    CHECK(apu_debug_audio_status(NULL) == AUDIO_CAPTURE_FAILED);
    arm_gameplay_capture(DEBUG_CAPTURE_PROFILE); start_pending_gameplay_capture(); CHECK(profiles == 1);
    arm_gameplay_capture(DEBUG_CAPTURE_RENDER_TRACE); start_pending_gameplay_capture(); CHECK(traces == 1);
    HDC dc = CreateCompatibleDC(NULL); CHECK(dc);
    audio_capture_publish(AUDIO_CAPTURE_REQUESTED, 0);
    draw_audio_capture_status(dc, 100); CHECK(strstr(drawn, "waiting"));
    audio_capture_publish(AUDIO_CAPTURE_RECORDING, 7500);
    draw_audio_capture_status(dc, 200); CHECK(strstr(drawn, "8 s remaining"));
    audio_capture_publish(AUDIO_CAPTURE_COMPLETE, 0);
    draw_audio_capture_status(dc, 1000); CHECK(strstr(drawn, "saved"));
    drawn[0] = 0; draw_audio_capture_status(dc, 6001); CHECK(!drawn[0]);
    audio_capture_publish(AUDIO_CAPTURE_FAILED, 0);
    draw_audio_capture_status(dc, 7000); CHECK(strstr(drawn, "failed"));
    DeleteDC(dc);
    free(audio_history); audio_history = NULL;
    apu_debug_audio_request(); audio_capture_frame(&state);
    CHECK(apu_debug_audio_status(NULL) == AUDIO_CAPTURE_FAILED);
    puts("Passed: deferred gameplay trigger, duplicate requests, pre/post WAV, countdown, file failures, missing history and on-screen notices.");
}
'''
(out / "audio_capture.c").write_text(code)
project = (out / "apu_irq_harness.vcxproj").read_text()
project = project.replace("apu_irq_harness.c", "audio_capture.c").replace("obj\\apu_irq\\", "obj\\audio_capture\\")
project = project.replace("</ItemDefinitionGroup>", "<Link><AdditionalDependencies>gdi32.lib;%(AdditionalDependencies)</AdditionalDependencies></Link></ItemDefinitionGroup>")
(out / "audio_capture.vcxproj").write_text(project)
subprocess.run([msbuild, str(out / "audio_capture.vcxproj"), "/p:Configuration=Release", "/p:Platform=x64", "/verbosity:minimal"], check=True)
working = out / "audio-capture"
working.mkdir(exist_ok=True)
subprocess.run([str(out / "bin/audio_capture.exe")], cwd=working, check=True)
