#ifndef NV2A_VULKAN_H
#define NV2A_VULKAN_H
#include "nv2a_gpu.h"

/* All calls belong to the title thread. begin copies state, but guest surface
 * and texture sources must remain valid until end. end copies vertex/texture
 * data into backend-owned storage. A guest batch end may retain an open Vulkan
 * render pass; backend transfer/clear/submit helpers close it before recording
 * operations that require being outside the pass. sync publishes dirty attachments
 * into guest memory before CPU consumers run. Color-only publication preserves
 * GPU depth ownership across target changes; explicit sync includes cached depth.
 * A depth_generation identifies each host slot assignment, never a guest ABI.
 * No window-thread guest access.
 * Unsupported draws return zero before recording geometry so the executor
 * can synchronize and use its existing software path. */
int nv_vk_initialize(char *error, unsigned capacity);
int nv_vk_begin(const NvGpuDraw *state);
unsigned nv_vk_append_vertex(const NvGpuVertex *vertex);
void nv_vk_indexed_triangle(const unsigned indices[3]);
void nv_vk_triangle(const NvGpuVertex *vertices);
void nv_vk_end(void);
void nv_vk_sync(void);
void nv_vk_sync_reason(const char *reason);
/* Color consumer only: finish color work, retaining GPU-owned depth. The
 * diagnostic reason labels the operation; it does not select ownership. */
void nv_vk_sync_color_reason(const char *reason);
void nv_vk_depth_slot_reused(const float *depth);
/* Title thread after synchronization. Borrowed CPU snapshot valid until the
 * next backend mutation. Returns null if guest CPU edits invalidate it; the
 * existing framebuffer publisher copies it under its own snapshot lock. */
const uint32_t *nv_vk_present_image(const uint8_t *guest, unsigned pitch, unsigned *width, unsigned *height);
/* After synchronization, invalidate physical history before a CPU clear,
 * including writes whose native values happen to equal the old snapshot. */
void nv_vk_discard_snapshots(const uint8_t *color, const float *depth);
/* Title thread immediately before the fallback CPU clear. Unrelated validated
 * linear clears retain pending work; otherwise finish it, publishing only
 * attachments the clear will not completely overwrite,
 * then invalidate affected snapshots. Depth denotes the executor's complete
 * host depth-buffer clear; color bounds must cover the complete active target.
 * The caller must perform the promised CPU writes before another GPU call. */
void nv_vk_prepare_cpu_clear(const uint8_t *color, unsigned pitch, unsigned format,
                             unsigned x, unsigned y, unsigned width, unsigned height,
                             const float *depth);
/* Same immediate CPU-mirror contract as nv_gpu_clear_attachments. Unsupported
 * and partial requests return zero without changing GPU state. */
int nv_vk_clear_attachments(const NvGpuClear *clear);
void nv_vk_texture_barrier(const uint8_t *data, uint64_t bytes);
void nv_vk_report(void);
void nv_vk_shutdown(void);
#endif
