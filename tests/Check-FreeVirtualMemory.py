from test_support import msbuild
from pathlib import Path
import subprocess
here=Path(__file__).resolve().parent
s=(here.parent/'build/runtime/xboxrecomp/src/kernel/kernel_bridge.c').read_text(encoding='utf-8')
a=s.index('static void bridge_NtFreeVirtualMemory(void)');b=s.index('/*',s.index('\n}\n',a)+3)
code=r"""
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[65536];static uint32_t args[3],g_eax;
#define STACK_ARG(i) args[i]
#define BRIDGE_MEM32(a) (*(uint32_t *)(ram+(a)))
#define XBOX_TO_NATIVE(a) (ram+(a))
static int guest_vmem_free(uint32_t *b,uint32_t *s,uint32_t t,uint32_t *status){return 0;}
static uint32_t xbox_HeapBlockSize(uint32_t a){return a>=0x2000 && a<0x6000 ? 0x6000-a : 0;}
static int reclaim;static uint32_t freed;
static int xbox_HeapReclaimEnabled(void){return reclaim;}
static void xbox_HeapFree(uint32_t a){freed=a;}
"""+s[a:b]+r"""
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Failed %d\n",__LINE__);return 1;}}while(0)
int main(void) {
 memset(ram,0xCC,sizeof(ram));args[0]=0x100;args[1]=0x108;args[2]=0x4000;
 BRIDGE_MEM32(0x100)=0x3001;BRIDGE_MEM32(0x108)=0x1000;
 bridge_NtFreeVirtualMemory();CHECK(g_eax==0);CHECK(BRIDGE_MEM32(0x100)==0x3000);CHECK(BRIDGE_MEM32(0x108)==0x2000);
 CHECK(BRIDGE_MEM32(0x104)==0xCCCCCCCC);CHECK(BRIDGE_MEM32(0x10C)==0xCCCCCCCC);
 CHECK(ram[0x2FFF]==0xCC && ram[0x3000]==0 && ram[0x4FFF]==0 && ram[0x5000]==0xCC);
 BRIDGE_MEM32(0x100)=0x5000;BRIDGE_MEM32(0x108)=0x2000;bridge_NtFreeVirtualMemory();CHECK(g_eax==0xC000000D);CHECK(ram[0x5000]==0xCC);
 BRIDGE_MEM32(0x100)=0x2000;BRIDGE_MEM32(0x108)=0;args[2]=0x8000;bridge_NtFreeVirtualMemory();CHECK(g_eax==0);CHECK(BRIDGE_MEM32(0x100)==0);CHECK(BRIDGE_MEM32(0x104)==0xCCCCCCCC);
 CHECK(freed==0);reclaim=1;BRIDGE_MEM32(0x100)=0x2000;bridge_NtFreeVirtualMemory();CHECK(g_eax==0 && freed==0x2000);
 args[0]=0;bridge_NtFreeVirtualMemory();CHECK(g_eax==0xC000000D);
 puts("Passed: guest 32-bit slots preserved, decommit rounded and bounded, release and null arguments handled.");return 0;
}
"""
out=here/'irq-test-build';(out/'free_virtual_memory.c').write_text(code)
project=(out/'apu_irq_harness.vcxproj').read_text().replace('apu_irq_harness.c','free_virtual_memory.c').replace('obj\\apu_irq\\','obj\\free_virtual_memory\\')
(out/'free_virtual_memory.vcxproj').write_text(project)
subprocess.run([msbuild,str(out/'free_virtual_memory.vcxproj'),'/p:Configuration=Release','/p:Platform=x64','/verbosity:minimal'],check=True)
subprocess.run([str(out/'bin/free_virtual_memory.exe')],check=True)
