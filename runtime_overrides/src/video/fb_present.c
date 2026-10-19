/**
 * Show the guest framebuffer in a window.
 *
 * The title renders into its own framebuffer in guest RAM and tells the kernel
 * where it is through AvSetDisplayMode; on hardware the CRTC scans that memory
 * out. Nothing here scans anything out, so however much of the GPU is
 * implemented, none of it is observable. This is the other half: a window that
 * reads that memory and puts it on screen.
 *
 * Deliberately plain GDI rather than the D3D8 layer. The point is to display
 * whatever the guest actually wrote, so the fewer stages between guest memory
 * and the screen the better -- and it must keep working while the D3D8 layer
 * is busy with something else, such as the FMV player's own window.
 *
 * Off unless RECOMP_FB_WINDOW is set.
 */
#include <stdint.h>
#include "../../include/xbox/debug_overlay.h"
#include "../../include/xbox/display_options.h"
#include "../../include/xbox/application_identity.h"
#include "../../include/xbox/renderer_options.h"

#if defined(_WIN32)
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern ptrdiff_t xbox_GetMemoryOffset(void);

/* 0: not started, 1: running, -1: closed permanently. A flip must never
 * restart the display while the closing thread is releasing its resources. */
static volatile LONG s_fb_running;
void xbox_DisplayRequestQuit(void) { InterlockedExchange(&s_fb_running,-1); }
static uint32_t      s_fb_va, s_fb_pitch, s_fb_width = 640, s_fb_height = 480;
static int s_pvideo_active; /* Actual movie pixels this frame, not feature enablement. */
static uint32_t     *s_rgb;           /* converted 32-bit copy for GDI */
static XboxOverlayDraw s_menu_draw;
static XboxOverlayUpdate s_menu_update, s_debug_update;
void xbox_DebugOverlayUpdateRegister(XboxOverlayUpdate update)
{
    s_debug_update = update;
}
void xbox_MenuOverlayUpdateRegister(XboxOverlayUpdate update)
{
    s_menu_update = update;
}
static XboxOverlayKey s_menu_key;
static XboxMenuClick s_menu_click;
static XboxMenuHover s_menu_hover;
static volatile LONG s_menu_capture;
static volatile LONG s_cursor_gameplay;
static int s_cursor_locked, s_cursor_suspended;
static RECT s_cursor_rect;
void xbox_DisplayGameplayCursor(int playing)
{ InterlockedExchange(&s_cursor_gameplay, playing != 0); }

