from test_support import msbuild
"""Exercise production 30 FPS limiter using real Windows timing."""
from pathlib import Path
import subprocess
here=Path(__file__).resolve().parent
s=(here.parent/'build/runtime/xboxrecomp/src/video/fb_present.c').read_text()
a=s.index('static void display_limit_frame(');b=s.index('\n}',a)+3
code='#include <windows.h>\n#include <stdio.h>\nstatic int enabled;\nstatic int xbox_DisplayFpsCap(void){return enabled;}\n'+s[a:b]+r'''
#define CHECK(x) do{if(!(x)){printf("FAIL %d\n",__LINE__);return 1;}}while(0)
int main(void){
 LARGE_INTEGER frequency,now,previous,start;QueryPerformanceFrequency(&frequency);
 QueryPerformanceCounter(&start);for(unsigned i=0;i<1000;i++)display_limit_frame();QueryPerformanceCounter(&now);
 CHECK((now.QuadPart-start.QuadPart)*1000.0/frequency.QuadPart<100);
 enabled=1;display_limit_frame();QueryPerformanceCounter(&previous);start=previous;
 for(unsigned i=0;i<30;i++){
  display_limit_frame();QueryPerformanceCounter(&now);
  CHECK((now.QuadPart-previous.QuadPart)*1000.0/frequency.QuadPart>=33.2);
  previous=now;
 }
 double elapsed=(now.QuadPart-start.QuadPart)*1000.0/frequency.QuadPart;CHECK(elapsed>=995 && elapsed<1500);
 Sleep(80);display_limit_frame();QueryPerformanceCounter(&previous);display_limit_frame();QueryPerformanceCounter(&now);
 CHECK((now.QuadPart-previous.QuadPart)*1000.0/frequency.QuadPart>=33.2);
 enabled=0;display_limit_frame();enabled=1;QueryPerformanceCounter(&start);display_limit_frame();QueryPerformanceCounter(&now);
 CHECK((now.QuadPart-start.QuadPart)*1000.0/frequency.QuadPart<10);
 printf("Passed: disabled mode, 30 FPS intervals, slow frames and toggle reset (30 intervals %.2f ms).\n",elapsed);return 0;
}
'''
out=here/'irq-test-build';(out/'fps_cap.c').write_text(code)
project=(out/'apu_irq_harness.vcxproj').read_text().replace('apu_irq_harness.c','fps_cap.c').replace('obj\\apu_irq\\','obj\\fps_cap\\')
(out/'fps_cap.vcxproj').write_text(project)
subprocess.run([msbuild,str(out/'fps_cap.vcxproj'),'/p:Configuration=Release','/p:Platform=x64','/verbosity:minimal'],check=True)
subprocess.run([str(out/'bin/fps_cap.exe')],check=True)
