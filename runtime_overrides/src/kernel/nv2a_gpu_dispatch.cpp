#include "../../include/xbox/renderer_options.h"
#include "nv2a_gpu.h"
#include "nv2a_vulkan.h"

/* Backend choice is immutable after startup. Only this layer knows which
 * implementation is selected; packet decoding continues to use nv_gpu_*. */
static bool use_vulkan;

extern "C"
{
    int nv_d3d11_enabled(void);
    int nv_d3d11_begin(const NvGpuDraw *);
    void nv_d3d11_triangle(const NvGpuVertex *);
    unsigned nv_d3d11_append_vertex(const NvGpuVertex *);
    void nv_d3d11_indexed_triangle(const unsigned *);
    int nv_d3d11_indexed_upload_enabled(void);
    void nv_d3d11_end(void);
    void nv_d3d11_sync_reason(const char *);
    void nv_d3d11_sync_depth(const char *);
    void nv_d3d11_prepare_depth_change(const uint8_t *);
    void nv_d3d11_discard_overwritten(const uint8_t *, unsigned, unsigned, unsigned, unsigned, unsigned,
                                      unsigned, const float *);
    int nv_d3d11_clear_attachments(const NvGpuClear *);
    const uint32_t *nv_d3d11_present_image(const uint8_t *, unsigned, unsigned *, unsigned *);
    void nv_d3d11_report(void);
    void nv_d3d11_texture_barrier(const uint8_t *, uint64_t);
    void nv_d3d11_shutdown(void);
}

int xbox_SelectExperimentalVulkan(char *error, unsigned capacity)
{
    if (!nv_vk_initialize(error, capacity))
    {
        return 0;
    }
    use_vulkan = true;
    return 1;
}

int nv_gpu_enabled(void)
{
    return use_vulkan || nv_d3d11_enabled();
}

int nv_gpu_begin(const NvGpuDraw *state)
{
    return use_vulkan ? nv_vk_begin(state) : nv_d3d11_begin(state);
}

unsigned nv_gpu_append_vertex(const NvGpuVertex *vertex)
{
    return use_vulkan ? nv_vk_append_vertex(vertex) : nv_d3d11_append_vertex(vertex);
}

int nv_gpu_indexed_upload_enabled(void)
{
    return use_vulkan || nv_d3d11_indexed_upload_enabled();
}

void nv_gpu_triangle(const NvGpuVertex *vertices)
{
    if (use_vulkan)
    {
        nv_vk_triangle(vertices);
    }
    else
    {
        nv_d3d11_triangle(vertices);
    }
}

void nv_gpu_indexed_triangle(const unsigned indices[3])
{
    if (use_vulkan)
    {
        nv_vk_indexed_triangle(indices);
    }
    else
    {
        nv_d3d11_indexed_triangle(indices);
    }
}

void nv_gpu_end(void)
{
    if (use_vulkan)
    {
        nv_vk_end();
    }
    else
    {
        nv_d3d11_end();
    }
}

void nv_gpu_sync(void)
{
    nv_gpu_sync_reason("explicit boundary");
}

void nv_gpu_sync_reason(const char *reason)
{
    if (use_vulkan)
    {
        nv_vk_sync_reason(reason);
    }
    else
    {
        nv_d3d11_sync_reason(reason);
    }
}

void nv_gpu_sync_depth(const char *reason)
{
    if (use_vulkan)
    {
        nv_vk_sync_reason(reason);
    }
    else
    {
        nv_d3d11_sync_depth(reason);
    }
}

void nv_gpu_prepare_depth_change(const uint8_t *color)
{
    if (use_vulkan)
    {
        nv_vk_sync_reason("depth attachment change");
    }
    else
    {
        nv_d3d11_prepare_depth_change(color);
    }
}

void nv_gpu_discard_overwritten(const uint8_t *color, unsigned pitch, unsigned format, unsigned x, unsigned y,
                                unsigned width, unsigned height, const float *depth)
{
    /* Unsupported/partial clears retain conservative publication. Full clears
     * can instead be mirrored by nv_gpu_clear_attachments below. */
    if (use_vulkan)
    {
        nv_vk_sync_reason("unsupported CPU clear");
        nv_vk_discard_snapshots(color, depth);
    }
    else
    {
        nv_d3d11_discard_overwritten(color, pitch, format, x, y, width, height, depth);
    }
}

int nv_gpu_clear_attachments(const NvGpuClear *clear)
{
    return use_vulkan ? nv_vk_clear_attachments(clear) : nv_d3d11_clear_attachments(clear);
}

const uint32_t *nv_gpu_present_image(const uint8_t *guest, unsigned pitch, unsigned *width, unsigned *height)
{
    return use_vulkan ? nv_vk_present_image(guest, pitch, width, height)
                      : nv_d3d11_present_image(guest, pitch, width, height);
}

void nv_gpu_texture_barrier(const uint8_t *data, uint64_t bytes)
{
    if (use_vulkan)
    {
        nv_vk_texture_barrier(data, bytes);
    }
    else
    {
        nv_d3d11_texture_barrier(data, bytes);
    }
}

void nv_gpu_report(void)
{
    if (use_vulkan)
    {
        nv_vk_report();
    }
    else
    {
        nv_d3d11_report();
    }
}

void nv_gpu_shutdown(void)
{
    if (use_vulkan)
    {
        nv_vk_shutdown();
    }
    else
    {
        nv_d3d11_shutdown();
    }
}
