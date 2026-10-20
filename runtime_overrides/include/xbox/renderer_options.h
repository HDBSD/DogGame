#ifndef XBOX_RENDERER_OPTIONS_H
#define XBOX_RENDERER_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* Startup/title thread only, before any draws. Selection is process-lifetime.
     * The title selects its saved preference or command-line override here.
     * Returns zero with an error on unavailable loader/device; never silently
     * substitutes D3D11 for an explicitly requested Vulkan backend. */
    int xbox_SelectExperimentalVulkan(char *error, unsigned capacity);
    /* Immutable after startup; safe for display settings queries. */
    int xbox_ExperimentalVulkanSelected(void);

#ifdef __cplusplus
}
#endif
#endif