static void cursor_release(void)
{
    if (s_cursor_locked) {
        ClipCursor(NULL);
        s_cursor_locked = 0;
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

static void cursor_update(HWND hwnd)
{
    RECT rect;
    if (!s_cursor_gameplay || s_menu_capture || xbox_DebugOverlayCaptured() ||
        s_cursor_suspended || GetForegroundWindow() != hwnd ||
        IsIconic(hwnd) || !IsWindowVisible(hwnd)) {
        cursor_release();
        return;
    }
    if (!GetClientRect(hwnd, &rect) || rect.right <= rect.left || rect.bottom <= rect.top) {
        cursor_release();
        return;
    }
    MapWindowPoints(hwnd, NULL, (POINT *)&rect, 2);
    if (!s_cursor_locked || !EqualRect(&rect, &s_cursor_rect)) {
        if (!ClipCursor(&rect)) { cursor_release(); return; }
        s_cursor_rect = rect;
        s_cursor_locked = 1;
        SetCursor(NULL);
    }
}
static const unsigned s_resolutions[][2] = {
    {640,480},{800,600},{1024,768},{1280,720},{1280,960},
    {1600,900},{1920,1080},{2560,1440},{3840,2160}
};
static unsigned s_resolution;
static wchar_t s_display_ini[MAX_PATH];
static volatile LONG s_internal_resolution;
static volatile LONG s_resolution_request;
static volatile LONG s_borderless,s_mode_request;
static volatile LONG s_internal_scaling;
static const wchar_t *display_renderer_section(void)
{
    return xbox_ExperimentalVulkanSelected() ? L"Vulkan" : L"Display";
}
int xbox_DisplayInternalScaling(void) { return s_internal_scaling!=0; }
void xbox_DisplayInternalScalingSet(int enabled)
{
    InterlockedExchange(&s_internal_scaling,enabled!=0);
    if(s_display_ini[0])WritePrivateProfileStringW(display_renderer_section(),L"InternalScaling",enabled?L"1":L"0",s_display_ini);
}
static RECT s_windowed_rect;
int xbox_DisplayBorderless(void) { return s_borderless!=0; }
void xbox_DisplayBorderlessSet(int enabled)
{
    InterlockedExchange(&s_borderless,enabled!=0);InterlockedExchange(&s_mode_request,1);
}
static volatile LONG s_widescreen;
static volatile LONG s_mouse_interaction=1;
static volatile LONG s_dynamic_prompts=1;
static volatile LONG s_pause_on_focus_loss = 1;
static volatile LONG s_focus_loss_pending;

int xbox_DisplayPauseOnFocusLoss(void)
{
    return InterlockedCompareExchange(&s_pause_on_focus_loss, 0, 0) != 0;
}

void xbox_DisplayPauseOnFocusLossSet(int enabled)
{
    InterlockedExchange(&s_pause_on_focus_loss, enabled != 0);
    if (!enabled)
    {
        InterlockedExchange(&s_focus_loss_pending, 0);
    }
    if (s_display_ini[0])
    {
        WritePrivateProfileStringW(L"Display", L"PauseOnFocusLoss", enabled ? L"1" : L"0", s_display_ini);
    }
}

int xbox_DisplayConsumeFocusLoss(void)
{
    return InterlockedExchange(&s_focus_loss_pending, 0) != 0 && xbox_DisplayPauseOnFocusLoss();
}

static void queue_focus_loss(void)
{
    if (xbox_DisplayPauseOnFocusLoss())
    {
        InterlockedExchange(&s_focus_loss_pending, 1);
    }
}
static volatile LONG s_fps_cap;
int xbox_DisplayFpsCap(void) { return s_fps_cap!=0; }
void xbox_DisplayFpsCapSet(int enabled)
{
    InterlockedExchange(&s_fps_cap,enabled!=0);
    if(s_display_ini[0]) WritePrivateProfileStringW(display_renderer_section(),L"FpsCap30",enabled?L"1":L"0",s_display_ini);
}
/* Limit guest flips, without changing its refresh clock or simulation delta.
 * Use the actual previous presentation time so late frames never produce
 * catch-up bursts. Slow frames are never delayed further. */
static void display_limit_frame(void)
{
    static LONGLONG deadline,period,frequency;
    static HANDLE timer;
    static int timer_attempted;
    LARGE_INTEGER now,value;
    if(!xbox_DisplayFpsCap()) { deadline=0;return; }
    if(!frequency){QueryPerformanceFrequency(&value);frequency=value.QuadPart;period=frequency/30;}
    if(!timer_attempted){timer_attempted=1;timer=CreateWaitableTimerExW(NULL,NULL,0x2,TIMER_ALL_ACCESS);}
    QueryPerformanceCounter(&now);
    if(!deadline || now.QuadPart>deadline+period)deadline=now.QuadPart;
    while(now.QuadPart<deadline){
        LONGLONG remaining=deadline-now.QuadPart;
        if(timer && remaining>frequency/2000){
            LARGE_INTEGER due;due.QuadPart=-(remaining*10000000/frequency-5000);
            if(SetWaitableTimer(timer,&due,0,NULL,NULL,FALSE))WaitForSingleObject(timer,INFINITE);
            else YieldProcessor();
        }else {
            DWORD milliseconds=(DWORD)(remaining*1000/frequency);
            if(milliseconds>2)Sleep(milliseconds-2);
            else YieldProcessor();
        }
        QueryPerformanceCounter(&now);
    }
    deadline=now.QuadPart+period;
}
int xbox_DisplayDynamicPrompts(void) { return s_dynamic_prompts!=0; }
void xbox_DisplayDynamicPromptsSet(int enabled)
{
    InterlockedExchange(&s_dynamic_prompts,enabled!=0);
    if(s_display_ini[0]) WritePrivateProfileStringW(L"Display",L"DynamicButtonPrompts",enabled?L"1":L"0",s_display_ini);
}
int xbox_DisplayMouseInteraction(void) { return s_mouse_interaction!=0; }
void xbox_DisplayMouseInteractionSet(int enabled)
{
    InterlockedExchange(&s_mouse_interaction,enabled!=0);
    if(s_display_ini[0]) WritePrivateProfileStringW(L"Display",L"MouseInteraction",enabled?L"1":L"0",s_display_ini);
}
int xbox_DisplayWidescreen(void) { return s_widescreen!=0; }
void xbox_DisplayWidescreenSet(int enabled)
{
    InterlockedExchange(&s_widescreen,enabled!=0);
    if(s_display_ini[0]) WritePrivateProfileStringW(L"Display",L"Widescreen",enabled?L"1":L"0",s_display_ini);
}

void xbox_MenuOverlayRegister(XboxOverlayDraw draw, XboxOverlayKey key, XboxMenuClick click)
{ s_menu_draw=draw; s_menu_key=key; s_menu_click=click; }
void xbox_MenuOverlayHoverRegister(XboxMenuHover hover) { s_menu_hover=hover; }
unsigned xbox_DisplayResolutionCount(void)
{
    unsigned count = sizeof(s_resolutions) / sizeof(s_resolutions[0]);
    return xbox_ExperimentalVulkanSelected() ? count : count - 1;
}
unsigned xbox_DisplayResolutionSelected(void) { return s_internal_scaling?(unsigned)s_internal_resolution:s_resolution; }
void xbox_DisplayResolutionSize(unsigned index, unsigned *width, unsigned *height)
{
    if(index>=xbox_DisplayResolutionCount()) index=0;
    *width=s_resolutions[index][0]; *height=s_resolutions[index][1];
}
void xbox_DisplayResolutionSelect(unsigned index)
{
    if(s_internal_scaling && index<xbox_DisplayResolutionCount()) {
        InterlockedExchange(&s_internal_resolution,index);
        if(s_display_ini[0]) {wchar_t value[32];_snwprintf(value,32,L"%u",index);WritePrivateProfileStringW(display_renderer_section(),L"InternalResolution",value,s_display_ini);}
        return;
    }
    if(index<xbox_DisplayResolutionCount()) InterlockedExchange(&s_resolution_request,(LONG)index+1);
}
static void display_load(void)
{
    int vulkan = xbox_ExperimentalVulkanSelected();
    const wchar_t *render_section = display_renderer_section();
    s_resolution=0;
    DWORD n=GetModuleFileNameW(NULL,s_display_ini,MAX_PATH);
    if(!n || n>=MAX_PATH) { s_display_ini[0]=0; return; }
    wchar_t *slash=wcsrchr(s_display_ini,L'\\');
    if(!slash || (size_t)(slash-s_display_ini)+13>=MAX_PATH) { s_display_ini[0]=0; return; }
    wcscpy(slash+1,L"display.ini");
    InterlockedExchange(&s_widescreen,GetPrivateProfileIntW(L"Display",L"Widescreen",0,s_display_ini)==1);
    InterlockedExchange(&s_mouse_interaction,GetPrivateProfileIntW(L"Display",L"MouseInteraction",1,s_display_ini)!=0);
    InterlockedExchange(&s_fps_cap,GetPrivateProfileIntW(render_section,L"FpsCap30",vulkan,s_display_ini)!=0);
    InterlockedExchange(&s_dynamic_prompts,GetPrivateProfileIntW(L"Display",L"DynamicButtonPrompts",1,s_display_ini)!=0);
    InterlockedExchange(&s_pause_on_focus_loss, GetPrivateProfileIntW(L"Display", L"PauseOnFocusLoss", 1, s_display_ini) != 0);
    InterlockedExchange(&s_borderless,GetPrivateProfileIntW(L"Display",L"Borderless",0,s_display_ini)!=0);
    InterlockedExchange(&s_internal_scaling,GetPrivateProfileIntW(render_section,L"InternalScaling",vulkan,s_display_ini)!=0);
    unsigned internal=GetPrivateProfileIntW(render_section,L"InternalResolution",0,s_display_ini);
    InterlockedExchange(&s_internal_resolution,internal<xbox_DisplayResolutionCount()?internal:0);
    unsigned width=GetPrivateProfileIntW(L"Display",L"Width",640,s_display_ini);
    unsigned height=GetPrivateProfileIntW(L"Display",L"Height",480,s_display_ini);
    for(unsigned i=0;i<xbox_DisplayResolutionCount();i++)
        if(s_resolutions[i][0]==width && s_resolutions[i][1]==height) { s_resolution=i; break; }
}
static void display_apply(HWND hwnd)
{
    LONG request=InterlockedExchange(&s_resolution_request,0);
    LONG mode_request=InterlockedExchange(&s_mode_request,0);
    if(request<=0 && !mode_request) return;
    unsigned index=request>0?(unsigned)request-1:s_resolution;
    if(s_borderless) {
        if(GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_OVERLAPPEDWINDOW)GetWindowRect(hwnd,&s_windowed_rect);
        MONITORINFO monitor={sizeof monitor};GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&monitor);
        SetWindowLongPtrW(hwnd,GWL_STYLE,WS_POPUP|WS_VISIBLE);
        SetWindowPos(hwnd,NULL,monitor.rcMonitor.left,monitor.rcMonitor.top,
            monitor.rcMonitor.right-monitor.rcMonitor.left,monitor.rcMonitor.bottom-monitor.rcMonitor.top,
            SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
    } else {
    SetWindowLongPtrW(hwnd,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);
    RECT r={0,0,(LONG)s_resolutions[index][0],(LONG)s_resolutions[index][1]};
    AdjustWindowRect(&r,(DWORD)GetWindowLongPtrW(hwnd,GWL_STYLE),FALSE);
    if(!SetWindowPos(hwnd,NULL,s_windowed_rect.left,s_windowed_rect.top,r.right-r.left,r.bottom-r.top,
        SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|(mode_request && s_windowed_rect.right?0:SWP_NOMOVE))) {
        fprintf(stderr,"[DISPLAY] Window resize failed: %lu\n",GetLastError()); return;
    }
    }
    s_resolution=index;
    if(s_display_ini[0]) {
        WritePrivateProfileStringW(L"Display",L"Borderless",s_borderless?L"1":L"0",s_display_ini);
        wchar_t value[32];
        _snwprintf(value,32,L"%u",s_resolutions[index][0]);
        BOOL ok=WritePrivateProfileStringW(L"Display",L"Width",value,s_display_ini);
        _snwprintf(value,32,L"%u",s_resolutions[index][1]);
        ok=WritePrivateProfileStringW(L"Display",L"Height",value,s_display_ini)&&ok;
        if(!ok) fprintf(stderr,"[DISPLAY] Could not save display.ini: %lu\n",GetLastError());
    }
}
static RECT display_view(HWND hwnd)
{
    RECT client,view; GetClientRect(hwnd,&client);
    LONG width=client.right,height=client.bottom;
    LONG aspect_w=s_widescreen?16:s_fb_width,aspect_h=s_widescreen?9:s_fb_height;
    if(width*(int64_t)aspect_h>height*(int64_t)aspect_w) width=(LONG)(height*(int64_t)aspect_w/aspect_h);
    else height=(LONG)(width*(int64_t)aspect_h/aspect_w);
    view.left=(client.right-width)/2;view.top=(client.bottom-height)/2;
    view.right=view.left+width;view.bottom=view.top+height;return view;
}

/* A finished frame, taken at the flip and shown until the next one.
 *
 * The window used to convert straight out of guest memory every 16 ms. Even
 * pointed at the buffer the title had just finished, that races the executor
 * drawing the next frame into the other one and, whenever the two swap, puts
 * a half-drawn image on the screen -- which is the flicker. Copying the
 * finished frame once per flip means the window never reads memory the
 * rasteriser is writing, so what it shows cannot be half of anything.
 *
 * Two buffers and an index, swapped after the copy completes, so the window
 * thread is never reading the one being filled. */
static uint32_t     *s_present[3];
static volatile LONG s_present_idx = -1;   /* -1 until the first flip */
static SRWLOCK s_present_lock=SRWLOCK_INIT;
static uint32_t *s_high_present[3];
static const uint32_t *s_high_rgb;
static size_t s_high_capacity[3];
static unsigned s_high_width[3],s_high_height[3],s_high_draw_width,s_high_draw_height;
static SRWLOCK s_snapshot_slots[3] = {SRWLOCK_INIT, SRWLOCK_INIT, SRWLOCK_INIT};
static LONG s_borrowed_slot = -1; /* Window thread only. */

/* Vulkan publication uses a free slot instead of waiting for a GDI draw.
 * Hold the index lock only long enough to lease the current immutable slot.
 * The original global-lock path remains in use for DirectX/pinned startup. */
static int acquire_present_snapshot(void)
{
    if (s_borrowed_slot >= 0)
    {
        return 0;
    }
    if (!TryAcquireSRWLockShared(&s_present_lock))
    {
        return 0;
    }
    if (xbox_ExperimentalVulkanSelected() && s_present_idx >= 0)
    {
        s_borrowed_slot = s_present_idx;
        AcquireSRWLockShared(&s_snapshot_slots[s_borrowed_slot]);
        ReleaseSRWLockShared(&s_present_lock);
    }
    return 1;
}

static void release_present_snapshot(void)
{
    if (s_borrowed_slot >= 0)
    {
        ReleaseSRWLockShared(&s_snapshot_slots[s_borrowed_slot]);
        s_borrowed_slot = -1;
    }
    else
    {
        ReleaseSRWLockShared(&s_present_lock);
    }
}
/* Window thread only, with the DirectX index lock or a Vulkan slot lease held
 * through every GDI consumer. The publisher cannot replace, resize or
 * overwrite that slot while it is borrowed. Borrowing the image avoids
 * another full-resolution allocation and copy per frame. Never retain the
 * pointer after releasing ownership, including across message dispatch. */
static void borrow_high_snapshot(void)
{
    s_high_rgb = NULL;
    s_high_draw_width = s_high_draw_height = 0;
    LONG index = s_borrowed_slot >= 0 ? s_borrowed_slot : s_present_idx;
    if (index < 0 || !xbox_DisplayInternalScaling())
    {
        return;
    }
    if (s_high_present[index] && s_high_width[index] && s_high_height[index])
    {
        s_high_rgb = s_high_present[index];
        s_high_draw_width = s_high_width[index];
        s_high_draw_height = s_high_height[index];
    }
}

static void release_high_snapshot(void)
{
    s_high_rgb = NULL;
    s_high_draw_width = s_high_draw_height = 0;
}

static void (*s_world_capture)(void);
void xbox_DebugWorldFrameRegister(void (*capture)(void)) {s_world_capture=capture;}

void xbox_FramebufferWindowSet(uint32_t fb_va, uint32_t pitch)
{
    /* RECOMP_FB_VA pins the window to one guest address instead of following
     * whichever surface is being drawn into. A black window cannot distinguish
     * "the read path is broken" from "the title rendered black", and pointing
     * it at memory known to have content settles that. */
    const char *pin = getenv("RECOMP_FB_VA");

    s_fb_va = pin ? (uint32_t)strtoul(pin, NULL, 0) : fb_va;
    if (pitch)
        s_fb_pitch = pitch;
}

/* Called by the pushbuffer executor when the title flips. */
void xbox_FramebufferWindowPresentHigh(uint32_t fb_va, uint32_t pitch, const uint32_t *pixels, unsigned width, unsigned height)
{
    const uint8_t *src;
    LONG next;
    uint32_t bpp, x, y;

    if (!s_fb_running || !fb_va || !pitch)
        return;
    if (getenv("RECOMP_FB_VA"))
        return;                       /* pinned: leave the old path alone */
    display_limit_frame();
    int leased_publication = xbox_ExperimentalVulkanSelected();
    if (leased_publication)
    {
        LONG current = InterlockedCompareExchange(&s_present_idx, 0, 0);
        for (next = 0; next < 3; next++)
        {
            if (next != current && TryAcquireSRWLockExclusive(&s_snapshot_slots[next]))
            {
                break;
            }
        }
        if (next == 3)
        {
            return;
        }
    }
    else
    {
        AcquireSRWLockExclusive(&s_present_lock);
        next = (s_present_idx == 0) ? 1 : 0;
    }
    if (!s_present[next]) {
        s_present[next] = (uint32_t *)calloc((size_t)s_fb_width * s_fb_height,
                                             4);
        if (!s_present[next]) {
            if (leased_publication)
            {
                ReleaseSRWLockExclusive(&s_snapshot_slots[next]);
            }
            else
            {
                ReleaseSRWLockExclusive(&s_present_lock);
            }
            return;
        }
    }
    bpp = pitch / s_fb_width;
    src = (const uint8_t *)((uintptr_t)fb_va + xbox_GetMemoryOffset());
    for (y = 0; y < s_fb_height; y++) {
        const uint8_t *row = src + (size_t)y * pitch;
        uint32_t *dst = s_present[next] + (size_t)y * s_fb_width;

        if (bpp == 4) {
            memcpy(dst, row, (size_t)s_fb_width * 4);
        } else if (bpp == 2) {
            const uint16_t *p = (const uint16_t *)row;
            for (x = 0; x < s_fb_width; x++) {
                uint16_t v = p[x];
                uint32_t r = (uint32_t)((v >> 11) & 0x1F) * 255u / 31u;
                uint32_t g = (uint32_t)((v >>  5) & 0x3F) * 255u / 63u;
                uint32_t b = (uint32_t)( v        & 0x1F) * 255u / 31u;
                dst[x] = (r << 16) | (g << 8) | b;
            }
        } else {
            memset(dst, 0, (size_t)s_fb_width * 4);
        }
    }
    s_high_width[next]=s_high_height[next]=0;
    if(pixels && width && height && width<=3840 && height<=2160){
        size_t count=(size_t)width*height;
        if(s_high_capacity[next]<count){
            uint32_t *buffer=(uint32_t*)realloc(s_high_present[next],count*4);
            if(buffer){s_high_present[next]=buffer;s_high_capacity[next]=count;}
        }
        if(s_high_capacity[next]>=count){
            memcpy(s_high_present[next],pixels,count*4);
            s_high_width[next]=width;s_high_height[next]=height;
        }
    }
    /* Published only once it is whole. */
    if(s_world_capture)s_world_capture();
    if (leased_publication)
    {
        AcquireSRWLockExclusive(&s_present_lock);
    }
    InterlockedExchange(&s_present_idx, next);
    ReleaseSRWLockExclusive(&s_present_lock);
    if (leased_publication)
    {
        ReleaseSRWLockExclusive(&s_snapshot_slots[next]);
    }
}

void xbox_FramebufferWindowPresent(uint32_t fb_va,uint32_t pitch)
{
    xbox_FramebufferWindowPresentHigh(fb_va,pitch,NULL,0,0);
}

/* Which keys are down, for the pad stand-in in src/input.
 *
 * GetAsyncKeyState looked like the cheaper way to ask and does not work
 * here: it reads a state Wine keeps for the X server, and a guest process
 * drawing through GDI never sees it change. The window that has the focus
 * is the thing that receives the keys, so that is what has to remember
 * them.
 *
 * Reading this needs no lock. Each entry is written only by the window
 * thread and read only by the USB thread, one byte at a time, and a press
 * seen a frame late is indistinguishable from one made a frame later. */
static volatile unsigned char s_key_down[256];
static volatile LONG s_keyboard_serial;
unsigned xbox_FramebufferKeyboardSerial(void) { return (unsigned)s_keyboard_serial; }

/* Title bar, the way ps3recomp's window shows it. Written by the flip,
 * read once a second by the window thread; a torn read shows one stale
 * number for a second, which nobody can tell apart from a real one. */
static wchar_t       s_title[48] = DOGGAME_APP_NAME_W;
static volatile LONG s_flips;
static volatile LONG s_frame_draws;
static XboxOverlayDraw s_debug_draw;
static XboxOverlayDraw s_debug_world_draw;
void xbox_DebugWorldRegister(XboxOverlayDraw draw) { s_debug_world_draw=draw; }
void xbox_DebugWorldComposite(void *device,const uint32_t *argb,unsigned width,unsigned height) {
    static uint32_t *composite;static size_t capacity;
    if(!s_rgb||width!=s_fb_width||height!=s_fb_height||!argb)return;
    unsigned output_width=s_high_draw_width?s_high_draw_width:width;
    unsigned output_height=s_high_draw_height?s_high_draw_height:height;
    size_t count=(size_t)output_width*output_height;
    if(count>capacity){uint32_t *p=realloc(composite,count*4);if(!p)return;composite=p;capacity=count;}
    for(unsigned y=0;y<output_height;y++)for(unsigned x=0;x<output_width;x++) {
        size_t i=(size_t)y*output_width+x;
        size_t native=(size_t)(y*height/output_height)*width+x*width/output_width;
        uint32_t src=argb[native],dst=s_high_draw_width?s_high_rgb[i]:s_rgb[native];
        unsigned a=src>>24;
        unsigned r=(((src>>16)&255)*a+((dst>>16)&255)*(255-a)+127)/255;
        unsigned g=(((src>>8)&255)*a+((dst>>8)&255)*(255-a)+127)/255;
        unsigned b=((src&255)*a+(dst&255)*(255-a)+127)/255;
        composite[i]=(r<<16)|(g<<8)|b;
    }
    BITMAPINFO info={0};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=output_width;info.bmiHeader.biHeight=-(LONG)output_height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    StretchDIBits((HDC)device,0,0,width,height,0,0,output_width,output_height,composite,&info,DIB_RGB_COLORS,SRCCOPY);
}
static XboxOverlayKey s_debug_key;
static volatile LONG s_debug_capture, s_debug_graph;
static SRWLOCK s_graph_lock = SRWLOCK_INIT;
static char s_graph_location[160]="Level: ----";
static float s_frame_ms[180];
static unsigned s_graph_head;
static LARGE_INTEGER s_graph_time;
void xbox_DebugOverlayRegister(XboxOverlayDraw draw, XboxOverlayKey key) { s_debug_draw=draw;s_debug_key=key; }
void xbox_DebugOverlayCapture(int capture) {
    LONG old=InterlockedExchange(&s_debug_capture,capture);
    if(old!=capture) memset((void *)s_key_down,0,sizeof s_key_down);
}
void xbox_MenuOverlayCapture(int capture) {
    LONG old=InterlockedExchange(&s_menu_capture,capture);
    if(old!=capture) memset((void *)s_key_down,0,sizeof s_key_down);
}
int xbox_DebugOverlayCaptured(void) { return s_debug_capture!=0 || s_menu_capture!=0; }
void xbox_DebugOverlayGraph(int enabled) { InterlockedExchange(&s_debug_graph,enabled); }
void xbox_DebugOverlayLocation(const char *text) {
    AcquireSRWLockExclusive(&s_graph_lock);
    snprintf(s_graph_location,sizeof s_graph_location,"%s",text);
    ReleaseSRWLockExclusive(&s_graph_lock);
}
static void debug_panel_background(HDC dc,int x,int y,int width,int height) {
    /* Composite against the actual presented pixels, then draw the small
     * result with the same DIB path as the game. Do not depend on display-DC
     * alpha blending, or mutate s_rgb (repaints must not darken it twice). */
    static uint32_t panel[612*46];
    if(!s_rgb||width<=0||height<=0||width>612||height>46)return;
    for(int row=0;row<height;row++)for(int col=0;col<width;col++) {
        uint32_t color=0;
        if((unsigned)(x+col)<s_fb_width&&(unsigned)(y+row)<s_fb_height)
            color=s_rgb[(size_t)(y+row)*s_fb_width+x+col];
        uint32_t red=((color>>16)&255)*102/255;
        uint32_t green=((color>>8)&255)*102/255;
        uint32_t blue=(color&255)*102/255;
        panel[row*width+col]=(red<<16)|(green<<8)|blue;
    }
    BITMAPINFO info={0};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    info.bmiHeader.biCompression=BI_RGB;
    StretchDIBits(dc,x,y,width,height,0,0,width,height,panel,&info,DIB_RGB_COLORS,SRCCOPY);
}
static void debug_overlay_draw(HDC dc) {
    if(s_menu_draw) s_menu_draw(dc,s_fb_width,s_fb_height);
    if(s_debug_draw) s_debug_draw(dc,s_fb_width,s_fb_height);
    if(s_debug_graph) {
        float samples[180];unsigned head;POINT points[180];char location[160];
        AcquireSRWLockShared(&s_graph_lock);
        memcpy(samples,s_frame_ms,sizeof samples);head=s_graph_head;
        memcpy(location,s_graph_location,sizeof location);ReleaseSRWLockShared(&s_graph_lock);
        debug_panel_background(dc,8,8,612,46);
        SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));
        TextOutA(dc,12,11,location,(int)strlen(location));
        char label[80];float ms=samples[(head+179)%180];
        snprintf(label,sizeof label,"%.1f FPS | %.1f ms",ms>0?1000/ms:0,ms);
        TextOutA(dc,12,32,label,(int)strlen(label));
        for(unsigned i=0;i<180;i++) {
            float v=samples[(head+i)%180];if(v>100)v=100;if(v<0)v=0;
            points[i].x=280+(int)i*332/179;points[i].y=50-(int)(v*0.18f);
        }
        HPEN p=CreatePen(PS_SOLID,1,RGB(70,255,110));HGDIOBJ old=SelectObject(dc,p);Polyline(dc,points,180);SelectObject(dc,old);DeleteObject(p);
    }
}

