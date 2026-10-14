from test_support import msbuild
"""Exercise production TLS allocation and the SDK's initialization accesses."""
from pathlib import Path
import subprocess

analysis = Path(__file__).resolve().parent
source = (Path(__file__).resolve().parents[1] / 'build/runtime/xboxrecomp/src/kernel/xbox_memory_layout.c').read_text()
start = source.index('uint32_t xbox_AllocThreadTib(void)')
end = source.index('\nuint32_t xbox_AllocThreadStack(void)', start)
prefix = r'''
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#define XBOX_TIB_MAIN 0x4000
static uint32_t memory[0x20000/4], next=0x10000;
static uintptr_t g_memory_offset;
static uint32_t g_tls_template_va=0x8000, g_tls_total=148;
static uint32_t xbox_HeapAlloc(uint32_t size, uint32_t align) {
    uint32_t out=(next+align-1)&~(align-1); next=out+size; return out;
}
#define WORD(va) memory[(va)/4]
#define CHECK(x) do { if (!(x)) { printf("FAIL %d\n",__LINE__); return 1; } } while(0)
'''
test = r'''
int main(void) {
    uint32_t a,b,ab,bb,at,bt,i;
    g_memory_offset=(uintptr_t)memory;
    WORD(XBOX_TIB_MAIN+0x20)=0x7000;
    WORD(XBOX_TIB_MAIN+0x28)=0x7600;
    for (i=4;i<148;i+=4) WORD(0x8000+i)=i;
    a=xbox_AllocThreadTib(); b=xbox_AllocThreadTib();
    at=WORD(a+0x28); bt=WORD(b+0x28);
    ab=WORD(at+0x28); bb=WORD(bt+0x28);
    CHECK(a!=b && at!=bt && ab!=bb);
    CHECK(WORD(a+0x18)==a && WORD(b+0x18)==b);
    CHECK(WORD(a+0x20)==0x7000 && WORD(b+0x20)==0x7000);
    CHECK(WORD(a)==0xFFFFFFFF && WORD(b)==0xFFFFFFFF);
    CHECK(WORD(a+4)-37*4==ab && WORD(b+4)-37*4==bb);
    CHECK(WORD(ab)==ab+4 && WORD(bb)==bb+4);
    for(i=4;i<148;i+=4) CHECK(WORD(ab+i)==i && WORD(bb+i)==i);
    /* XAPI's self-pointer plus complete 144-byte zero-fill. */
    WORD(ab)=ab+4; memset((uint8_t *)memory+ab+4,0xA5,144);
    CHECK(WORD(a+0x20)==0x7000 && WORD(b+0x20)==0x7000);
    CHECK(WORD(bb+144)==144 && WORD(bb)==bb+4);
    WORD(bb)=bb+4; memset((uint8_t *)memory+bb+4,0x5A,144);
    CHECK(WORD(ab+144)==0xA5A5A5A5 && WORD(a+0x20)==0x7000);
    puts("Passed: per-thread TLS, SDK initialization, negative index, self pointers, payload bounds, and isolation.");
    return 0;
}
'''
output = analysis / 'irq-test-build'
(output / 'thread_tls.c').write_text(prefix + source[start:end] + test)
project = (output / 'apu_irq_harness.vcxproj').read_text().replace(
    'apu_irq_harness.c', 'thread_tls.c'
).replace('obj\\apu_irq\\</IntDir>', 'obj\\thread_tls\\</IntDir>')
(output / 'thread_tls.vcxproj').write_text(project)
subprocess.run([msbuild,
                str(output / 'thread_tls.vcxproj'), '/p:Configuration=Release',
                '/p:Platform=x64', '/verbosity:minimal'], check=True)
subprocess.run([str(output / 'bin/thread_tls.exe')], check=True)
