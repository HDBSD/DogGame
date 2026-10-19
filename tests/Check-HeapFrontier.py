"""Exercise the production guest heap with the E009 failure boundary and churn."""
from pathlib import Path
import subprocess
import json
import struct

root = Path(__file__).resolve().parents[1]
out = root / "build/heap-frontier-test"
out.mkdir(parents=True, exist_ok=True)
source = (root / "runtime_overrides/src/kernel/xbox_memory_layout.c").read_text()
metadata = source[source.index("static uint32_t g_heap_next ="):source.index("/*\n * Simulated stacks")]
allocator_start = source.index("static uint32_t heap_alloc_locked(uint32_t size, uint32_t alignment);")
allocator = source[allocator_start:source.index("HANDLE xbox_GetMappingHandle", allocator_start)]
prefix = r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define XBOX_HEAP_BASE 0x00F80000u
#define XBOX_HEAP_TOP 0x04000000u
static unsigned char ram[XBOX_HEAP_TOP];
static uintptr_t g_memory_offset;
'''
checks = r'''
static void reset(void)
{
    memset(g_heap_blocks, 0, sizeof g_heap_blocks);
    g_heap_block_count = 0;
    g_heap_next = XBOX_HEAP_BASE;
}
int main(void)
{
    _putenv_s("RECOMP_HEAP_RECLAIM", "1");
    g_memory_offset = (uintptr_t)ram;
    reset();
#ifdef REPLAY_CAPTURED_HEAP
    FILE *capture = fopen("build/heap-frontier-test/captured-heap.bin", "rb");
    assert(capture);
    assert(fread(&g_heap_block_count, 4, 1, capture) == 1);
    assert(fread(&g_heap_next, 4, 1, capture) == 1);
    assert(g_heap_block_count > 0 && g_heap_block_count < XBOX_HEAP_MAX_BLOCKS);
    assert(fread(g_heap_blocks, sizeof g_heap_blocks[0], g_heap_block_count, capture) == (size_t)g_heap_block_count);
    fclose(capture);
    uint32_t starts[XBOX_HEAP_MAX_BLOCKS], sizes[XBOX_HEAP_MAX_BLOCKS];
    unsigned live = 0;
    for (int i = 0; i < g_heap_block_count; i++)
    {
        if (!g_heap_blocks[i].size || g_heap_blocks[i].free) continue;
        starts[live] = g_heap_blocks[i].addr;
        sizes[live] = g_heap_blocks[i].size;
        ram[starts[live]] = 0x5A;
        live++;
    }
    uint32_t replay = xbox_HeapAlloc(5376016, 4096);
    assert(replay);
    for (unsigned i = 0; i < live; i++)
    {
        assert(replay + 5376016 <= starts[i] || replay >= starts[i] + sizes[i]);
        assert(ram[starts[i]] == 0x5A);
    }
    puts("Passed: full captured E009 heap accepts the sound bank without overlapping or clearing live allocations.");
    return 0;
#endif
    /* Exact captured E009 tail. Larger than either free span individually,
     * smaller than their union. Preserve a live block below it. */
    g_heap_next = 0x03F0A810;
    g_heap_blocks[0].addr = XBOX_HEAP_BASE;
    g_heap_blocks[0].size = 60895232 - XBOX_HEAP_BASE;
    g_heap_blocks[1].addr = 60895232;
    g_heap_blocks[1].size = 5208080;
    g_heap_blocks[1].free = 1;
    g_heap_block_count = 2;
    ram[XBOX_HEAP_BASE] = 0x5A;
    uint32_t bank = xbox_HeapAlloc(5376016, 4096);
#ifdef EXPECT_LEGACY_FAILURE
    assert(!bank);
    puts("Reproduced: legacy frontier rejects E009's sound bank despite enough tail space.");
    return 0;
#endif
    assert(bank == 60895232 && ram[XBOX_HEAP_BASE] == 0x5A);
    assert(xbox_HeapBlockSize(XBOX_HEAP_BASE) == 60895232 - XBOX_HEAP_BASE);
    assert(ram[bank] == 0 && ram[bank + 5376015] == 0);
    xbox_HeapFree(bank);
    assert(g_heap_next == bank);
    reset();
    /* Alignment padding must be represented, so release in either order can
     * recover the whole region. A live neighbour must never be reclaimed. */
    for (unsigned pass = 0; pass < 200; pass++)
    {
        uint32_t a = xbox_HeapAlloc(17, 16);
        uint32_t b = xbox_HeapAlloc(8193, 4096);
        uint32_t c = xbox_HeapAlloc(33, 4096);
        assert(a && b && c && !(b & 4095) && !(c & 4095));
        ram[b] = 0xA5;
        xbox_HeapFree(a);
        xbox_HeapFree(c);
        assert(ram[b] == 0xA5 && xbox_HeapBlockSize(b) >= 8193);
        xbox_HeapFree(b);
        assert(g_heap_next == XBOX_HEAP_BASE && !g_heap_block_count);
    }
    /* A genuine exhaustion fails without moving or clearing live storage. */
    uint32_t all = xbox_HeapAlloc(XBOX_HEAP_TOP - XBOX_HEAP_BASE, 16);
    assert(all == XBOX_HEAP_BASE);
    ram[all] = 0xCC;
    assert(!xbox_HeapAlloc(16, 16) && ram[all] == 0xCC);
    xbox_HeapFree(all);
    assert(g_heap_next == XBOX_HEAP_BASE);
    puts("Passed: E009 allocation, zeroing, live ownership, alignment padding, repeated reuse and real exhaustion.");
    return 0;
}
'''
compiler = root / "build/portable/tools/llvm/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe"
cases = [
    ("legacy", allocator.replace("heap_trim_free_tail();", "/* legacy frontier */"), ["-DEXPECT_LEGACY_FAILURE"]),
    ("fixed", allocator, []),
]
capture = root / "build/audio-corruption-20261019/heap-metadata.json"
if capture.exists():
    metadata_capture = json.loads(capture.read_text())
    payload = struct.pack("<II", metadata_capture["count"], metadata_capture["bump"])
    payload += b"".join(struct.pack("<IIB3x", *block) for block in metadata_capture["blocks"])
    (out / "captured-heap.bin").write_bytes(payload)
    cases.append(("captured", allocator, ["-DREPLAY_CAPTURED_HEAP"]))
for name, body, flags in cases:
    fixture = out / f"{name}.c"
    fixture.write_text(prefix + metadata + body + checks)
    executable = out / f"{name}.exe"
    subprocess.run([str(compiler), "-std=c11", "-O2", *flags, str(fixture), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
