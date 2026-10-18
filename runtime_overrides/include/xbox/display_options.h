#ifndef XBOX_DISPLAY_OPTIONS_H
#define XBOX_DISPLAY_OPTIONS_H
#include "debug_overlay.h"
typedef int (*XboxMenuClick)(int x, int y);
typedef void (*XboxMenuHover)(int x, int y);
/* UI coordinates remain in the guest's 640x480 space after window resizing. */
void xbox_MenuOverlayRegister(XboxOverlayDraw draw, XboxOverlayKey key, XboxMenuClick click);
void xbox_MenuOverlayHoverRegister(XboxMenuHover hover);
void xbox_MenuOverlayCapture(int capture);
unsigned xbox_DisplayResolutionCount(void);
unsigned xbox_DisplayResolutionSelected(void);
void xbox_DisplayResolutionSize(unsigned index, unsigned *width, unsigned *height);
void xbox_DisplayResolutionSelect(unsigned index);
int xbox_DisplayWidescreen(void);
void xbox_DisplayWidescreenSet(int enabled);
int xbox_DisplayMouseInteraction(void);
void xbox_DisplayMouseInteractionSet(int enabled);
int xbox_DisplayFpsCap(void);
void xbox_DisplayFpsCapSet(int enabled);
int xbox_DisplayDynamicPrompts(void);
void xbox_DisplayDynamicPromptsSet(int enabled);
/* Atomic host policy. Persisted alongside the other Recomp Settings.
 * The title thread consumes focus-loss requests and decides whether to pause;
 * window callbacks never read or mutate guest state. Disabling clears pending
 * requests. Regaining focus does not resume the game. */
int xbox_DisplayPauseOnFocusLoss(void);
void xbox_DisplayPauseOnFocusLossSet(int enabled);
int xbox_DisplayConsumeFocusLoss(void);
/* Request client-area cursor confinement and hiding during gameplay.
 * The display thread applies it only while the game window is active. */
void xbox_DisplayGameplayCursor(int playing);
void xbox_DisplayRequestQuit(void);
int xbox_DisplayBorderless(void);
void xbox_DisplayBorderlessSet(int enabled);
int xbox_DisplayInternalScaling(void);
void xbox_DisplayInternalScalingSet(int enabled);
#endif
