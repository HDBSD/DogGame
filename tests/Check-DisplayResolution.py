from test_support import msbuild
"""Exercise production resizing, aspect ratio, input mapping and Options gating."""
from pathlib import Path
import subprocess

here = Path(__file__).resolve().parent
source = here.parent / 'build/runtime/xboxrecomp/src/video/fb_present.c'
out = here / 'irq-test-build'
title = (here.parent / 'src/title_display.c').read_text()
title = title.replace('#include "gen/recomp_types.h"', '')
title = title.replace('#include "title_display.h"', '')
title = title.replace('display_draw', 'title_display_draw')
for module in ['title_guest_state', 'title_guest_menu', 'title_guest_ui']:
    production = (here.parent / ('src/api/' + module + '.c')).read_text()
    production = production.replace('#include "gen/recomp_types.h"', '')
    for header in ['title_guest_state', 'title_guest_menu', 'title_guest_ui']:
        production = production.replace('#include "' + header + '.h"', '')
    title = title.replace('#include "api/' + module + '.h"',
                          '#include "' + (here.parent / ('src/api/' + module + '.h')).as_posix() + '"\n' + production)
manual = (here.parent / 'src/recomp_manual.c').read_text()
start = manual.index('static void native_display_event(void)')
wrapper = manual[start:manual.index('\n}\n', start)+3]
overrides = (here.parent / 'src/api/title_guest_routines.c').read_text()
start = overrides.index('void title_guest_update_camera_projection(void)')
camera = overrides[start:overrides.index('\n}\n',start)+3]
camera = camera.replace('title_guest_update_camera_projection', 'sub_000A0260')
code = r'''
#include <windows.h>
#include <Xinput.h>
#include <stdio.h>
#include <stdlib.h>
ptrdiff_t xbox_GetMemoryOffset(void) { return 0; }
''' + '#include "' + source.as_posix() + '"\n' + r'''
static unsigned char ram[0x200000];
static ptrdiff_t g_xbox_mem_offset=(ptrdiff_t)ram;
#define MEM32(a) (*(uint32_t *)(ram+(a)))
#define MEM8(a) ram[a]
#define MEM16(a) (*(uint16_t *)(ram+(a)))
#define MEMF(a) (*(float *)(ram+(a)))
#define XBOX_PTR(a) ((uintptr_t)(ram+(a)))
static uint32_t heap_next=0x10000,heap_calls;
uint32_t xbox_HeapAlloc(uint32_t size,uint32_t alignment) { (void)alignment;uint32_t result=heap_next;heap_next+=size;heap_calls++;return result; }
uint32_t title_display_find_asset(uint32_t id) { return id==0x9268FB6Cu?0x6000:0; }
static uint32_t menu_target,menu_event;
static void title_menu_event(uint32_t target,uint32_t event){menu_target=target;menu_event=event;if(event==79)MEM32(target+0xF0)|=2;if(event==80)MEM32(target+0xF0)&=~2u;}
''' + title + r'''
static uint32_t esp,eax,native_event_calls;
#define g_esp esp
#define g_eax eax
static void sub_0007C490(void) { native_event_calls++;eax=7;esp+=4; }
''' + wrapper + r'''
#include <math.h>
static uint32_t ecx;
static double g_fp_stack[8];static unsigned g_fp_top;
typedef struct {float f[4];} TestXmm;
static TestXmm xmm0,xmm1,xmm2;
static TestXmm scalar(float f) {TestXmm x={{f,0,0,0}};return x;}
#define XMM_SCALAR(v) scalar(v)
#define RECOMP_FP_PC(v) (v)
#define PUSH32(s,v) do{(s)-=4;MEM32(s)=(v);}while(0)
#define RECOMP_ABI_CALL(va,fn) (fn)()
static float camera_x,camera_y;
static void sub_000BDE40(void) {
    uint32_t window=MEM32(esp+8);camera_x=MEMF(window);camera_y=MEMF(window+4);esp+=4;
}
''' + camera + r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#c); return 1; } } while(0)
static int clicked_x,clicked_y;
static int click(int x,int y) { clicked_x=x;clicked_y=y;return 1; }
int main(void) {
    WNDCLASSA wc={0};wc.lpfnWndProc=fb_wndproc;wc.hInstance=GetModuleHandle(NULL);wc.lpszClassName="DisplayResolutionTest";
    CHECK(RegisterClassA(&wc));
    /* A hidden unit-test window; no game process or gameplay automation. */
    HWND hwnd=CreateWindowA(wc.lpszClassName,"test",WS_OVERLAPPEDWINDOW,0,0,640,480,NULL,NULL,wc.hInstance,NULL);
    /* FMV feature enabled must not suppress the high-resolution picture.
     * Test the production GDI draw, then an actual active movie overlay. */
    {
        BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth=640;bi.bmiHeader.biHeight=-480;
        bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
        void *bits=NULL;HDC dc=CreateCompatibleDC(NULL);
        HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,NULL,0);
        HGDIOBJ old=SelectObject(dc,bitmap);
        uint32_t high[4]={0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000};
        s_rgb=calloc(640*480,4);for(unsigned i=0;i<640*480;i++)s_rgb[i]=0xFF0000FF;
        s_high_rgb=high;s_high_draw_width=s_high_draw_height=2;
        _putenv_s("RECOMP_PVIDEO_OVERLAY","1");s_pvideo_active=0;
        display_draw_content(hwnd,dc,&bi);CHECK(GetPixel(dc,300,200)==RGB(255,0,0));
        s_pvideo_active=1;display_draw_content(hwnd,dc,&bi);
        CHECK(GetPixel(dc,300,200)==RGB(0,0,255));
        s_pvideo_active=0;s_high_draw_width=s_high_draw_height=0;s_high_rgb=NULL;
        free(s_rgb);s_rgb=NULL;_putenv_s("RECOMP_PVIDEO_OVERLAY","");
        SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
    }

    /* Borrowed high-resolution images remain publisher-owned. Selecting a
     * new publication or disabling scaling must never retain the old view. */
    {
        uint32_t first[4]={1,2,3,4},second[9]={0};
        int was_scaling=s_internal_scaling;s_internal_scaling=1;
        s_high_present[0]=first;s_high_width[0]=s_high_height[0]=2;
        s_high_present[1]=second;s_high_width[1]=s_high_height[1]=3;
        AcquireSRWLockShared(&s_present_lock);
        s_present_idx=0;borrow_high_snapshot();
        CHECK(s_high_rgb==first && s_high_draw_width==2);
        release_high_snapshot();CHECK(!s_high_rgb && !s_high_draw_width);
        s_present_idx=1;borrow_high_snapshot();
        CHECK(s_high_rgb==second && s_high_draw_height==3);
        s_internal_scaling=0;borrow_high_snapshot();CHECK(!s_high_rgb);
        s_present_idx=-1;borrow_high_snapshot();CHECK(!s_high_draw_width);
        ReleaseSRWLockShared(&s_present_lock);
        s_internal_scaling=was_scaling;
        s_high_present[0]=s_high_present[1]=NULL;
        s_high_width[0]=s_high_width[1]=s_high_height[0]=s_high_height[1]=0;
    }

    CHECK(hwnd);
    wchar_t tempdir[MAX_PATH],config[MAX_PATH];CHECK(GetTempPathW(MAX_PATH,tempdir));
    CHECK(GetTempFileNameW(tempdir,L"sdr",0,config));wcscpy(s_display_ini,config);
    for(unsigned i=0;i<xbox_DisplayResolutionCount();i++) {
        unsigned w,h;xbox_DisplayResolutionSize(i,&w,&h);xbox_DisplayResolutionSelect(i);display_apply(hwnd);
        RECT client,view=display_view(hwnd);GetClientRect(hwnd,&client);
        CHECK(client.right==(LONG)w && client.bottom==(LONG)h);
        CHECK(xbox_DisplayResolutionSelected()==i);
        CHECK(view.left>=0 && view.top>=0 && view.right<=client.right && view.bottom<=client.bottom);
        CHECK(llabs((view.right-view.left)*3LL-(view.bottom-view.top)*4LL)<=4);
        CHECK(GetPrivateProfileIntW(L"Display",L"Width",0,config)==w);
        CHECK(GetPrivateProfileIntW(L"Display",L"Height",0,config)==h);
    }
    xbox_DisplayResolutionSelect(3);display_apply(hwnd); /* 1280x720: pillarboxed 4:3 */
    RECT view=display_view(hwnd);CHECK(view.left==160 && view.right==1120 && view.top==0 && view.bottom==720);
    xbox_DisplayWidescreenSet(1);view=display_view(hwnd);
    CHECK(view.left==0 && view.right==1280 && view.bottom==720);
    CHECK(GetPrivateProfileIntW(L"Display",L"Widescreen",0,config)==1);
    /* Exercise both native FOV branches, toggling without accumulating scale. */
    MEMF(0x180B44)=1;MEMF(0x180DF4)=0.5f;MEMF(0x1906EC)=1;MEMF(0x18D5D8)=1;
    for(unsigned branch=0;branch<2;branch++) {
        MEMF(0x1A7004)=branch?640:480;MEMF(0x1A7008)=branch?480:640;
        float base_x=0,base_y=0;
        for(unsigned pass=0;pass<3;pass++) {
            xbox_DisplayWidescreenSet(pass==1);esp=0x9000;MEM32(esp+4)=0x4000;MEMF(esp+8)=1;
            sub_000A0260();CHECK(esp==0x9004 && g_fp_top==0);
            if(!pass) {base_x=camera_x;base_y=camera_y;}
            CHECK(fabsf(camera_x-base_x*(pass==1?4.0f/3.0f:1))<0.00001f);
            CHECK(camera_y==base_y);
        }
    }
    xbox_MenuOverlayRegister(NULL,NULL,click);
    fb_wndproc(hwnd,WM_LBUTTONDOWN,0,MAKELPARAM(640,360));CHECK(clicked_x==320 && clicked_y==240);
    clicked_x=-1;fb_wndproc(hwnd,WM_LBUTTONDOWN,0,MAKELPARAM(80,360));CHECK(clicked_x==-1);
    xbox_DisplayResolutionSelect(999);CHECK(s_resolution_request==0);
    xbox_DebugOverlayCapture(1);xbox_MenuOverlayCapture(1);xbox_DebugOverlayCapture(0);CHECK(xbox_DebugOverlayCaptured());
    xbox_MenuOverlayCapture(0);CHECK(!xbox_DebugOverlayCaptured());
    MEM32(0x1B8D9C)=5;title_display_frame();CHECK(s_options);CHECK(display_key('R'));CHECK(s_open && xbox_DebugOverlayCaptured());
    unsigned before=xbox_DisplayResolutionSelected();display_key(VK_RIGHT);CHECK(s_resolution_request==(LONG)((before+1)%xbox_DisplayResolutionCount())+1);
    int old_aspect=xbox_DisplayWidescreen();display_key(VK_DOWN);CHECK(s_selection==1);display_key(VK_RIGHT);CHECK(xbox_DisplayWidescreen()!=old_aspect);
    display_key(VK_DOWN);CHECK(s_selection==2);display_key(VK_RIGHT);CHECK(xbox_DisplayInternalScaling());
    LONG window_request=s_resolution_request;display_key(VK_UP);display_key(VK_UP);display_key(VK_RIGHT);
    CHECK(s_internal_resolution==1 && s_resolution_request==window_request);
    display_key(VK_DOWN);display_key(VK_DOWN);display_key(VK_LEFT);CHECK(!xbox_DisplayInternalScaling());
    display_key(VK_DOWN);display_key(VK_RIGHT);display_apply(hwnd);
    CHECK(xbox_DisplayBorderless() && !(GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CAPTION));
    display_key(VK_LEFT);display_apply(hwnd);CHECK(!xbox_DisplayBorderless() && (GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CAPTION));
    display_key(VK_ESCAPE);CHECK(!s_open);
    MEM32(0x1B8D9C)=6;title_display_frame();CHECK(!s_options && !display_key('R'));
    MEM32(0x1B8D9C)=7;MEM32(0x1A3C68)=0;title_display_frame();CHECK(!s_options);
    MEM32(0x1A3C68)=1;title_display_frame();CHECK(s_options);
    /* Actual MNU3 Options stays in mode2/state0. Native UI visibility gates it. */
    MEM32(0x1B8D9C)=2;MEM32(0x1A3C68)=0;MEM32(0x1BF88C)=0x1000;
    MEM32(0x1100)=3;MEM32(0x11AC)=0x2000;
    MEM32(0x2000)=0x6855A343;MEM32(0x2100)=0x8F2D6816;MEM32(0x2200)=0x41F6FF46;
    title_display_frame();CHECK(!s_options);
    MEM8(0x2014)=1;title_display_frame();CHECK(!s_options && !display_key('R'));
    /* Recomp entry redirects to the native computer-screen Options route. */
    MEM32(0x1100)=5;MEM32(0x2300)=0xB488CA08;MEM32(0x2400)=0xB5FB267B;
    MEM32(0x2308)=0x5000;MEM8(0x2305)=2;
    MEM16(0x5000)=63;MEM16(0x5002)=18;MEM32(0x5004)=0xBAD;
    MEM16(0x5020)=74;MEM16(0x5022)=79;MEM32(0x5024)=0x2CA9AD4B;
    MEM32(0x2408)=0x5100;MEM8(0x2405)=1;
    MEM16(0x5100)=63;MEM16(0x5102)=18;MEM32(0x5104)=0x477904E8;
    title_display_frame();CHECK(s_recomp_entry==0x2300);
    CHECK(!title_display_native_event(0x2300,63) && s_recomp);
    CHECK(MEM8(0x2305)==2 && MEM16(s_recomp_links)==74 && MEM32(s_recomp_links+36)==0x477904E8);
    MEM32(0x10FC)=2;MEM32(0x11A8)=0x7000;MEM32(0x7000)=0x417297B0;
    MEM32(0x70EC)=0x8000;MEMF(0x8020)=260;
    MEM32(0x70F8)=0xA13417C3;MEM8(0x710C)=1;
    title_display_frame();CHECK(s_open==2 && s_options && !(MEM8(0x2014)&1));
    CHECK(!(MEM8(0x7014)&1) && !(MEM8(0x710C)&1) && MEMF(0x8020)==112);
    MEM8(0x7014)=1;title_display_frame();CHECK(MEM8(0x7014)&1); /* Native blink owns visibility. */
    display_key(VK_DOWN);CHECK(s_selection==1);
    CHECK(xbox_DisplayMouseInteraction());display_key(VK_RETURN);CHECK(!xbox_DisplayMouseInteraction());
    CHECK(GetPrivateProfileIntW(L"Display",L"MouseInteraction",1,config)==0);
    clicked_x=-1;fb_wndproc(hwnd,WM_LBUTTONDOWN,0,MAKELPARAM(640,360));CHECK(clicked_x==-1);
    display_key(VK_RETURN);CHECK(xbox_DisplayMouseInteraction());
    display_key(VK_DOWN);CHECK(s_selection==2 && xbox_DisplayDynamicPrompts());
    display_key(VK_RETURN);CHECK(!xbox_DisplayDynamicPrompts());
    CHECK(GetPrivateProfileIntW(L"Display",L"DynamicButtonPrompts",1,config)==0);
    display_key(VK_RETURN);CHECK(xbox_DisplayDynamicPrompts());
    display_key(VK_DOWN);CHECK(s_selection==3 && !xbox_DisplayFpsCap());
    display_key(VK_RETURN);CHECK(xbox_DisplayFpsCap());
    CHECK(GetPrivateProfileIntW(L"Display",L"FpsCap30",0,config)==1);
    display_key(VK_RETURN);CHECK(!xbox_DisplayFpsCap());
    CHECK(GetPrivateProfileIntW(L"Display",L"FpsCap30",1,config)==0);
    display_hover(160,275);CHECK(s_selection==4);
    display_key(VK_DOWN);CHECK(s_selection==0);
    display_key(VK_UP);CHECK(s_selection==4);
    display_hover(160,235);CHECK(s_selection==3);
    display_click(160,235);CHECK(xbox_DisplayFpsCap());
    display_click(160,235);CHECK(!xbox_DisplayFpsCap());
    display_hover(160,195);CHECK(s_selection==2);

    display_key(VK_UP);
    display_key(VK_UP);display_key(VK_RETURN);CHECK(s_open==1);
    display_key(VK_ESCAPE);CHECK(s_open==2 && s_selection==0);
    display_key(VK_ESCAPE);CHECK(s_recomp_back);title_display_frame();
    CHECK(!s_open && !s_recomp && menu_target==0x2000 && menu_event==66 && s_return_focus);
    CHECK(!(MEM8(0x7014)&1) && MEMF(0x8020)==260);
    /* Simulate the native Back sequence focusing Options after our callback. */
    MEM8(0x2014)=MEM8(0x2114)=MEM8(0x2214)=0;MEM8(0x2314)=1;MEM32(0x24F0)=2;
    title_display_frame();CHECK(!(MEM8(0x7014)&1));
    CHECK(!s_return_focus && !(MEM32(0x24F0)&2) && (MEM32(0x23F0)&2));
    /* Native Quit row inherits Credits styling, follows it, and hides in submenus. */
    MEM32(0x1100)=7;MEM32(0x2500)=0xF7B264FC;MEM32(0x25F8)=0xA000;
    MEM32(0xA080)=0x9268FB6C;MEM32(0x6000)=10;
    MEM32(0x2600)=0x2CA9AD4B;MEM32(0x26F8)=0xA100;MEMF(0xA120)=220;
    MEM16(0xA154)=200;MEM16(0xA156)=40;MEM16(0x26FC)=123;
    MEM32(0x2608)=0xA200;MEM8(0x2605)=1;MEM16(0xA200)=73;MEM16(0xA202)=79;
    MEM8(0x2614)=1;MEM8(0x1C2029)=1;
    title_display_frame();CHECK(s_quit_entry==0x2500 && (MEM8(0x2514)&1));
    CHECK(MEMF(0xA020)==260 && MEM16(0x25FC)==123 && !strcmp((char*)ram+0x6004,"Quit Game"));
    CHECK(MEM8(0x2605)==3 && MEM32(s_credits_links+68)==0xF7B264FC);
    CHECK(item_event(0x2500)==63);
    s_fb_running=1;CHECK(title_display_native_event(0x2500,63) && s_fb_running==-1);
    MEM8(0x2314)=0;title_display_frame();CHECK(!(MEM8(0x2514)&1) && !(MEM32(0x25F0)&8));
    s_fb_running=1;CHECK(!title_display_native_event(0x2500,63) && s_fb_running==1);s_fb_running=0;
    DeleteFileW(config);DestroyWindow(hwnd);
    puts("Passed: exact client sizes, saved selection, 4:3 scaling, scaled mouse coordinates, capture ownership and main/pause Options gating.");
    return 0;
}
'''
(out / 'display_resolution.c').write_text(code, encoding='utf-8')
project = (out / 'apu_irq_harness.vcxproj').read_text().replace('apu_irq_harness.c', 'display_resolution.c').replace('obj\\apu_irq\\', 'obj\\display_resolution\\')
includes = (here.parent / 'build/runtime/xboxrecomp/include').as_posix()
project = project.replace('<RuntimeLibrary>', f'<AdditionalIncludeDirectories>{includes};%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories><RuntimeLibrary>')
project = project.replace('<ItemDefinitionGroup>', '<ItemDefinitionGroup><Link><AdditionalDependencies>User32.lib;Gdi32.lib;Xinput.lib;%(AdditionalDependencies)</AdditionalDependencies></Link>')
(out / 'display_resolution.vcxproj').write_text(project)
subprocess.run([msbuild, str(out / 'display_resolution.vcxproj'), '/p:Configuration=Release', '/p:Platform=x64', '/verbosity:minimal'], check=True)
subprocess.run([str(out / 'bin/display_resolution.exe')], check=True)