static void display_draw_content(HWND hwnd,HDC dc,const BITMAPINFO *bi)
{
    RECT client,view=display_view(hwnd);GetClientRect(hwnd,&client);
    if(view.right<=view.left || view.bottom<=view.top) return;
    /* Clear only the bars; clearing the picture first would flash black
     * between the GDI clear and blit on every frame. */
    RECT bars[4]={{0,0,client.right,view.top},{0,view.bottom,client.right,client.bottom},
        {0,view.top,view.left,view.bottom},{view.right,view.top,client.right,view.bottom}};
    for(unsigned i=0;i<4;i++) if(bars[i].right>bars[i].left && bars[i].bottom>bars[i].top)
        FillRect(dc,&bars[i],(HBRUSH)GetStockObject(BLACK_BRUSH));
    SetStretchBltMode(dc,COLORONCOLOR);
    if(s_high_draw_width && s_high_draw_height && !s_pvideo_active) {
        BITMAPINFO high=*bi;
        high.bmiHeader.biWidth=s_high_draw_width;
        high.bmiHeader.biHeight=-(LONG)s_high_draw_height;
        high.bmiHeader.biSizeImage=s_high_draw_width*s_high_draw_height*4;
        StretchDIBits(dc,view.left,view.top,view.right-view.left,view.bottom-view.top,
            0,0,s_high_draw_width,s_high_draw_height,s_high_rgb,&high,DIB_RGB_COLORS,SRCCOPY);
    }
    else if(s_rgb) StretchDIBits(dc,view.left,view.top,view.right-view.left,view.bottom-view.top,
        0,0,s_fb_width,s_fb_height,s_rgb,bi,DIB_RGB_COLORS,SRCCOPY);
    else FillRect(dc,&view,(HBRUSH)GetStockObject(BLACK_BRUSH));
    int saved=SaveDC(dc);
    SetMapMode(dc,MM_ANISOTROPIC);
    SetWindowExtEx(dc,s_fb_width,s_fb_height,NULL);
    SetViewportExtEx(dc,view.right-view.left,view.bottom-view.top,NULL);
    SetViewportOrgEx(dc,view.left,view.top,NULL);
    if (s_debug_world_draw) s_debug_world_draw(dc, s_fb_width, s_fb_height);
    RestoreDC(dc,saved);
}

