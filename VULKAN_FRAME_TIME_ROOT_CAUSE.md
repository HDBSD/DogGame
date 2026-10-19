# Vulkan frame-time root cause and proposed correction

## Finding

The renderer treats GPU attachment changes as CPU readback dependencies. This
serializes the title thread with the GPU several times inside one frame, then
converts and copies the completed attachments into CPU buffers. Retaining GPU
images has not removed that CPU ownership requirement. The latest capture
confirms this is a substantial recurring frame cost; it does not measure pure
GPU raster time or establish that this is the only cause of every slow frame.

The preceding clear optimization did not fix the reported slowdown. Work moved
from the clear boundary to the next depth-attachment boundary. The disappearance
of one named wait category was not evidence of a frame-time improvement.

## Evidence from the supplied capture

Source: `build/vulkan-speed/user-drop-4/debug-render.log`, copied from the
launcher installation before further runs. The ten-second profile begins with
presentation count 750 and ends with 1022. These are cumulative counter deltas,
not totals over the complete session. Internal resolution is selected as 4K.

| Measurement | Capture delta | Per rendered frame |
| --- | ---: | ---: |
| Wall time / rendered flips | 10,016 ms / 272 | 36.82 ms, 27.16 FPS |
| Raster-path elapsed CPU time, including waits | 8,217.15 ms | 30.21 ms |
| Bridge elapsed time, nested within raster | 6,819.11 ms | 25.07 ms |
| Vertex transforms, within raster | 908.54 ms | 3.34 ms |
| Geometry/fallback, within raster | 376.43 ms | 1.38 ms |
| All backend fence waits | 2,551.3 ms | 9.38 ms |
| Depth-change fence waits | 2,076.0 ms | 7.63 ms |
| CPU attachment publication | 1,302.3 ms | 4.79 ms |
| Depth-change synchronization calls | 2,176 | 8.00 |
| Depth-change color readback | 1,498.92 MiB | 5.51 MiB |
| Depth-change depth readback | 7,873.92 MiB | 28.95 MiB |

The depth-change path accounts for 81.4% of measured fence-wait time. Across
all reasons there are 3,264 synchronization calls: twelve per frame, including
one presentation, eight depth changes, one nonlinear texture dependency and
two render-target texture dependencies. This is approximately 326 synchronous
submissions per second during this capture, not one asynchronous frame submit.

Fence wait and CPU publication are separate timed scopes. Their combined cost
is 14.17 ms per frame. They must NOT be added to the bridge or raster totals:
those enclosing timers already include this work. The backend preparation
timer also contains nested synchronization; it is not an additional cost.
The `bridge/hash` label does not isolate hashing and cannot establish a hash
bottleneck. The 30 FPS budget is 33.33 ms; this capture exceeds it by 3.49 ms
per frame on average. These averages do not explain every individual spike.

## Exact dependency chain

1. `gpu_begin_batch` in `runtime_overrides/src/kernel/nv2a_pb_exec.c` compares
   `previous_zeta` with the selected guest zeta offset. A change calls
   `nv_gpu_prepare_depth_change` before selecting the host depth buffer.
2. The Vulkan branch in `nv2a_gpu_dispatch.cpp` unconditionally calls
   `nv_vk_sync_reason("depth attachment change")`. It does not determine
   whether the CPU actually needs the previous depth values. In particular,
   the name does not restrict this operation to depth publication.
3. `nv_vk_sync_reason` ends the render pass, retains scaled depth history,
   records readback of dirty attachments, submits, and calls
   `vkWaitForFences(..., UINT64_MAX)` on the title thread.
4. `publish_attachments` converts selected physical depth samples to the
   native host float buffer, copies color to guest RAM, and updates CPU
   snapshots. Only after that does title execution resume.
5. `ensure_target` reuses or reloads attachments according to CPU snapshot
   equality. GPU images therefore remain subordinate to CPU copies rather
   than serving as the authoritative render resources.

