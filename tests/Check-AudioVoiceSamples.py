"""Exercise the maintained sample reader with synthetic guest memory.

Uses the portable compiler from the configured game build. The fixture keeps
guest reads, offsets and completion notifications observable; it does not
validate the Xbox layout or establish a cause of the gameplay corruption.
"""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/audio-voice-tests"
out.mkdir(parents=True, exist_ok=True)
vp = (root / "runtime_overrides/src/apu/apu_vp.c").read_text()
reader = vp[vp.index("static int voice_get_samples("):vp.index("/* ============================================================\n * Streaming linear resampling")]
constants = sorted(set(re.findall(r"\b(?:NV\w+|MCPX_HW_NOTIFIER_SSLA_DONE)\b", reader)))
values = {
    "NV_PAVS_VOICE_CFG_FMT_CONTAINER_SIZE_ADPCM": 2,
    "NV_PAVS_VOICE_CFG_FMT_SAMPLE_SIZE_U8": 0,
    "NV_PAVS_VOICE_CFG_FMT_SAMPLE_SIZE_S16": 1,
    "NV_PAVS_VOICE_CFG_FMT_SAMPLE_SIZE_S24": 2,
    "NV_PAVS_VOICE_CFG_FMT_SAMPLE_SIZE_S32": 3,
}
code = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define MCPX_HW_MAX_VOICES 1
#define ADPCM_SAMPLES_PER_BLOCK 64
typedef uint32_t hwaddr;
typedef struct MCPXAPUVoiceFilter {
    bool adpcm_valid, adpcm_stream;
    uint32_t adpcm_address, adpcm_sge, adpcm_size, adpcm_channels;
    int16_t adpcm_samples[130];
} MCPXAPUVoiceFilter;
typedef struct MCPXAPUState {
    uint32_t regs[512];
    struct {
        MCPXAPUVoiceFilter filters[1];
        struct { int ssl_index, ssl_seg, base[2], count[2]; } ssl[1];
    } vp;
} MCPXAPUState;
struct McpxApuDebugVoice {
    bool stream, loop, persist;
    uint32_t ebo, cbo, lbo, ba;
    int container_size, sample_size;
    unsigned samples_per_block;
};
static struct { struct { struct McpxApuDebugVoice v[1]; } vp; } g_dbg;
static unsigned char memory[4096];
static uint32_t fields[512];
static int address_space_memory;
static unsigned notifications, completions;
'''
for i, name in enumerate(constants, 32):
    code += f"#define {name} {values.get(name, i)}\n"
code += r'''
static uint32_t voice_get_mask(MCPXAPUState *d, uint16_t v, unsigned reg, unsigned mask)
{ return fields[mask]; }
static void voice_set_mask(MCPXAPUState *d, uint16_t v, unsigned reg, unsigned mask, uint32_t value)
{ fields[mask] = value; }
static void voice_off(MCPXAPUState *d, uint16_t v) { completions++; }
static void set_notify_status(MCPXAPUState *d, unsigned v, unsigned slot, unsigned status)
{ notifications++; }
static void *mcpx_apu_phys(unsigned addr) { assert(addr < sizeof memory); return memory + addr; }
static uint32_t get_data_ptr(unsigned sge, unsigned mask, unsigned addr) { return addr; }
static unsigned ldub_phys(int space, unsigned addr) { assert(addr < sizeof memory); return memory[addr]; }
static unsigned lduw_le_phys(int space, unsigned addr)
{ return ldub_phys(space, addr) | (ldub_phys(space, addr + 1) << 8); }
static unsigned ldl_le_phys(int space, unsigned addr)
{ return lduw_le_phys(space, addr) | (lduw_le_phys(space, addr + 2) << 16); }
static float uint8_to_float(uint8_t v) { return ((int)v - 128) / 128.0f; }
static float int16_to_float(int16_t v) { return v / 32768.0f; }
static float int24_to_float(uint32_t v) { return (int32_t)(v << 8) / 2147483648.0f; }
static float int32_to_float(uint32_t v) { return (int32_t)v / 2147483648.0f; }
/* These fixtures exercise PCM; reaching ADPCM is a fixture failure. */
static int adpcm_decode_block(int16_t *out, const uint8_t *in, size_t size, int channels)
{ assert(!"unexpected ADPCM"); return 0; }
'''
code += reader
code += r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
static void store32(unsigned addr, uint32_t value) { memcpy(memory + addr, &value, 4); }
int main(void)
{
    MCPXAPUState state = {0};
    float samples[4][2] = {0};
    const int16_t pcm[] = {1000, -2000, 3000, -4000};
    memcpy(memory + 512, pcm, sizeof pcm);
    fields[NV_PAVS_VOICE_CFG_FMT_SAMPLE_SIZE] = NV_PAVS_VOICE_CFG_FMT_SAMPLE_SIZE_S16;
    fields[NV_PAVS_VOICE_CFG_FMT_CONTAINER_SIZE] = 1;
    fields[NV_PAVS_VOICE_CUR_PSL_START_BA] = 512;
    fields[NV_PAVS_VOICE_PAR_NEXT_EBO] = 3;
    CHECK(voice_get_samples(&state, 0, samples, 4) == 4);
    for (unsigned i = 0; i < 4; i++)
    {
        CHECK(samples[i][0] == int16_to_float(pcm[i]));
        CHECK(samples[i][1] == samples[i][0]);
    }
    CHECK(completions == 1);
    CHECK(fields[NV_PAVS_VOICE_PAR_OFFSET_CBO] == 3);

    /* A segment overrides an initially byte-sized voice container. */
    fields[NV_PAVS_VOICE_CFG_FMT_DATA_TYPE] = 1;
    fields[NV_PAVS_VOICE_CFG_FMT_PERSIST] = 1;
    fields[NV_PAVS_VOICE_CFG_FMT_CONTAINER_SIZE] = 0;
    fields[NV_PAVS_VOICE_PAR_OFFSET_CBO] = 0;
    state.regs[NV_PAPU_VPSSLADDR] = 128;
    state.vp.ssl[0].count[0] = 1;
    store32(128, 512);
    store32(132, 4 | (1 << 16));
    CHECK(voice_get_samples(&state, 0, samples, 4) == 4);
    for (unsigned i = 0; i < 4; i++) CHECK(samples[i][0] == int16_to_float(pcm[i]));
    CHECK(notifications == 1);
    CHECK(completions == 1);
    CHECK(state.vp.ssl[0].ssl_index == 1);
    CHECK(fields[NV_PAVS_VOICE_PAR_OFFSET_CBO] == 0);
    CHECK(g_dbg.vp.v[0].stream && g_dbg.vp.v[0].ba == 512);

    /* Stereo PCM uses the segment width for channel spacing as well as
     * frame advancement; completing a segment must still notify once. */
    state.vp.ssl[0].ssl_index = 0;
    state.vp.ssl[0].ssl_seg = 0;
    fields[NV_PAVS_VOICE_CFG_FMT_STEREO] = 1;
    fields[NV_PAVS_VOICE_CFG_FMT_SAMPLES_PER_BLOCK] = 1;
    store32(132, 2 | (1 << 16) | (1 << 18) | (1 << 23));
    CHECK(voice_get_samples(&state, 0, samples, 2) == 2);
    CHECK(samples[0][0] == int16_to_float(pcm[0]));
    CHECK(samples[0][1] == int16_to_float(pcm[1]));
    CHECK(samples[1][0] == int16_to_float(pcm[2]));
    CHECK(samples[1][1] == int16_to_float(pcm[3]));
    CHECK(notifications == 2 && completions == 1);
    puts("PCM sample reader and segment completion passed");
    return 0;
}
'''
source = out / "voice_samples.c"
source.write_text(code)
cache = (root / "build/portable/compile/CMakeCache.txt").read_text()
compiler = re.search(r"^CMAKE_C_COMPILER:[^=]+=(.+)$", cache, re.M).group(1)
exe = out / "voice_samples.exe"
subprocess.run([compiler, "-Wno-unused-function", str(source), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