/* Compose the game, world diagnostics and menus offscreen before one blit.
 * Drawing the base image directly to the window exposed it between overlay
 * paints, especially while collision rasterization was in progress. */
/* UI-owned bitmaps retain a complete background, including its matching world
 * diagnostics. Menus compose over that image even when publication is busy.
 * No borrowed publisher pointers survive the shared-lock scope. */
typedef struct WindowCanvas
{
    HDC dc;
    HBITMAP bitmap;
    HGDIOBJ original;
    LONG width, height;
} WindowCanvas;

static WindowCanvas s_base_canvas, s_menu_canvas;

static void release_canvas(WindowCanvas *canvas)
{
    if (canvas->dc)
    {
        SelectObject(canvas->dc, canvas->original);
        DeleteObject(canvas->bitmap);
        DeleteDC(canvas->dc);
    }
    memset(canvas, 0, sizeof(*canvas));
}

static int resize_canvas(WindowCanvas *canvas, HDC dc, LONG width, LONG height)
{
    if (canvas->dc && canvas->width == width && canvas->height == height)
    {
        return 1;
    }
    HDC next_dc = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
    if (!next_dc || !bitmap)
    {
        if (next_dc) DeleteDC(next_dc);
        if (bitmap) DeleteObject(bitmap);
        return 0;
    }
    release_canvas(canvas);
    canvas->dc = next_dc;
    canvas->bitmap = bitmap;
    canvas->original = SelectObject(next_dc, bitmap);
    canvas->width = width;
    canvas->height = height;
    PatBlt(next_dc, 0, 0, width, height, BLACKNESS);
    return 1;
}

