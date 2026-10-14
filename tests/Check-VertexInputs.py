from test_support import msbuild
"""Compare optimized vertex inputs with the original per-attribute fetch."""
from pathlib import Path
import subprocess
here=Path(__file__).resolve().parent
source=(here.parent/'build/runtime/xboxrecomp/src/kernel/nv2a_pb_exec.c').read_text()
fetch=source[source.index('static int fetch_attr('):source.index('static uint32_t surface_bpp(void);')]
inputs=source[source.index('static void fetch_vertex_inputs('):source.index('static int transform_vertex(')]
code=r'''#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NV_VERTEX_ATTRS 16
#define NV2A_VSH_INPUTS 16
typedef struct {uint32_t offset,type,size,stride;} VertexAttr;
static struct {VertexAttr attr[16];float imm_attr[16][4];unsigned inline_active,inline_count;uint32_t inline_buf[16384];} s_gpu;
static unsigned char memory[65536];
static void *xbox_GetMemoryOffset(void){return memory;}
'''+fetch+inputs+r'''
int main(void){
 for(unsigned i=0;i<sizeof(memory);i++)memory[i]=(unsigned char)(i*73);
 memcpy(s_gpu.inline_buf,memory,sizeof(memory));
 for(unsigned trial=0;trial<10000;trial++){
  s_gpu.inline_active=trial&1;s_gpu.inline_count=trial%16384;
  for(unsigned a=0;a<16;a++){
   for(unsigned ch=0;ch<4;ch++)s_gpu.imm_attr[a][ch]=(float)(trial+a*4+ch)*.125f;
   s_gpu.attr[a].type=(trial+a)%8;s_gpu.attr[a].size=(trial+a)%5;
   s_gpu.attr[a].stride=(trial+a)%3?32:0;s_gpu.attr[a].offset=(trial+a)%7?1024+a*256:0;
  }
  float expected[16][4],actual[16][4];
  for(unsigned a=0;a<16;a++)fetch_attr(&s_gpu.attr[a],trial%4,expected[a]);
  fetch_vertex_inputs(trial%4,actual);
  if(memcmp(expected,actual,sizeof(actual))){printf("FAIL %u\n",trial);return 1;}
 }
 puts("Passed: 10000 vertex input comparisons, latched attributes, formats, disabled arrays and inline bounds.");return 0;
}
'''
out=here/'irq-test-build';(out/'vertex_inputs.c').write_text(code)
project=(out/'apu_irq_harness.vcxproj').read_text().replace('apu_irq_harness.c','vertex_inputs.c').replace('obj\\apu_irq\\','obj\\vertex_inputs\\')
(out/'vertex_inputs.vcxproj').write_text(project)
subprocess.run([msbuild,str(out/'vertex_inputs.vcxproj'),'/p:Configuration=Release','/p:Platform=x64','/verbosity:minimal'],check=True)
subprocess.run([str(out/'bin/vertex_inputs.exe')],check=True)
