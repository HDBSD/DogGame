#ifndef XBOX_DEBUG_OVERLAY_H
#define XBOX_DEBUG_OVERLAY_H
#include <stdint.h>
/* Register before opening the framebuffer window. All UI callbacks run on
 * the window thread; queue guest actions instead of executing them there. */
typedef void (*XboxOverlayDraw)(void *dc, unsigned width, unsigned height);
typedef int (*XboxOverlayKey)(unsigned key);
void xbox_DebugOverlayRegister(XboxOverlayDraw draw, XboxOverlayKey key);
/* World diagnostics draw before menus. Pixels are ARGB, composed against the
 * displayed framebuffer without modifying it; callable only on the UI thread. */
void xbox_DebugWorldRegister(XboxOverlayDraw draw);
void xbox_DebugWorldFrameRegister(void (*capture)(void));
void xbox_DebugWorldComposite(void *dc, const uint32_t *argb, unsigned width, unsigned height);
void xbox_DebugOverlayCapture(int capture);
int xbox_DebugOverlayCaptured(void);
void xbox_DebugOverlayGraph(int enabled);
void xbox_DebugOverlayLocation(const char *text);
/* One short Start press through the regular player-one input backend. */
void xbox_InputPulseStart(void);
void nv2a_debug_trace_request(void);
void nv2a_debug_profile_request(void);
void nv2a_debug_trace_seconds_request(unsigned seconds);
int xbox_FramebufferFocused(void);
#endif
