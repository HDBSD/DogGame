#ifndef TITLE_RENDERER_SETTINGS_H
#define TITLE_RENDERER_SETTINGS_H

/* Startup and window-thread host policy; no guest access or device mutation.
 * Preferences live beside the executable in display.ini. Missing/invalid
 * preferences select Vulkan. Saving returns zero on failure. */
int title_renderer_settings_prefer_vulkan(void);
int title_renderer_settings_save(int vulkan);
#endif
