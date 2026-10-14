from test_support import msbuild
"""Verify the production vertex interpreter's cache safety classification."""
from pathlib import Path
import subprocess
here = Path(__file__).resolve().parent
runtime = here.parent / 'build/runtime/xboxrecomp/src/kernel/nv2a_vsh_interp.c'
code = f'#include "{runtime.as_posix()}"\n#include "{(here / "vsh_operand_reference.inc").as_posix()}"\n' + r'''
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Failed %d\n",__LINE__);return 1;}}while(0)
static uint32_t seed=123456789;
static uint32_t next_word(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(void) {
 for(unsigned trial=0;trial<10000;trial++){
  float inputs[NV2A_VSH_INPUTS][4],initial[NV2A_VSH_CONSTANTS][4],expected_constants[NV2A_VSH_CONSTANTS][4];
  for(unsigned i=0;i<NV2A_VSH_INPUTS;i++)for(unsigned ch=0;ch<4;ch++)inputs[i][ch]=(next_word()%31+1)*.125f;
  for(unsigned i=0;i<NV2A_VSH_CONSTANTS;i++)for(unsigned ch=0;ch<4;ch++)initial[i][ch]=(next_word()%31+1)*.125f;
  memcpy(s_const,initial,sizeof initial);nv2a_vsh_set_start_slot(0);nv2a_vsh_set_cxt_write(trial&1);
  for(unsigned slot=0;slot<4;slot++){
   uint32_t words[4]={0,next_word(),next_word(),next_word()};words[3]=(words[3]&~1u)|(slot==3);
   if(trial&1){nv2a_vsh_set_load_slot(slot);for(unsigned word=0;word<4;word++)nv2a_vsh_program_word(words[word]);}
   else nv2a_vsh_set_instruction(slot,words);
  }
  Nv2aVshOutput expected={0},actual={0};int a=nv2a_vsh_run_reference(inputs,&expected);
  memcpy(expected_constants,s_const,sizeof s_const);memcpy(s_const,initial,sizeof initial);
  int b=nv2a_vsh_run(inputs,&actual);
  CHECK(a==b && !memcmp(&expected,&actual,sizeof actual));
  CHECK(!memcmp(expected_constants,s_const,sizeof s_const));
 }

 uint32_t writing[4]={0,(1u<<21)|0x1B,2u<<26,1u|(15u<<12)};
 uint32_t pure[4]={0,0,0,1u|(1u<<11)};
 nv2a_vsh_set_start_slot(0);nv2a_vsh_set_instruction(0,writing);
 nv2a_vsh_set_cxt_write(0);CHECK(nv2a_vsh_cacheable());
 nv2a_vsh_set_cxt_write(1);CHECK(!nv2a_vsh_cacheable());
 float inputs[NV2A_VSH_INPUTS][4]={{2,3,4,5}};Nv2aVshOutput out;
 CHECK(nv2a_vsh_run(inputs,&out));CHECK(nv2a_vsh_constant(0)[0]==2);
 nv2a_vsh_set_instruction(0,pure);CHECK(nv2a_vsh_cacheable());
 pure[3]&=~1u;nv2a_vsh_set_instruction(0,pure);
 nv2a_vsh_set_instruction(1,writing);CHECK(!nv2a_vsh_cacheable());
 pure[3]|=1;nv2a_vsh_set_instruction(2,pure);
 nv2a_vsh_set_start_slot(2);CHECK(nv2a_vsh_cacheable());
 nv2a_vsh_set_instruction(2,writing);CHECK(!nv2a_vsh_cacheable());
 nv2a_vsh_set_start_slot(NV2A_VSH_SLOTS-1);
 uint32_t empty[4]={0};nv2a_vsh_set_instruction(NV2A_VSH_SLOTS-1,empty);
 CHECK(!nv2a_vsh_cacheable());
 puts("Passed: actual constant writes bypass cache; pure shaders, start-slot changes and program replacement classified correctly.");return 0;
}
'''
out = here / 'irq-test-build'
(out / 'vsh_cacheability.c').write_text(code)
project = (out / 'apu_irq_harness.vcxproj').read_text().replace('apu_irq_harness.c', 'vsh_cacheability.c').replace('obj\\apu_irq\\', 'obj\\vsh_cacheability\\')
(out / 'vsh_cacheability.vcxproj').write_text(project)
subprocess.run([msbuild, str(out / 'vsh_cacheability.vcxproj'), '/p:Configuration=Release', '/p:Platform=x64', '/verbosity:minimal'], check=True)
subprocess.run([str(out / 'bin/vsh_cacheability.exe')], check=True)