This chain is established by maintained source and runtime counters, not a
new Ghidra recovery. It explains why the last optimization moved the wait:
the next attachment change still demands the same publication.

Two related paths enforce the same design. `nv_vk_texture_barrier` publishes
a rendered surface to RAM so the CPU texture decoder/hash path can consume
it, instead of binding the rendered GPU image as a texture. Presentation
reads the physical color image to CPU storage; `fb_present.c` then presents
copied pixels with `StretchDIBits`. Neither is a Vulkan swapchain path.

## Why deleting the wait is unsafe

`zbuf_current` uses four reusable 1024x1024 host float buffers. A host pointer
can be reassigned to another guest zeta offset and initialized immediately.
It is not a stable GPU resource identity. The current synchronization protects
those mutations and software rendering consumers. Removing it without changing
ownership can publish old GPU results into a reused buffer or load stale depth.

Texture decoding genuinely reads RAM under the current API. Suppressing its
barrier without supplying a GPU image instead can corrupt effects. Likewise,
CPU edits, software fallbacks and diagnostics still require explicit ownership
transfers. Larger caches or narrower readbacks alone do not remove this chain.

## Proposed fix

Implement persistent Vulkan surface ownership first, starting with depth. This
is a runtime adapter change; generated guest source stays untouched. No title
routine replacement is required by the evidence currently available.

1. Give depth resources a stable identity derived from guest zeta allocation
   and its lifetime generation, plus dimensions, format and depth scale.
   Track any host slot reassignment explicitly. Separate depth-image lifetime
   from the active color-target/framebuffer cache and from host float pointers.
2. Track whether the GPU or CPU owns the latest contents. Switching attachments
   ends the pass and records the required GPU barriers, but does not publish
   to RAM or wait for the GPU. Use submission completion tracking to retire
   resources, descriptors and upload storage only after their last use.
3. Route depth clears and software depth access through that ownership API.
   Clear GPU depth directly; update or invalidate the CPU mirror explicitly.
   Read back only when a real software consumer requires current values.
   Slot eviction must not silently destroy the authoritative GPU version.
4. Resolve rendered-texture references to resident GPU color images and use
   image barriers or GPU copies for feedback/aliasing. Keep the CPU decoder
   path for unsupported layouts and actual CPU consumers. Do not substitute
   guessed allocation spans for unresolved nonlinear layouts.
5. Present through a Vulkan swapchain, with bounded frames in flight and
   explicit synchronization. Keep native-resolution CPU publication only
   where guest code needs it; screenshots can request an explicit readback.
   Port the existing menu/overlay composition and resize handling as part of
   that change. This removes the full-resolution GPU-to-CPU-to-window path.

Depth ownership is the first priority because it is the largest measured wait
category. Direct image sampling and swapchain presentation remove the other
recurring ownership transfers. This is a renderer ownership correction, not
a promise that switching graphics API alone makes the game faster.

## Validation required before claiming a fix

- Add GPU timestamp queries around raster, attachment copies and resolves,
  plus non-overlapping CPU timers for hashing, decoding, publication and
  frame work outside rendering. Fence latency alone is not GPU execution time;
  no GPU timestamp query instrumentation exists in the current backend.
- Replay the same interactive E006 route and settings on baseline and corrected
  builds. A stationary, audio-disabled smoke run is not an equivalent workload.
  Preserve raw counters and compare complete frame time, not renamed categories.
- Confirm GPU-only attachment switches cause zero host readback and zero host
  fence waits. Any remaining readback must identify its actual CPU consumer.
- Exercise more than four alternating zeta allocations, slot reuse, overlapping
  color/depth lifetimes, partial clears, software fallback, CPU texture edits,
  render-target feedback, resolution changes, resize and resource retirement.
  Compare native depth/color samples and subpixel occlusion with the baseline.
