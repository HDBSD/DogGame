#ifndef NV2A_GPU_H
#define NV2A_GPU_H
#include <stdint.h>
#include "nv2a_combiner.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {float pos[4], color[4], secondary[4], tex[4], extra[3][4], fog;} NvGpuVertex;
typedef int (*NvGpuDecode)(void *, unsigned, unsigned, uint32_t *);
/* Pure layout decoder: the result must depend only on format, x and y.
 * Residency caches decoded offsets while that format/layout is retained. */
typedef uint64_t (*NvGpuSurfaceOffset)(unsigned, unsigned, unsigned);
typedef struct {
    unsigned valid,width,height,addr_u,addr_v,linear;
    uint64_t key,hash;NvGpuDecode decode;void *arg;
    const uint8_t *guest;unsigned format,pitch;
} NvGpuTexture;
typedef struct {
    uint8_t *color; float *depth;
    unsigned width,height,pitch,depth_stride,clip_x,clip_y,clip_w,clip_h;
    unsigned depth_test,depth_write,depth_func,blend,src,dst,equation,mask;
    unsigned alpha_test,alpha_func,alpha_ref,mode;
    float zscale,fog_color[4];
    Nv2aCombiner combiner;
    unsigned textured,tw,th,addr_u,addr_v,linear;
    uint64_t texture_key,texture_hash;
    NvGpuDecode decode;void *decode_arg;
    const uint8_t *texture_guest;unsigned texture_format,texture_pitch;
    NvGpuTexture extra[3];
    unsigned clip_plane_mode;
    /* Optional non-linear guest surface layout; offsets are in bytes. */
    unsigned surface_format;
    NvGpuSurfaceOffset surface_offset;
    /* Host depth-slot lifetime, not a guest address. Zero supports callers
     * whose depth allocation is stable for the backend's whole lifetime. */
    uint64_t depth_generation;
} NvGpuDraw;
/* Before reassigning a host depth slot, discard its previous GPU ownership.
 * Title thread only; the caller then initializes the replacement CPU buffer. */
void nv_gpu_depth_slot_reused(const float *depth);
int nv_gpu_enabled(void);
int nv_gpu_begin(const NvGpuDraw *);
void nv_gpu_triangle(const NvGpuVertex *);
/* Title thread, between successful begin/end. Vertices are copied; returned
 * indices belong only to this batch. Emit only valid indices from this batch.
 * Preserve original triangle/provoking-vertex order. No guest memory access. */
unsigned nv_gpu_append_vertex(const NvGpuVertex *vertex);
void nv_gpu_indexed_triangle(const unsigned indices[3]);
int nv_gpu_indexed_upload_enabled(void);
void nv_gpu_end(void);
void nv_gpu_sync(void);
void nv_gpu_sync_reason(const char *reason);
/* Publish color for CPU texture decoding/presentation without requesting a
 * depth read. Vulkan retains authoritative depth until an explicit consumer. */
void nv_gpu_sync_color_reason(const char *reason);
void nv_gpu_sync_depth(const char *reason);
void nv_gpu_prepare_depth_change(const uint8_t *next_color);
void nv_gpu_discard_overwritten(const uint8_t *color, unsigned pitch, unsigned format, unsigned x, unsigned y, unsigned width, unsigned height, const float *depth);
/* Prepare an immediate CPU clear. Returns 1 when the caller can omit its
 * additional synchronization; does not omit the CPU writes themselves. */
int nv_gpu_prepare_cpu_clear(const uint8_t *color, unsigned pitch, unsigned format,
                             unsigned x, unsigned y, unsigned width, unsigned height, const float *depth);
typedef struct
{
    const uint8_t *color;
    const float *depth;
    unsigned pitch, format, x, y, width, height;
    uint32_t color_value;
    float depth_value;
} NvGpuClear;

/* Title/render thread only. Returns 1 when complete attachment clears were
 * issued on the current GPU target. The caller must immediately perform the
 * matching CPU clears without another sync. Unrelated dirty attachments stay
 * GPU-owned. Returns 0 without side effects for unsupported/partial clears. */
int nv_gpu_clear_attachments(const NvGpuClear *clear);
const uint32_t *nv_gpu_present_image(const uint8_t *, unsigned, unsigned *, unsigned *);
void nv_gpu_report(void);
void nv_gpu_texture_barrier(const uint8_t *, uint64_t);
void nv_gpu_shutdown(void);
#ifdef __cplusplus
}
#endif
#endif