static void window_update_input(void)
{
    static ULONGLONG last_update;
    ULONGLONG started = GetTickCount64();
    int trace = getenv("DOGGAME_INPUT_TRACE") != NULL;
    if (trace && last_update && started - last_update > 100)
    {
        fprintf(stderr, "[INPUT TRACE %llu] UI update gap=%llu ms\n",
                (unsigned long long)started, (unsigned long long)(started - last_update));
    }
    last_update = started;
    if (s_menu_update)
    {
        s_menu_update();
    }
    if (s_debug_update)
    {
        s_debug_update();
    }
}

static void display_draw(HWND hwnd, HDC dc, const BITMAPINFO *bi, int refresh_base)
{
    RECT client, view = display_view(hwnd);
    GetClientRect(hwnd, &client);
    if (client.right <= 0 || client.bottom <= 0 ||
        view.right <= view.left || view.bottom <= view.top)
    {
        return;
    }
    if (!resize_canvas(&s_base_canvas, dc, client.right, client.bottom) ||
        !resize_canvas(&s_menu_canvas, dc, client.right, client.bottom))
    {
        return;
    }
    if (refresh_base)
    {
        display_draw_content(hwnd, s_base_canvas.dc, bi);
    }
    BitBlt(s_menu_canvas.dc, 0, 0, client.right, client.bottom,
           s_base_canvas.dc, 0, 0, SRCCOPY);
    int saved = SaveDC(s_menu_canvas.dc);
    SetMapMode(s_menu_canvas.dc, MM_ANISOTROPIC);
    SetWindowExtEx(s_menu_canvas.dc, s_fb_width, s_fb_height, NULL);
    SetViewportExtEx(s_menu_canvas.dc, view.right - view.left, view.bottom - view.top, NULL);
    SetViewportOrgEx(s_menu_canvas.dc, view.left, view.top, NULL);
    debug_overlay_draw(s_menu_canvas.dc);
    RestoreDC(s_menu_canvas.dc, saved);
    BitBlt(dc, 0, 0, client.right, client.bottom, s_menu_canvas.dc, 0, 0, SRCCOPY);
}

void xbox_FramebufferWindowSetTitle(const uint16_t *name, int max_chars)
{
    /* The host product name takes precedence over the original XBE certificate. */
    (void)name;
    (void)max_chars;
}

void xbox_FramebufferWindowFrameStats(uint32_t draws)
{
    if(s_debug_graph) {
        LARGE_INTEGER now,freq;QueryPerformanceCounter(&now);QueryPerformanceFrequency(&freq);
        AcquireSRWLockExclusive(&s_graph_lock);
        if(s_graph_time.QuadPart) {s_frame_ms[s_graph_head]=(float)((now.QuadPart-s_graph_time.QuadPart)*1000.0/freq.QuadPart);s_graph_head=(s_graph_head+1)%180;}
        s_graph_time=now;ReleaseSRWLockExclusive(&s_graph_lock);
    } else s_graph_time.QuadPart=0;
    InterlockedIncrement(&s_flips);
    InterlockedExchange(&s_frame_draws, (LONG)draws);
}

int xbox_FramebufferFocused(void)
{
 HWND foreground=GetForegroundWindow();DWORD pid=0;
 if(!foreground)return 0;GetWindowThreadProcessId(foreground,&pid);
 char class_name[64];GetClassNameA(foreground,class_name,sizeof class_name);
 return s_fb_running==1 && pid==GetCurrentProcessId() && !strcmp(class_name,"XboxRecompFramebuffer") && !IsIconic(foreground);
}
int xbox_FramebufferKeyDown(int vk)
{
    if ((unsigned)vk > 255)
        return 0;
    return s_key_down[vk] != 0;
}

