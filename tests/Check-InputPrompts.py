from test_support import msbuild
"""Check production last-device detection and rich-text prompt conversion."""
from pathlib import Path
import subprocess
here=Path(__file__).resolve().parent
out=here/'irq-test-build'
input_source=here.parent/'build/runtime/xboxrecomp/src/input/xinput_device.c'
prompt_header=here.parent/'src/api/prompt_text.h'
prefix=r'''
#include <windows.h>
#include <xinput.h>
#include <stdio.h>
static unsigned serial;
static unsigned char keys[256];
static XINPUT_STATE physical;
static int connected=1,captured;
static DWORD fake_get_state(DWORD p,XINPUT_STATE *s){(void)p;*s=physical;return connected?ERROR_SUCCESS:ERROR_DEVICE_NOT_CONNECTED;}
#define XInputGetState fake_get_state
unsigned xbox_FramebufferKeyboardSerial(void){return serial;}
int xbox_FramebufferKeyDown(int k){return keys[k];}
int xbox_DebugOverlayCaptured(void){return captured;}
'''
test=r'''
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 XBOX_INPUT_STATE s;CHECK(!xbox_InputUsesKeyboard());
 serial++;keys['L']=1;xbox_InputGetState(0,&s);CHECK(xbox_InputUsesKeyboard()&&s.Gamepad.bAnalogButtons[XBOX_BUTTON_B]==255);
 keys['L']=0;physical.Gamepad.sThumbLX=1000;xbox_InputGetState(0,&s);CHECK(xbox_InputUsesKeyboard());
 physical.Gamepad.wButtons=XINPUT_GAMEPAD_A;xbox_InputGetState(0,&s);CHECK(!xbox_InputUsesKeyboard());
 serial++;xbox_InputGetState(0,&s);CHECK(xbox_InputUsesKeyboard()); /* held controller does not override new key */
 physical.Gamepad.wButtons=0;physical.Gamepad.sThumbLX=0;xbox_InputGetState(0,&s);CHECK(xbox_InputUsesKeyboard());
 physical.Gamepad.sThumbLX=16000;xbox_InputGetState(0,&s);CHECK(!xbox_InputUsesKeyboard());
 serial++;xbox_InputGetState(0,&s);CHECK(xbox_InputUsesKeyboard());
 physical.Gamepad.sThumbLX=16500;xbox_InputGetState(0,&s);CHECK(xbox_InputUsesKeyboard());
 physical.Gamepad.bRightTrigger=80;xbox_InputGetState(0,&s);CHECK(!xbox_InputUsesKeyboard());
 serial++;xbox_InputPulseStart();xbox_InputGetState(0,&s);CHECK(xbox_InputUsesKeyboard()); /* synthesized Start */
 captured=1;physical.Gamepad.wButtons=XINPUT_GAMEPAD_B;xbox_InputGetState(0,&s);CHECK(!xbox_InputUsesKeyboard());
 serial++;CHECK(xbox_InputUsesKeyboard());captured=0;connected=0;xbox_InputGetState(0,&s);CHECK(xbox_InputUsesKeyboard());
 char text[4096];uint16_t offsets[4096];
 const char *src="PRESS $A112024016~$C TO USE WARP GATE";
 size_t n=prompt_text_convert(src,strlen(src),text,sizeof text,offsets);
 CHECK(n&&strcmp(text,"PRESS [L] TO USE WARP GATE")==0);
 CHECK(offsets[6]==6&&offsets[9]==20&&offsets[n]==strlen(src));
 src="$A064064128#$C $A096032096{$C $A000096032}$C $A064064128|$C";
 CHECK(prompt_text_convert(src,strlen(src),text,sizeof text,offsets));CHECK(!strcmp(text,"[SPACE] [J] [L] [ESC]"));
 src="$A000096032}$C Back";
 CHECK(prompt_text_convert(src,strlen(src),text,sizeof text,offsets));CHECK(!strcmp(text,"[L] Back"));
 src="$A255000000Warning$C ordinary # ~ {} and $VLevel$";
 CHECK(!prompt_text_convert(src,strlen(src),text,sizeof text,offsets));CHECK(!strcmp(text,src));
 src="$A112024016~$C";CHECK(!prompt_text_convert(src,strlen(src),text,3,offsets));
 puts("Passed: last-input switching, idle/drift/release/held-pad filtering, synthetic input, overlay input, labels and continuation offsets.");return 0;
}
'''
(out/'input_prompts.c').write_text(prefix+f'\n#include "{input_source.as_posix()}"\n#include "{prompt_header.as_posix()}"\n'+test)
project=(out/'apu_irq_harness.vcxproj').read_text().replace('apu_irq_harness.c','input_prompts.c').replace('obj\\apu_irq\\','obj\\input_prompts\\')
project=project.replace('<ClCompile>',f'<ClCompile><AdditionalIncludeDirectories>{here.parent/"build/runtime/xboxrecomp/include"};{here.parent/"build/runtime/xboxrecomp/src"};%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>')
(out/'input_prompts.vcxproj').write_text(project)
subprocess.run([msbuild,str(out/'input_prompts.vcxproj'),'/p:Configuration=Release','/p:Platform=x64','/verbosity:minimal'],check=True)
subprocess.run([str(out/'bin/input_prompts.exe')],check=True)

