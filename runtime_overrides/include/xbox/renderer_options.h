#ifndef XBOX_RENDERER_OPTIONS_H
#define XBOX_RENDERER_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* Startup/title thread only, before any draws. Selection is process-lifetime.
     * Explicit debug opt-in only: no environment variable selects Vulkan.
     * Returns zero with an error on unavailable loader/device; never silently
     * substitutes D3D11 for an explicitly requested Vulkan backend. */
    int xbox_SelectExperimentalVulkan(char *error, unsigned capacity);

#ifdef __cplusplus
}
#endif
#endif