static LRESULT CALLBACK fb_wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_SETCURSOR:
        cursor_update(h);
        if (s_cursor_locked && LOWORD(l) == HTCLIENT) {
            SetCursor(NULL);
            return TRUE;
        }
        break;
    case WM_ENTERSIZEMOVE:
    case WM_ENTERMENULOOP:
        s_cursor_suspended = 1;
        cursor_release();
        break;
    case WM_EXITSIZEMOVE:
    case WM_EXITMENULOOP:
        s_cursor_suspended = 0;
        cursor_update(h);
        break;
    case WM_ACTIVATEAPP:
        if (!w) cursor_release();
        break;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
        if(!s_mouse_interaction) return 0;
        {
            RECT view=display_view(h);
            int x=(short)LOWORD(l),y=(short)HIWORD(l);
            if(view.right>view.left && view.bottom>view.top && x>=view.left && x<view.right && y>=view.top && y<view.bottom) {
                x=(int)((x-view.left)*(int64_t)s_fb_width/(view.right-view.left));
                y=(int)((y-view.top)*(int64_t)s_fb_height/(view.bottom-view.top));
                if(m==WM_MOUSEMOVE) {
                    if(s_menu_hover) s_menu_hover(x,y);
                    return 0;
                }
                if(s_menu_click && s_menu_click(x,y)) return 0;
                if(s_debug_key && x>=12 && x<300 && y>=(int)s_fb_height-38 && y<(int)s_fb_height-10 && s_debug_key(VK_F1)) return 0;
            }
        }
        break;
    case WM_PRINTCLIENT:
        if(s_rgb) {
            BITMAPINFO bi;memset(&bi,0,sizeof bi);
            bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=s_fb_width;bi.bmiHeader.biHeight=-(LONG)s_fb_height;
            bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
            int acquired = acquire_present_snapshot();
            if (acquired) borrow_high_snapshot();
            display_draw(h, (HDC)w, &bi, acquired);
            if (acquired)
            {
                release_high_snapshot();
                release_present_snapshot();
            }
        }
        return 0;
    case WM_CLOSE:
    case WM_DESTROY:
        cursor_release();
        InterlockedExchange(&s_fb_running, -1);
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (getenv("DOGGAME_INPUT_TRACE"))
        {
            fprintf(stderr, "[INPUT TRACE %llu] keyboard down=%u repeat=%d\n",
                    (unsigned long long)GetTickCount64(), (unsigned)w, (l & (1L << 30)) != 0);
        }
        if(m==WM_KEYDOWN && !(l&(1L<<30))) InterlockedIncrement(&s_keyboard_serial);
        if(m==WM_KEYDOWN && s_menu_key && s_menu_key((unsigned)w)) return 0;
        if(m==WM_KEYDOWN && w==VK_F1 && (l&(1L<<30))) return 0;
        if(m==WM_KEYDOWN && s_debug_key && s_debug_key((unsigned)w)) return 0;
        if ((unsigned)w < 256)
            s_key_down[w] = 1;
        if(w==VK_SHIFT) s_key_down[MapVirtualKeyA((unsigned)(l>>16)&255,MAPVK_VSC_TO_VK_EX)]=1;
        if(w==VK_CONTROL) s_key_down[(l&(1L<<24))?VK_RCONTROL:VK_LCONTROL]=1;
        /* RECOMP_KEY_TRACE: each key as it arrives, edge-triggered.
         *
         * The obvious diagnostic -- sampling which keys are held, once a
         * second, from the input path -- cannot tell a key that was never
         * pressed from one that was tapped: a 100 ms press is caught about
         * one time in ten. That ambiguity is expensive when the only way
         * to test is to ask someone to press a key and describe what
         * happened. This answers "did it arrive" on its own. */
        if (getenv("RECOMP_KEY_TRACE")) {
            static unsigned n;
            if (n++ < 40) {
                fprintf(stderr, "  [KEY] down vk=0x%02X\n", (unsigned)w);
                fflush(stderr);
            }
        }
        /* System keys still go to Windows, or Alt+F4 stops closing us. */
        return m == WM_SYSKEYDOWN ? DefWindowProcA(h, m, w, l) : 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if ((unsigned)w < 256)
            s_key_down[w] = 0;
        if(w==VK_SHIFT) s_key_down[MapVirtualKeyA((unsigned)(l>>16)&255,MAPVK_VSC_TO_VK_EX)]=0;
        if(w==VK_CONTROL) s_key_down[(l&(1L<<24))?VK_RCONTROL:VK_LCONTROL]=0;
        return m == WM_SYSKEYUP ? DefWindowProcA(h, m, w, l) : 0;

    /* Alt-tabbing away with a key held would leave it held for ever. */
    case WM_KILLFOCUS:
        cursor_release();
        queue_focus_loss();
        memset((void *)s_key_down, 0, sizeof s_key_down);
        return 0;
    }
    return DefWindowProcA(h, m, w, l);
}

/* The display formats an Xbox front buffer is actually set to. The pitch says
 * how wide a row is in bytes, so pitch/width gives the pixel size; the exact
 * component layout only matters for 16-bit, where 5:6:5 and 1:5:5:5 differ. */
static void fb_convert(const uint8_t *src, uint32_t bpp)
{
    uint32_t x, y;

    for (y = 0; y < s_fb_height; y++) {
        const uint8_t *row = src + (size_t)y * s_fb_pitch;
        uint32_t *dst = s_rgb + (size_t)y * s_fb_width;

        if (bpp == 4) {
            memcpy(dst, row, (size_t)s_fb_width * 4);
        } else if (bpp == 2) {
            const uint16_t *p = (const uint16_t *)row;
            for (x = 0; x < s_fb_width; x++) {
                uint16_t v = p[x];
                uint32_t r = (uint32_t)((v >> 11) & 0x1F) * 255u / 31u;
                uint32_t g = (uint32_t)((v >>  5) & 0x3F) * 255u / 63u;
                uint32_t b = (uint32_t)( v        & 0x1F) * 255u / 31u;
                dst[x] = (r << 16) | (g << 8) | b;
            }
        } else {
            memset(dst, 0, (size_t)s_fb_width * 4);
        }
    }
}

/* Write what the window is currently showing to a 24-bit BMP.
 *
 * A black window is ambiguous: it means either that the read path is wrong or
 * that the title really did render black. Dumping the same converted pixels
 * the window draws settles which, and does it without a screenshot. */
int xbox_FramebufferDumpBmp(const char *path)
{
    FILE *f;
    uint32_t row = ((s_fb_width * 3u) + 3u) & ~3u;
    uint32_t img = row * s_fb_height, total = 54u + img, y, x;
    uint8_t hdr[54], *line;

    if (!s_rgb || !s_fb_va)
        return -1;
    f = fopen(path, "wb");
    if (!f)
        return -1;
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    memcpy(hdr + 2, &total, 4);
    hdr[10] = 54; hdr[14] = 40;
    memcpy(hdr + 18, &s_fb_width, 4);
    memcpy(hdr + 22, &s_fb_height, 4);
    hdr[26] = 1; hdr[28] = 24;
    memcpy(hdr + 34, &img, 4);
    fwrite(hdr, 1, sizeof(hdr), f);

    line = (uint8_t *)calloc(1, row);
    for (y = 0; y < s_fb_height; y++) {
        const uint32_t *src = s_rgb + (size_t)(s_fb_height - 1 - y) * s_fb_width;
        for (x = 0; x < s_fb_width; x++) {
            line[x * 3 + 0] = (uint8_t)(src[x] & 0xFF);
            line[x * 3 + 1] = (uint8_t)((src[x] >> 8) & 0xFF);
            line[x * 3 + 2] = (uint8_t)((src[x] >> 16) & 0xFF);
        }
        fwrite(line, 1, row, f);
    }
    free(line);
    fclose(f);
    fprintf(stderr, "  [FBWIN] wrote %s (%ux%u from 0x%08X)\n",
            path, s_fb_width, s_fb_height, s_fb_va);
    return 0;
}

static uint32_t overlay_channel(int value)
{
    return value < 0 ? 0u : value > 255 ? 255u : (uint32_t)value;
}

/* PVIDEO scans a packed YUY2 surface independently of the 3D framebuffer.
 * XMV uses this path, so presenting only CRTC memory produces black intros. */
