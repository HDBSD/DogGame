"""Check the actual float mixdown and its observation-only meters."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/audio-voice-tests"
out.mkdir(parents=True, exist_ok=True)
dsp = (root / "runtime_overrides/src/apu/apu_dsp.c").read_text()
frame = dsp[dsp.index("void mcpx_apu_dsp_frame("):]
code = f'#include "{(root / "runtime_overrides/src/apu/apu_signal_meter.h").as_posix()}"\n'
code += r'''
#include <assert.h>
#define NUM_MIXBINS 32
#define NUM_SAMPLES_PER_FRAME 32
#define MCPX_APU_DEBUG_MON_VP 1
typedef struct MCPXAPUState {
    int ep_frame_div;
    struct { int point; int16_t frame_buf[256][2]; } monitor;
} MCPXAPUState;
static struct { struct { int cycles; } gp, ep; } g_dbg;
static ApuSignalMeter s_mixdown_meter;
static int all_bins;
static int mcpx_apu_mixdown_all(void) { return all_bins; }
'''
code += frame
code += frame.replace("void mcpx_apu_dsp_frame(", "void dsp_frame_without_meters(").replace(
    "int meter_enabled = apu_signal_meter_enabled();", "int meter_enabled = 0;")
code += r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    _putenv_s("RECOMP_DEBUG", "1");
    ApuSignalMeter meter = {0};
    const float inputs[] = {0, 1, -1, 1.25f, -2, NAN, INFINITY};
    for (unsigned i = 0; i < 7; i++) apu_signal_meter_sample(&meter, inputs[i]);
    CHECK(meter.samples == 7 && meter.over_range == 2);
    CHECK(meter.non_finite == 2 && meter.peak == 2);
    FILE *report = tmpfile();
    CHECK(report);
    apu_signal_meter_write(report, "test", &meter);
    CHECK(!meter.samples && !meter.over_range && !meter.non_finite && !meter.peak);
    rewind(report);
    char text[256] = {0};
    CHECK(fgets(text, sizeof text, report));
    CHECK(strstr(text, "test_samples=7") && strstr(text, "test_nonfinite=2"));
    fclose(report);

    MCPXAPUState state = {0};
    float bins[32][32] = {0};
    bins[0][0] = .75f;
    bins[2][0] = .75f;
    bins[1][0] = -.75f;
    bins[3][0] = -.75f;
    all_bins = 1;
    mcpx_apu_dsp_frame(&state, bins);
    CHECK(state.monitor.frame_buf[0][0] == 32767);
    CHECK(state.monitor.frame_buf[0][1] == -32767);
    CHECK(s_mixdown_meter.samples == 64 && s_mixdown_meter.over_range == 2);
    CHECK(s_mixdown_meter.peak == 1.5f);

    /* Disabling observation must leave the mixed samples identical. */
    state.ep_frame_div = 1;
    dsp_frame_without_meters(&state, bins);
    CHECK(!memcmp(state.monitor.frame_buf[0], state.monitor.frame_buf[32], 32 * 4));
    CHECK(s_mixdown_meter.samples == 64);
    all_bins = 0;
    mcpx_apu_dsp_frame(&state, bins);
    CHECK(state.monitor.frame_buf[32][0] == (int16_t)(.75f * 32767));
    CHECK(state.monitor.frame_buf[32][1] == (int16_t)(-.75f * 32767));
    puts("Signal meters, report reset, DSP clipping and stereo bin routing passed");
    return 0;
}
'''
source = out / "signal_meters.c"
source.write_text(code)
cache = (root / "build/portable/compile/CMakeCache.txt").read_text()
compiler = re.search(r"^CMAKE_C_COMPILER:[^=]+=(.+)$", cache, re.M).group(1)
exe = out / "signal_meters.exe"
subprocess.run([compiler, str(source), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