- Establish uncapped rendering headroom below the 33.33 ms budget, then verify
  capped 30 FPS through the interactive route, including frame-time percentiles.
  GPU timestamps must establish whether 4K raster work itself fits the budget
  on the target hardware. Do not assert a universal hardware-independent lock.

## Confidence and remaining limits

Confirmed: the repeated CPU/GPU serialization, the source that requests it,
its measured cost, and the fact that the previous change did not remove the
underlying dependency. Confirmed: a depth change alone is treated as a full
CPU publication boundary by this adapter.

Not yet established: how much of each fence wait is raster versus transfer or
driver scheduling; isolated texture-hash cost; the exact contribution of game
simulation, audio and pacing to each individual slow frame; and the corrected
renderer's achieved frame rate on the supplied interactive route. The analysis
above preceded implementation. The following section records the implemented
depth correction and its measured limits.

## Implemented depth ownership correction

Vulkan now distinguishes color publication from an explicit CPU depth consumer
using separate APIs, rather than inferring ownership from a diagnostic string.
The dispatch no longer forces publication on every zeta change. Cached target
pairs retain pending depth, allocation generations and ordered content revisions.
Compatible bindings copy the newest physical depth on the GPU. A true software
consumer, incompatible depth interpretation or bounded-pool eviction requests
native publication; superseded versions never overwrite the newest CPU data.

`zbuf_current` notifies Vulkan before slot reassignment and increments the host
generation. The old images stay alive through their existing fences, but cannot
publish into the replacement slot. CPU writes and mirrored clears invalidate
affected versions, including clears whose native values are unchanged. Color
revisions and GPU copies preserve physical subpixel color when one color surface
is paired with different depth allocations. Snapshot updates cover only the
attachments actually published or mirrored; a color-only boundary cannot hide
a CPU depth edit by refreshing its validation snapshot.

This is a partial ownership correction with a measurable benefit, not the full
GPU-resident renderer proposed above. Color texture decoding and GDI presentation
still use CPU publication. Cache eviction and incompatible layouts can still
materialize depth. Existing color fences continue to protect resource retirement.
DirectX keeps its previous synchronization policy. Generated sources are untouched.

The real-driver fixture verifies deferred depth, shared depth across color
targets, explicit consumers while another target is active, slot reuse before
submission, mirrored clears, changed layouts, ten outstanding allocations with
bounded eviction, CPU edits, and 4K subpixel color across alternate depth binds.
Existing Vulkan and DirectX raster checks pass, as do portable Clang and MSVC
game builds. This does not replace interactive gameplay or visual validation.

Separate 35-second MSVC E006 runs used internal/output 3840x2160, audio disabled,
focus pausing disabled and the renderer cap disabled. The title still paces
itself near 30 FPS. No GPU regression tests or other owned probes ran during
these final measurement runs. These are stationary smoke runs, not deterministic
replays of the supplied route. The last complete reporting windows show:

| Measurement per frame | Previous build | Corrected build |
| --- | ---: | ---: |
| Raster-path elapsed CPU time, including waits | 24.89 ms | 21.83 ms |
| GPU fence waiting | 4.84 ms | 3.15 ms |
| CPU publication | 4.81 ms | 3.19 ms |
| All depth readbacks | 21.83 MiB | 0.51 MiB |
| Publication cadence mean | 33.895 ms | 33.390 ms |
| Publication cadence p99 | 48.388 ms | 50.997 ms |
| Publication cadence maximum | 58.287 ms | 52.027 ms |

The corrected run's last seven one-second captions all reported 30 FPS. Tail
latency is still imperfect; the p99 did not improve in this pair of runs. The
supported conclusion is about 3.06 ms less measured raster-path CPU time and
97.7% less depth readback in this workload, not a demonstrated universal lock.
Raw artifacts are under `build/vulkan-speed/4k-depth-owner-before/` and
`build/vulkan-speed/4k-depth-owner-final/`; derived values are in
`build/vulkan-speed/depth-ownership-summary.json`. The deployed preview build
uses the portable compiler and retains preview locking.