static void fb_overlay(void)
{
    static int stats_enabled = -1;
    static uint32_t stats_source = UINT32_MAX;
    static DWORD stats_start;
    static unsigned stats_frames;
    ptrdiff_t offset;
    volatile uint32_t *regs;
    uint32_t bank, format, pitch, physical, in, out, point, xy, dx, dy, key;
    uint32_t iw, ih, ow, oh, ox, oy, x, y;
    const uint8_t *src;
    s_pvideo_active=0;
    if (!s_rgb || !getenv("RECOMP_PVIDEO_OVERLAY")) return;
    offset = xbox_GetMemoryOffset();
    regs = (volatile uint32_t *)((uintptr_t)0xFD000000u + offset);
    if (!(regs[0x8700u / 4] & 0x11u) || (regs[0x8704u / 4] & 1u)) return;
    bank = (regs[0x8700u / 4] & 1u) ? 0u : 8u;
#define OVERLAY_REG(a) regs[((a) + bank) / 4]
    format = OVERLAY_REG(0x8958u);
    if (((format >> 16) & 3u) != 1u) return;
    pitch = format & 0x1FFFu;
    physical = (OVERLAY_REG(0x8900u) + OVERLAY_REG(0x8920u)) & 0x0FFFFFFFu;
    in = OVERLAY_REG(0x8928u); out = OVERLAY_REG(0x8950u);
    point = OVERLAY_REG(0x8930u); xy = OVERLAY_REG(0x8948u);
    dx = OVERLAY_REG(0x8938u); dy = OVERLAY_REG(0x8940u);
    key = regs[0x8B00u / 4] & 0xFFFFFFu;
#undef OVERLAY_REG
    iw = in & 0x7FFu; ih = (in >> 16) & 0x7FFu;
    ow = out & 0xFFFu; oh = (out >> 16) & 0xFFFu;
    ox = xy & 0xFFFu; oy = (xy >> 16) & 0xFFFu;
    if (!pitch || !iw || !ih || !ow || !oh || pitch < iw * 2u
            || (uint64_t)physical + (uint64_t)pitch * ih > 0x04000000u) return;
    /* This opt-in scanout uses the contiguous DMA window. */
    src = (const uint8_t *)((uintptr_t)(physical | 0x80000000u) + offset);
    /* Movies can rewrite one fixed scanout surface. Sample its contents to
     * measure visible changes; repeated/static frames intentionally do not
     * count. This is an observed update rate, not the encoded frame rate. */
    if (stats_enabled < 0) stats_enabled = getenv("RECOMP_OVERLAY_STATS") != NULL;
    if (stats_enabled) {
        uint32_t fingerprint = physical;
        DWORD now = GetTickCount();
        for (y = 0; y < 32; y++)
            for (x = 0; x < 64; x++)
                fingerprint = (fingerprint ^ src[(size_t)(y * ih / 32) * pitch
                                                   + (x * iw / 64) * 2]) * 16777619u;
        if (!stats_start) stats_start = now;
        if (fingerprint != stats_source) { stats_source = fingerprint; stats_frames++; }
        if (now - stats_start >= 5000) {
            fprintf(stderr, "[PVIDEO] observed content updates: %.1f/s\n",
                    stats_frames * 1000.0 / (now - stats_start));
            stats_start = now; stats_frames = 0;
        }
    }
    for (y = 0; y < oh && oy + y < s_fb_height; y++) {
        uint32_t sy = ((point >> 16) >> 4) + (uint32_t)(((uint64_t)y * dy) >> 20);
        if (sy >= ih) continue;
        for (x = 0; x < ow && ox + x < s_fb_width; x++) {
            uint32_t sx = ((point & 0x7FFFu) >> 4) + (uint32_t)(((uint64_t)x * dx) >> 20);
            uint32_t *pixel = s_rgb + (size_t)(oy + y) * s_fb_width + ox + x;
            const uint8_t *p;
            int c, u, v;
            if (sx >= iw || (sx & ~1u) * 2u + 3u >= pitch) continue;
            if ((format & 0x100000u) && (*pixel & 0xFFFFFFu) != key) continue;
            p = src + (size_t)sy * pitch + (sx & ~1u) * 2u;
            c = (int)p[(sx & 1u) ? 2 : 0] - 16;
            u = (int)p[1] - 128; v = (int)p[3] - 128;
            s_pvideo_active=1;
            *pixel = 0xFF000000u | (overlay_channel((298*c + 409*v + 128) >> 8) << 16)
                | (overlay_channel((298*c - 100*u - 208*v + 128) >> 8) << 8)
                | overlay_channel((298*c + 516*u + 128) >> 8);
        }
    }
}

/* Never wait for a publisher on the window thread. A busy publisher leaves
 * the last coherent background available while input and menus keep updating. */
static void display_present(HWND hwnd, HDC hdc, const BITMAPINFO *bi)
{
    int acquired = acquire_present_snapshot();
    if (acquired) borrow_high_snapshot();
    if (acquired && s_present_idx >= 0 && s_rgb) {
        /* A finished frame, published by the flip. Copied into s_rgb so
         * the dump path and GDI see one consistent image even if the
         * next flip lands mid-blit. */
        LONG idx = s_borrowed_slot >= 0 ? s_borrowed_slot : s_present_idx;
        if (s_present[idx])
            memcpy(s_rgb, s_present[idx],
                   (size_t)s_fb_width * s_fb_height * 4);
        fb_overlay();
    } else if (acquired && s_fb_va && s_fb_pitch && s_rgb) {
        /* No flip yet, or pinned with RECOMP_FB_VA: read guest memory as
         * before, which is also what a title that never flips needs. */
        const uint8_t *src =
            (const uint8_t *)((uintptr_t)s_fb_va + xbox_GetMemoryOffset());
        fb_convert(src, s_fb_pitch / s_fb_width);
        fb_overlay();
    }
    display_draw(hwnd, hdc, bi, acquired);
    if (acquired)
    {
        release_high_snapshot();
        release_present_snapshot();
    }
}

/* Window-only 60 Hz cadence. Drawing consumes the interval's budget rather
 * than adding to a fixed sleep. The timer can remain precise when Windows
 * relaxes the process-wide timer resolution for an occluded window. */
typedef struct WindowRefreshClock
{
    HANDLE timer;
    LONGLONG frequency, deadline, phase;
} WindowRefreshClock;

static void window_refresh_initialize(WindowRefreshClock *clock)
{
    LARGE_INTEGER frequency, now;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&now);
    clock->frequency = frequency.QuadPart;
    clock->deadline = now.QuadPart;
    clock->phase = 0;
    /* HIGH_RESOLUTION is supported by newer Windows versions. Older hosts
     * reject the flag; retain the ordinary waitable-timer fallback there. */
    clock->timer = CreateWaitableTimerExW(NULL, NULL, 0x00000002u, TIMER_ALL_ACCESS);
    if (!clock->timer)
    {
        clock->timer = CreateWaitableTimerW(NULL, FALSE, NULL);
    }
}

static void window_refresh_wait(WindowRefreshClock *clock)
{
    LARGE_INTEGER now, due;
    QueryPerformanceCounter(&now);
    clock->deadline += clock->frequency / 60;
    clock->phase += clock->frequency % 60;
    if (clock->phase >= 60)
    {
        clock->deadline++;
        clock->phase -= 60;
    }
    LONGLONG remaining = clock->deadline - now.QuadPart;
    if (remaining <= 0)
    {
        if (-remaining > clock->frequency / 60)
        {
            clock->deadline = now.QuadPart;
            clock->phase = 0;
        }
        return;
    }
    due.QuadPart = -(remaining * 10000000 / clock->frequency);
    if (!due.QuadPart)
    {
        due.QuadPart = -1;
    }
    if (clock->timer && SetWaitableTimer(clock->timer, &due, 0, NULL, NULL, FALSE))
    {
        WaitForSingleObject(clock->timer, INFINITE);
    }
    else
    {
        Sleep((DWORD)((remaining * 1000 + clock->frequency - 1) / clock->frequency));
    }
}

