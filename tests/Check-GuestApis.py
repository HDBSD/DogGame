"""Check production system adapters with simulated guest memory and native calls."""
from pathlib import Path
import os
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/guest-api-test'
(out / 'gen').mkdir(parents=True, exist_ok=True)
modules = ['title_guest_calls', 'title_guest_state', 'title_guest_player', 'title_guest_geometry', 'title_guest_ui', 'title_guest_menu', 'title_guest_symbols']
functions = sorted(set(re.findall(r'\bsub_[0-9A-F]+', '\n'.join((root/'src/api'/(name+'.c')).read_text() for name in modules))))
(out/'gen/recomp_funcs.h').write_text('\n'.join('void '+name+'(void);' for name in functions))
(out/'gen/recomp_types.h').write_text(r'''
#ifndef RECOMP_TYPES_H
#define RECOMP_TYPES_H
#include <stdint.h>
#include <stddef.h>
typedef union {float f[4];uint32_t u[4];} RecompXmm;
extern unsigned char memory[];
extern uint32_t eax,ecx,edx,esp,ebx,esi,edi,g_ebp,g_seh_ebp;
extern RecompXmm xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;
extern double g_fp_stack[8];
extern int g_fp_top,g_fp_cmp,g_df;
extern uint16_t g_fp_control_word,g_fp_cc;
#define MEM32(p) (*(uint32_t *)(memory+(p)))
#define MEM16(p) (*(uint16_t *)(memory+(p)))
#define MEM8(p) memory[(p)]
#define MEMF(p) (*(float *)(memory+(p)))
#define XBOX_PTR(p) (memory+(p))
#define PUSH32(s,v) do{(s)-=4;MEM32(s)=(v);}while(0)
#endif
''')
test = r'''
#include <stdio.h>
#include <string.h>
#include "title_guest_calls.h"
#include "title_guest_player.h"
#include "title_guest_state.h"
#include "title_guest_geometry.h"
#include "title_guest_ui.h"
#include "title_guest_routines.h"
unsigned char memory[0x4000000];
uint32_t eax,ecx,edx,esp,ebx,esi,edi,g_ebp,g_seh_ebp;
RecompXmm xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;
double g_fp_stack[8];int g_fp_top,g_fp_cmp,g_df;uint16_t g_fp_control_word,g_fp_cc;
static uint32_t calls[64], arguments[64];static unsigned call_count;
static void record_call(uint32_t address) {
    calls[call_count]=address;arguments[call_count++]=MEM32(esp+4);
    eax=0x1234;esp+=4;
}
STUBS
#define CHECK(c) do{if(!(c)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void) {
    CHECK(title_guest_routine_address("title_guest_update_camera_projection")==0xA0260);
    CHECK(title_guest_routine_address("title_guest_apply_player_damage")==0x1D970);
    CHECK(!title_guest_routine_address("unknown")&&!title_guest_routine_address(NULL));
    esp=0x10000;eax=1;ecx=2;edx=3;ebx=4;esi=5;edi=6;g_ebp=7;g_seh_ebp=8;
    xmm0.u[0]=0xDEADBEEF;xmm7.u[3]=0x12345678;g_fp_stack[3]=1.25;
    g_fp_top=3;g_fp_cmp=2;g_df=1;g_fp_control_word=0x27F;g_fp_cc=0x100;
    GuestRegisters saved=save_guest_registers();
    eax=ecx=edx=esp=ebx=esi=edi=g_ebp=g_seh_ebp=0;
    xmm0.u[0]=xmm7.u[3]=0;g_fp_stack[3]=0;g_fp_top=g_fp_cmp=g_df=0;g_fp_control_word=g_fp_cc=0;
    restore_guest_registers(&saved);
    CHECK(eax==1&&ecx==2&&edx==3&&esp==0x10000&&ebx==4&&esi==5&&edi==6&&g_ebp==7&&g_seh_ebp==8);
    CHECK(xmm0.u[0]==0xDEADBEEF&&xmm7.u[3]==0x12345678&&g_fp_stack[3]==1.25&&g_fp_top==3&&g_fp_cmp==2&&g_df==1&&g_fp_control_word==0x27F&&g_fp_cc==0x100);
    CHECK(call_guest_no_arguments(sub_00075090)==0x1234&&esp==0x10000);
    CHECK(call_guest_one_argument(sub_0005F3A0,11)==0x1234&&esp==0x10000&&arguments[call_count-1]==11);
    title_state_set_player_powers(0x12345678);call_count=0;
    title_guest_warp_to_scene(0x48303031);
    const uint32_t expected[]={0x5F3A0,0x5CAF0,0x5D2A0,0x5D330,0xADFA0,0x5F2F0};
    CHECK(call_count==6&&!memcmp(calls,expected,sizeof expected)&&esp==0x10000);
    CHECK(arguments[0]==11&&arguments[2]==0x48303031&&arguments[5]==1);
    CHECK(title_state_player_powers()==0x12345678&&MEM32(0x1B8B68)==84&&MEM32(0x1B8B6C)==2);
    CHECK(title_guest_damage_is_from_monster(0,0x2C19A));
    CHECK(!title_guest_damage_is_from_monster(0,0x28E80));
    MEM8(0x20004)=2;CHECK(title_guest_damage_is_from_monster(0x20000,0x28E80));
    MEM8(0x20004)=0x19;CHECK(title_guest_damage_is_from_monster(0x20000,0x28E80));
    MEM8(0x20004)=3;CHECK(!title_guest_damage_is_from_monster(0x20000,0x28E80));
    uint32_t player=0x1BEF20,frame=0x30000,collision=0x40000;
    MEM32(player+0x40)=frame;MEM32(player+0x44)=collision;MEM8(collision+0xB)=7;
    title_state_set_player_frame(frame);eax=99;call_count=0;
    for(unsigned i=0;i<3;i++){MEMF(frame+0x30+i*4)=10;MEMF(frame+0xE0+i*4)=5;MEMF(frame+0x8C+i*4)=6;}
    title_guest_apply_player_flight(player,0x50000,frame,0x3DCCCCCD,1,2,3);
    CHECK(call_count==2&&calls[0]==0x860C0&&calls[1]==0x861A0&&esp==0x10000&&eax==99);
    CHECK(MEMF(frame+0x30)==11&&MEMF(frame+0x34)==12&&MEMF(frame+0x38)==13&&!MEM8(collision+0xB));
    for(unsigned i=0;i<3;i++)CHECK(MEMF(frame+0xE0+i*4)==0&&MEMF(frame+0x8C+i*4)==0);
    title_guest_clear_player_motion();for(unsigned i=0;i<3;i++)CHECK(MEMF(frame+0xC8+i*4)==0);

    uint32_t scene=0x60000,cam=0x61000,camframe=0x62000,env=0x63000,geom=0x64000,world=0x65000,leaf=0x66000,vertices=0x67000,indices=0x68000;
    title_state_set_menu_mode(11);title_state_set_current_scene(scene);title_state_set_active_camera(cam);
    MEM32(cam+4)=camframe;MEMF(cam+0x68)=MEMF(cam+0x6C)=1;
    MEMF(camframe+0x50)=1;MEMF(camframe+0x64)=1;MEMF(camframe+0x78)=1;
    MEM32(scene+0x40)=env;MEM32(env)=geom;MEM32(geom+4)=world;MEM32(world+0x1C)=leaf;
    MEM32(leaf)=0xFFFFFFFF;MEM32(leaf+8)=vertices;MEM32(leaf+4)=indices;MEM16(leaf+0x8A)=3;MEM16(leaf+0x8C)=1;
    MEM16(indices+2)=0;MEM16(indices+4)=1;MEM16(indices+6)=2;
    MEMF(vertices+12)=1;MEMF(vertices+24+4)=1;
    title_guest_geometry_capture(0,1);CHECK(title_geometry_snapshot.camera_frame_valid&&title_geometry_snapshot.triangle_count==1);
    MEMF(vertices+12)=99;CHECK(title_geometry_snapshot.triangles[0].p[1].x==1);
    title_guest_geometry_invalidate();title_guest_geometry_capture(0,1);CHECK(title_geometry_snapshot.triangles[0].p[1].x==99);
    title_state_set_menu_mode(7);title_guest_geometry_capture(0,1);CHECK(!title_geometry_snapshot.camera_frame_valid);

    uint32_t item=0x70000,asset=0x71000,link=0x72000;
    MEM32(scene+0x100)=1;MEM32(scene+0x1AC)=item;
    MEM32(item)=0x12345678;MEM8(item+0x14)=1;MEM32(item+0xF0)=8;MEM32(item+0xF8)=asset;MEM8(item+5)=1;MEM32(item+8)=link;
    MEM16(link)=63;MEM16(asset+0x54)=100;MEM16(asset+0x56)=40;MEM16(asset+0x96)=14;
    MenuHit hits[256];CHECK(title_guest_ui_collect_hits(scene,2,hits,0,0)==1&&hits[0].item==item&&hits[0].h==20);
    MEM8(item+0x14)=0;CHECK(title_guest_ui_collect_hits(scene,2,hits,0,0)==0);
    title_guest_ui_write_link(link,73,79,0xABCD);CHECK(MEM16(link)==73&&MEM16(link+2)==79&&MEM32(link+4)==0xABCD&&MEM32(link+28)==0);
    puts("Passed: guest register/stack contracts, portal order, powers and clock, damage classification, player motion, geometry ownership and UI hits.");return 0;
}
'''
stubs = '\n'.join('void '+name+'(void){record_call(0x'+name[4:]+');'+ ('eax=100;edx=2;' if name=='sub_000ADFA0' else '')+'}' for name in functions)
test=test.replace('STUBS',stubs).replace('#include "title_guest_calls.h"','#include "gen/recomp_funcs.h"\n#include "title_guest_calls.h"')
(out/'guest_apis.c').write_text(test)
vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
installation=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
if not installation: raise RuntimeError('Visual Studio C++ build tools were not found.')
sources=' '.join('"'+str(root/'src/api'/(name+'.c'))+'"' for name in modules)
commands=['@echo off',f'call "{Path(installation)/"VC/Auxiliary/Build/vcvars64.bat"}" >nul','if errorlevel 1 exit /b 1',
          f'cl /nologo /std:c11 /O2 /I"{out}" /I"{root/"src"}" /I"{root/"src/api"}" guest_apis.c {sources} /Feguest_apis.exe', 'if errorlevel 1 exit /b 1']
(out/'compile.cmd').write_text('\n'.join(commands))
subprocess.run(['cmd','/c','compile.cmd'],cwd=out,check=True)
subprocess.run([str(out/'guest_apis.exe')],check=True)