# Exercise the complete production draw wrapper with a fake guest text renderer.
wrapper=(here.parent/'src/title_prompts.c').read_text()
assert 'void sub_0008CE00(void)' in wrapper
assert 'void sub_0008CE00(void)' not in (here.parent/'src/recomp/gen/recomp_0007.c').read_text()
wrapper=wrapper[wrapper.index('static uint32_t s_prompt_buffers'):wrapper.index('/* Native text renderer retained')]
text_adapter=(here.parent/'src/api/title_guest_text.c').read_text()
text_adapter=text_adapter[text_adapter.index('extern uint32_t xbox_HeapAlloc'):text_adapter.index('/* Native text renderer retained')]
wrapper=text_adapter+wrapper
draw_prefix=r'''
#include <stdint.h>
#include <string.h>
#include <stdio.h>
static unsigned char memory[0x100000];
#define MEM32(p) (*(uint32_t*)(memory+(p)))
#define XBOX_PTR(p) (memory+(p))
static uint32_t esp=0x10000,eax;
static int keyboard,errors;
static const char *expected;
static unsigned continuation;
static int xbox_InputUsesKeyboard(void){return keyboard;}
static int dynamic_prompts=1;
static int xbox_DisplayDynamicPrompts(void){return dynamic_prompts;}
uint32_t xbox_HeapAlloc(uint32_t size,uint32_t alignment){static uint32_t next=0x80000;uint32_t p=next;next+=size;(void)alignment;return p;}
void title_prompt_draw_original(void){
 uint32_t box=MEM32(esp+4),p=MEM32(box+0x44);
 if(strcmp((const char*)XBOX_PTR(p),expected))errors++;
 eax=p+continuation;esp+=4;
}
'''
draw_test=r'''
int main(void){
 uint32_t box=0x2000,source=0x4000;const char *original="PRESS $A112024016~$C TO TALK";
 strcpy((char*)XBOX_PTR(source),original);MEM32(box+0x44)=source;
 MEM32(esp+4)=box;keyboard=1;expected="PRESS [L] TO TALK";continuation=9;
 sub_0008CE00();
 if(errors||esp!=0x10004||MEM32(box+0x44)!=source||eax!=source+20||strcmp((char*)XBOX_PTR(source),original))return 1;
 esp=0x10000;keyboard=0;expected=original;continuation=6;sub_0008CE00();
 if(errors||eax!=source+6||MEM32(box+0x44)!=source)return 2;
 esp=0x10000;keyboard=1;expected="PRESS [L] TO TALK";continuation=strlen(expected);sub_0008CE00();
 if(errors||eax!=source+strlen(original))return 3;
 esp=0x10000;dynamic_prompts=0;expected=original;continuation=6;sub_0008CE00();
 if(errors||esp!=0x10004||eax!=source+6||MEM32(box+0x44)!=source)return 4;
 puts("Passed: complete draw wrapper preserves guest text/stack and maps dialogue continuation pointers in both input modes.");return 0;
}
'''
(out/'prompt_draw.c').write_text(draw_prefix+f'\n#include "{prompt_header.as_posix()}"\n'+wrapper+draw_test)
(out/'prompt_draw.vcxproj').write_text(project.replace('input_prompts.c','prompt_draw.c').replace('obj\\input_prompts\\','obj\\prompt_draw\\'))
subprocess.run([msbuild,str(out/'prompt_draw.vcxproj'),'/p:Configuration=Release','/p:Platform=x64','/verbosity:minimal'],check=True)
subprocess.run([str(out/'bin/prompt_draw.exe')],check=True)