static DWORD WINAPI fb_thread(LPVOID unused)
{
    HWND hwnd;
    HDC hdc;
    BITMAPINFO bi;
    RECT r;

    (void)unused;

    {
        WNDCLASSA wc;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc   = fb_wndproc;
        wc.hInstance     = GetModuleHandleA(NULL);
        wc.hIcon         = LoadIconA(wc.hInstance, MAKEINTRESOURCEA(DOGGAME_ICON_ID));
        wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
        wc.lpszClassName = "XboxRecompFramebuffer";
        RegisterClassA(&wc);
    }
    display_load();
    r.left = 0; r.top = 0; r.right = (LONG)s_resolutions[s_resolution][0]; r.bottom = (LONG)s_resolutions[s_resolution][1];
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd = CreateWindowExA(0, "XboxRecompFramebuffer", s_debug_draw ? DOGGAME_APP_NAME " [DEBUG]" : DOGGAME_APP_NAME,
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           r.right - r.left, r.bottom - r.top,
                           NULL, NULL, GetModuleHandleA(NULL), NULL);
    if (!hwnd) {
        InterlockedExchange(&s_fb_running, 0);
        return 0;
    }
    /* Match the executable icon at the smaller taskbar/title-bar size. */
    SendMessageA(hwnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadImageA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(DOGGAME_ICON_ID), IMAGE_ICON,
                                   GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
    if(s_borderless){InterlockedExchange(&s_mode_request,1);display_apply(hwnd);}
    hdc = GetDC(hwnd);

    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize        = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth       = (LONG)s_fb_width;
    bi.bmiHeader.biHeight      = -(LONG)s_fb_height;   /* top-down */
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    s_rgb = (uint32_t *)calloc((size_t)s_fb_width * s_fb_height, 4);

    fprintf(stderr, "  [FBWIN] framebuffer window open (%ux%u)\n",
            s_fb_width, s_fb_height);

    WindowRefreshClock refresh_clock;
    window_refresh_initialize(&refresh_clock);
    while (InterlockedCompareExchange(&s_fb_running, 1, 1) == 1) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (InterlockedCompareExchange(&s_fb_running, 1, 1) != 1)
            break;
        window_update_input();
        display_apply(hwnd);
        cursor_update(hwnd);
        display_present(hwnd, hdc, &bi);
        {
            /* One dump a few seconds in, so the title has had time to render
             * something rather than catching the first blank frame.
             *
             * RECOMP_FB_WINDOW_DUMP_EVERY=<frames> dumps repeatedly instead.
             * This window follows the address AvSetDisplayMode gave, which is
             * what the CRTC scans and therefore what a person sees; the
             * pushbuffer executor's own dump follows its draw surface. With
             * double buffering those are different buffers, and measuring
             * progress from the executor's dump reports a blank screen while
             * the window is showing the title's logo. Ask the window. */
            const char *dump = getenv("RECOMP_FB_DUMP");
            const char *every = getenv("RECOMP_FB_WINDOW_DUMP_EVERY");
            static int frames;
            int period = every ? atoi(every) : 0;
            frames++;
            if (dump && period > 0) {
                if (frames % period == 0)
                    xbox_FramebufferDumpBmp(dump);
            } else if (dump && frames == 600) {
                xbox_FramebufferDumpBmp(dump);
            }
        }
        {
            static DWORD t0;
            static LONG f0;
            DWORD now = GetTickCount();
            if (now - t0 >= 1000) {
                LONG f = s_flips;
                wchar_t tb[128];
                _snwprintf(tb, 127, L"%ls%ls | FPS: %.1f | draws: %ld", s_title,
                           s_debug_draw ? L" [DEBUG]" : L"",
                           t0 ? (f - f0) * 1000.0 / (now - t0) : 0.0,
                           (long)s_frame_draws);
                tb[127] = 0;
                SetWindowTextW(hwnd, tb);
                t0 = now; f0 = f;
            }
        }
        window_refresh_wait(&refresh_clock);
    }
    if (refresh_clock.timer)
    {
        CloseHandle(refresh_clock.timer);
    }

    cursor_release();
    release_canvas(&s_base_canvas);
    release_canvas(&s_menu_canvas);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    release_high_snapshot();
    for(unsigned i=0;i<3;i++){free(s_high_present[i]);s_high_present[i]=NULL;s_high_capacity[i]=0;}
    free(s_rgb);
    s_rgb = NULL;
    /* The recompiled guest keeps running on another thread; quitting this
     * message loop alone cannot stop it. Match the host firmware-exit path. */
    fprintf(stderr, "[EXIT] framebuffer window closed\n");
    fflush(stderr);
    ExitProcess(0);
    return 0;
}

void xbox_FramebufferWindowStart(void)
{
    HANDLE th;

    if (!getenv("RECOMP_FB_WINDOW"))
        return;
    if (InterlockedCompareExchange(&s_fb_running, 1, 0) != 0)
        return;
    th = CreateThread(NULL, 0, fb_thread, NULL, 0, NULL);
    if (th)
        CloseHandle(th);
    else
        InterlockedExchange(&s_fb_running, 0);
}

#else
void xbox_DisplayGameplayCursor(int playing) { (void)playing; }
void xbox_MenuOverlayRegister(XboxOverlayDraw d, XboxOverlayKey k, XboxMenuClick c) { (void)d;(void)k;(void)c; }
void xbox_MenuOverlayHoverRegister(XboxMenuHover h) { (void)h; }
void xbox_MenuOverlayCapture(int c) { (void)c; }
unsigned xbox_DisplayResolutionCount(void) { return 1; }
unsigned xbox_DisplayResolutionSelected(void) { return 0; }
void xbox_DisplayResolutionSize(unsigned i,unsigned *w,unsigned *h) { (void)i;*w=640;*h=480; }
void xbox_DisplayResolutionSelect(unsigned i) { (void)i; }
int xbox_DisplayWidescreen(void) { return 0; }
void xbox_DisplayWidescreenSet(int enabled) { (void)enabled; }
int xbox_DisplayMouseInteraction(void) { return 1; }
void xbox_DisplayMouseInteractionSet(int enabled) { (void)enabled; }
int xbox_DisplayFpsCap(void) { return 0; }
void xbox_DisplayFpsCapSet(int enabled) { (void)enabled; }
int xbox_DisplayDynamicPrompts(void) { return 1; }
int xbox_DisplayPauseOnFocusLoss(void) { return 1; }
void xbox_DisplayPauseOnFocusLossSet(int enabled) { (void)enabled; }
int xbox_DisplayConsumeFocusLoss(void) { return 0; }
void xbox_DisplayRequestQuit(void) {}
int xbox_DisplayBorderless(void) { return 0; }
void xbox_DisplayBorderlessSet(int enabled) { (void)enabled; }
int xbox_DisplayInternalScaling(void) { return 0; }
void xbox_DisplayInternalScalingSet(int enabled) { (void)enabled; }
void xbox_DisplayDynamicPromptsSet(int enabled) { (void)enabled; }
void xbox_DebugOverlayUpdateRegister(XboxOverlayUpdate u) { (void)u; }
void xbox_MenuOverlayUpdateRegister(XboxOverlayUpdate u) { (void)u; }
void xbox_DebugOverlayRegister(XboxOverlayDraw d, XboxOverlayKey k) { (void)d;(void)k; }
void xbox_DebugWorldRegister(XboxOverlayDraw d) { (void)d; }
void xbox_DebugWorldFrameRegister(void (*capture)(void)) { (void)capture; }
void xbox_DebugWorldComposite(void *dc,const uint32_t *p,unsigned w,unsigned h) { (void)dc;(void)p;(void)w;(void)h; }
void xbox_DebugOverlayCapture(int c) { (void)c; }
int xbox_DebugOverlayCaptured(void) { return 0; }
void xbox_DebugOverlayGraph(int e) { (void)e; }
void xbox_DebugOverlayLocation(const char *t) { (void)t; }
void xbox_FramebufferWindowSet(uint32_t fb_va, uint32_t pitch) { (void)fb_va; (void)pitch; }
void xbox_FramebufferWindowPresent(uint32_t fb_va, uint32_t pitch) { (void)fb_va; (void)pitch; }
void xbox_FramebufferWindowStart(void) {}
int xbox_FramebufferKeyDown(int vk) { (void)vk; return 0; }
unsigned xbox_FramebufferKeyboardSerial(void) { return 0; }
int xbox_FramebufferFocused(void) { return 0; }
void xbox_FramebufferWindowSetTitle(const uint16_t *n, int m) { (void)n; (void)m; }
void xbox_FramebufferWindowFrameStats(uint32_t draws) { (void)draws; }
#endif
