"""Exercise maintained movie observers and actual window snapshot implementation."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/movie-frame-test"
(out / "gen").mkdir(parents=True, exist_ok=True)
(out / "gen/recomp_types.h").write_text(r'''
#pragma once
#include <stdint.h>
extern uint8_t memory[];
extern uint32_t esp, eax, ecx, edx;
#define XBOX_PTR(a) ((uintptr_t)(memory + ((uint32_t)(a) & 0x03FFFFFF)))
#define MEM32(a) (*(uint32_t *)XBOX_PTR(a))
''')
video = (root / "runtime_overrides/src/video/fb_present.c").read_text()
snapshot = video[video.index("typedef struct MovieImage"):video.index("/* 0: not started")]
fixture = r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
uint8_t memory[0x04000000];
uint32_t esp, eax, ecx, edx;
''' + snapshot + f'\n#include "{(root / "src/api/title_guest_movie_frame.c").as_posix()}"\n' + r'''
static unsigned step_calls, lock_calls;
static uint32_t result_status = 1, return_address = 0x0015415F;
void title_translated_001000B0(void)
{
    lock_calls++;
    MEM32(MEM32(esp + 8)) = 8;
    MEM32(MEM32(esp + 8) + 4) = 0xF0100000;
    esp += 20;
    eax = 111; ecx = 222; edx = 333;
}
void title_translated_00152FEC(void)
{
    step_calls++;
    uint32_t entry = esp;
    esp -= 32;
    MEM32(esp) = return_address;
    MEM32(esp + 8) = 0x20000;
    title_guest_movie_lock_surface();
    assert(esp == entry - 12);
    assert(eax == 111 && ecx == 222 && edx == 333);
    esp = entry;
    memset(memory + 0x100000, 0x5A, 16);
    MEM32(MEM32(esp + 12)) = result_status;
    esp += 20;
    eax = 444; ecx = 555; edx = 666;
}
static void step(void)
{
    esp = 0x30000;
    MEM32(esp + 4) = 0x40000;
    MEM32(esp + 12) = 0x50000;
    MEM32(0x40040) = 4; MEM32(0x40044) = 2;
    title_guest_movie_step();
    assert(esp == 0x30014);
    assert(eax == 444 && ecx == 555 && edx == 666);
}
int main(void)
{
    step();
    assert(step_calls == 1 && lock_calls == 1 && s_movie_sequence == 1);
    memset(memory + 0x100000, 0xFF, 16);
    const uint8_t *pixels = movie_image_acquire(0x100000, 8, 4, 2);
    assert(pixels && pixels[0] == 0x5A && pixels[15] == 0x5A);
    result_status = 3; step();
    assert(s_movie_sequence == 1);
    result_status = 1; return_address = 0xBAD; step();
    assert(s_movie_sequence == 1);
    assert(!movie_image_acquire(0x200000, 8, 4, 2));
    assert(!movie_image_acquire(0x100000, 16, 4, 2));
    AcquireSRWLockExclusive(&s_movie_lock);
    pixels = movie_image_acquire(0x100000, 8, 4, 2);
    assert(pixels && pixels[0] == 0x5A);
    ReleaseSRWLockExclusive(&s_movie_lock);
    xbox_MovieFramePublish(memory, 0, 1, 4, 2);
    assert(s_movie_sequence == 1);
    puts("Passed: native delegation, stack/register outputs, status filtering, owned pixels and contention.");
}
'''
(out / "fixture.c").write_text(fixture)
compiler = root / "build/portable/tools/llvm/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe"
subprocess.run([str(compiler), "-std=c11", "-I" + str(out),
                "-I" + str(root / "runtime_overrides/include"),
                str(out / "fixture.c"), "-o", str(out / "fixture.exe")], check=True)
subprocess.run([str(out / "fixture.exe")], check=True)
