"""Exercise the maintained VP traversal while an idle-voice trap is pending.

Guest layout and DSP output are mocked. Checks notification ownership, call
order, later voice lists and publication of VP monitor samples.
"""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/audio-voice-tests"
out.mkdir(parents=True, exist_ok=True)
vp = (root / "runtime_overrides/src/apu/apu_vp.c").read_text()
frame = vp[vp.index("void mcpx_apu_vp_frame("):vp.index("void mcpx_apu_vp_init(")]
code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define MCPX_HW_MAX_VOICES 256
#define NUM_MIXBINS 32
#define NUM_SAMPLES_PER_FRAME 32
#define MCPX_APU_DEBUG_MON_VP 1
#define NV_PAVS_VOICE_TAR_PITCH_LINK 0
#define NV_PAVS_VOICE_TAR_PITCH_LINK_NEXT_VOICE_HANDLE 1
#define NV_PAVS_VOICE_PAR_STATE 2
#define NV_PAVS_VOICE_PAR_STATE_ACTIVE_VOICE 3
#define NV_PAPU_FECTL 9
#define NV_PAPU_FECTL_FEMETHMODE 3
#define NV_PAPU_FECTL_FEMETHMODE_TRAPPED 2
#define SE2FE_IDLE_VOICE 42
#define DPRINTF(...) ((void)0)
typedef unsigned hwaddr;
typedef struct MCPXAPUState {
    unsigned regs[16];
    int ep_frame_div;
    struct { float sample_buf[32][2]; } vp;
    struct { int point; int16_t frame_buf[256][2]; } monitor;
} MCPXAPUState;
static struct { struct { struct { int active; } v[256]; } vp; } g_dbg;
static const struct { hwaddr top, current, next; } voice_list_regs[] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}
};
static unsigned links[256], active[256], calls[256], call_count;
static unsigned trapped_voice, trap_count;
static uint64_t s_idle_traps, s_deferred_idle_voices;
static int apu_signal_meter_enabled(void) { return 1; }
static unsigned voice_get_mask(MCPXAPUState *d, unsigned v, unsigned reg, unsigned mask)
{ assert(v < 256); return mask == 1 ? links[v] : active[v]; }
static void fe_method(MCPXAPUState *d, unsigned method, unsigned voice)
{
    assert(method == SE2FE_IDLE_VOICE);
    trapped_voice = voice;
    trap_count++;
    d->regs[NV_PAPU_FECTL] = NV_PAPU_FECTL_FEMETHMODE_TRAPPED;
}
static void voice_process(MCPXAPUState *d, float bins[32][32], float samples[32][2], unsigned v, int list)
{
    calls[call_count++] = v;
    g_dbg.vp.v[v].active = 1;
    for (unsigned i = 0; i < 32; i++)
    {
        bins[0][i] += 0.125f;
        samples[i][0] += 0.125f;
    }
}
static void src_float_to_short_array(float *in, int16_t *out, int n)
{ for (int i = 0; i < n; i++) out[i] = (int16_t)(in[i] * 32767); }
'''
code += frame
code += r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    MCPXAPUState state = {0};
    float bins[32][32] = {0};
    for (unsigned i = 0; i < 256; i++) links[i] = 0xFFFF;
    state.regs[0] = 0; state.regs[3] = 2; state.regs[6] = 4;
    links[0] = 1; links[2] = 3;
    active[1] = active[3] = active[4] = 1;
    mcpx_apu_vp_frame(&state, bins);
    CHECK(trap_count == 1 && trapped_voice == 0);
    CHECK(call_count == 3 && calls[0] == 1 && calls[1] == 3 && calls[2] == 4);
    CHECK(bins[0][0] == .375f);
    CHECK(s_idle_traps == 1 && s_deferred_idle_voices == 1);

    /* Unacknowledged traps retain their owner across frames. */
    call_count = 0;
    state.monitor.point = MCPX_APU_DEBUG_MON_VP;
    memset(bins, 0, sizeof bins);
    mcpx_apu_vp_frame(&state, bins);
    CHECK(trap_count == 1 && trapped_voice == 0 && call_count == 3);
    CHECK(state.monitor.frame_buf[0][0] == (int16_t)(.375f * 32767));
    CHECK(bins[0][0] == 0);
    CHECK(s_idle_traps == 1 && s_deferred_idle_voices == 3);

    /* Guest services/removes the first idle voice; the next idle voice can
     * then own the trap, without suppressing the other lists. */
    state.regs[NV_PAPU_FECTL] = 0;
    state.monitor.point = 0;
    state.regs[0] = 1;
    call_count = 0;
    mcpx_apu_vp_frame(&state, bins);
    CHECK(trap_count == 2 && trapped_voice == 2 && call_count == 3);
    CHECK(calls[0] == 1 && calls[1] == 3 && calls[2] == 4);

    /* Empty lists clear processed membership and report no new trap. */
    state.regs[0] = state.regs[3] = state.regs[6] = 0xFFFF;
    call_count = 0;
    mcpx_apu_vp_frame(&state, bins);
    CHECK(trap_count == 2 && call_count == 0);
    CHECK(!g_dbg.vp.v[1].active && !g_dbg.vp.v[3].active && !g_dbg.vp.v[4].active);
    puts("VP traversal preserves pending trap ownership and mixes later voices/lists");
    return 0;
}
'''
source = out / "voice_traversal.c"
source.write_text(code)
cache = (root / "build/portable/compile/CMakeCache.txt").read_text()
compiler = re.search(r"^CMAKE_C_COMPILER:[^=]+=(.+)$", cache, re.M).group(1)
exe = out / "voice_traversal.exe"
subprocess.run([compiler, str(source), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
