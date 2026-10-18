# Experimental Vulkan renderer

Launch from the installed game's directory:

```powershell
.\DogGame.exe --debug --vulkan
# Optional direct gameplay probe:
.\DogGame.exe --debug --vulkan --level H001
```

`--vulkan` requires `--debug`, in either argument order. Ordinary launches and
debug launches without this flag retain the existing D3D11 backend. No saved
setting or environment variable selects Vulkan. Preview authentication still
applies when the executable was built with preview locking.

The graphics driver must provide Vulkan. Users need no Vulkan SDK or shader
compiler. Missing loader/device support produces a startup error rather than
quietly using D3D11. Debug output identifies the selected GPU with
`[GPU-VULKAN]`; its counters report submitted batches, triangles, readback
boundaries, rejected batches, pipeline count and texture reuse.
`[GPU-VULKAN-MEMORY]` identifies upload/readback memory flags, and
`[GPU-VULKAN-CPU]` records cumulative preparation, command recording, queue
submission, fence wait and guest publication time. Preparation/recording can
include nested flushes, so those counters must not be added as disjoint totals.

## Implementation and ownership

`nv2a_gpu_dispatch.cpp` retains the packet executor's public `nv_gpu_*` boundary
and chooses the implementation once at startup. `nv2a_gpu.cpp` remains the
reference D3D11 implementation, with private symbol names.

`nv2a_vulkan.cpp` owns a graphics queue, command buffer, fence, selectable-resolution
BGRA8/D32 attachments, bounded upload arena and texture/pipeline caches. All
calls and guest memory access belong to the title thread. Vertex, constant and
texture uploads remain alive until the submission fence completes. The window
continues consuming copied pixels; it never reads Vulkan-owned guest state.

Uploads use coherent host-visible storage. Readbacks have a separate allocation
that prefers host-cached memory, with explicit post-fence invalidation when it
is noncoherent. If the driver offers no cached memory, a bulk copy into an owned
CPU snapshot precedes scalar depth/layout processing. Linear color publication
uses row copies; nonlinear surfaces retain their layout callback.

The backend batches commands until existing synchronization boundaries or an
allocation budget require publication. At CPU access, target switches, clears
and presentation, pending color/depth writes are copied back before the guest
consumer runs. Texture overlap barriers publish pending render-target writes
before CPU texture decoding. Nonlinear targets synchronize conservatively.
Texture caching uses the existing executor's identity/content hash and sampling
state; anonymous API textures are recopied. An updated hash replaces the lookup
entry for that identity; its old image remains alive until the submission fence
completes. Per-draw image snapshots keep multiple stages bound to their exact
versions. Static textures survive repeated animated updates. This inherits the
executor's content-hash guarantees.

Completed command chunks are submitted every 32 draws so the GPU can execute
while the title thread prepares later draws. Upload offsets, descriptors and
images stay alive until the final same-queue fence; only then are pools reset
and retired textures released. Each segment binds its own complete draw state.
The fence's coverage of prior submissions follows the
[Vulkan fence synchronization rules](https://docs.vulkan.org/spec/latest/chapters/synchronization.html#synchronization-fences-signaling).

Compatible batches now share a render pass. Fixed-function color/depth ordering
is retained within that pass, and every draw still binds its own pipeline,
viewport, scissor, vertices and descriptors. Texture uploads, attachment clears,
target switches, early submissions and guest publication close the pass before
recording outside-pass commands. Cached textures need neither an upload nor a
pass break. This removes per-draw whole-pipeline barriers while retaining
conservative barriers at transfer/submission/CPU visibility boundaries. The
change follows the separation of attachment and shader stages described in
[Khronos synchronization examples](https://docs.vulkan.org/guide/latest/synchronization_examples.html).

Supported full attachment clears are recorded directly on the current GPU
target and immediately mirrored by the executor's CPU clear. Only overwritten
dirty attachments are discarded. The next batch checks the mirrored values;
subsequent CPU edits trigger publication of unrelated writes and a reload.
Partial/mismatched color clears and invalid values retain the conservative path.

Unsupported batches return to the existing software renderer before accepting
geometry. Failure after geometry acceptance terminates with a reported error;
there is no silent dropped draw or automatic backend switch.

## Shaders and provenance

This is a host backend implementation against the maintained `NvGpuDraw`
contract. It does not recover or replace the game's high-level renderer, and
does not edit generated `src/recomp/` files or guest call/stack contracts.

The shader generator extracts the existing D3D11 HLSL, adjusts Vulkan's viewport
Y convention and assigns separate descriptor bindings. A specialization
constant selects simple material modes at pipeline creation so those pipelines
can remove the general combiner interpreter. Mode zero retains that interpreter.
The embedded header includes a source digest checked by the options test.

Regeneration is an explicit developer action, not part of setup/normal builds:

```powershell
python scripts/compile_vulkan_shaders.py --compiler path/to/glslang.exe
```

The initial artifact used Khronos glslang 16.6.0, downloaded from its official
`main-tot` Windows x86-64 release. The local archive SHA256 was
`520099102c8605488d0352365d363aef1423a49f04eb5ef3d3f88eed7bea723b`.
That release URL is mutable; the checksum identifies the tool actually used,
not a guarantee that a future download has identical contents.
Vulkan header origin/license is recorded beside the vendored declarations;
the license is copied beside built executables.

## Performance follow-up: 18 October 2026

The first implementation used the same mapped allocation for uploads and
readbacks. On the test RTX 5090, its selected memory flags were `0x6`
(host-visible/coherent, uncached). Timing a private H001 startup probe showed
7,859 ms of CPU publication versus 179 ms of fence waiting in the first report;
the next cumulative report reached 16,748 ms publication for 587 readbacks.
Reading uncached mapped memory one pixel at a time was the dominant measured
cost, rather than GPU execution waiting.

Separating readback storage selected flags `0xE` (host-cached as well). The
repeat probe recorded 418.5 ms of publication for 4,565 readbacks: approximately
0.092 ms per boundary versus 28.5 ms before. Subsequent texture replacement
reduced preparation work and unnecessary re-uploading of static contents.

Sequential 22-second private H001 runs sampled the hidden framebuffer window's
own FPS caption. Samples at seconds 16–22 were 3–4 FPS for the baseline,
26–28 FPS with cached readbacks, and 29–30 FPS with both changes. Original
timing logs and sampled captions are under `build/vulkan-speed/{baseline,cached,optimized}`.
These runs used portable Release builds, `--debug --vulkan --level H001 --noaudio`,
no scripted movement/save actions, and the same local machine. Captured H001
output was inspected. This is a scene-specific result, not a guarantee of
performance across hardware, levels, FMVs or extended gameplay.

## Validation and current limits

### Submission overlap and clear follow-up

The supplied gameplay log's final interval spent 1,720.8 ms in fence waits over
10,022.8 ms wall time. This is significant, but excludes other CPU costs and is
not a measurement of pure GPU execution time.

Per-reason `[GPU-VULKAN-SYNC]` counters now record submission counts, fence wait
time and copied attachment bytes. An H001 run with mirrored clears recorded
557 accelerated depth clears and 3,319.9 ms of waits over 558 presentations.
The largest remaining reason was the conservative clear path for another target:
1,842.3 ms across 557 calls. Mirrored clears alone removed a boundary per frame
without establishing a reduction in total wait time.

Submitting completed chunks earlier reduced the corresponding total to
1,483.8 ms across 564 presentations: 2.63 ms per presentation versus 5.95 ms,
about 56% less measured CPU waiting. The conservative-clear wait category fell
to 263.7 ms over 563 calls. Extra queue-submission CPU time increased from
196.1 ms to 238.4 ms in these reporting windows. These are sequential private
H001 startup probes on the same machine, not a controlled GPU benchmark or a
guarantee on other devices/scenes. Window FPS remained 29–30 and captured
output was inspected. Logs/captions are in `build/vulkan-speed/clears` and
`build/vulkan-speed/early-submit`.

The fixture now verifies no host fence wait or guest publication occurs during
80 draws spanning multiple early submissions, followed by correct final pixels.
It also checks clear ordering, retention of unrelated dirty attachments, CPU
edits after a mirrored clear, and rejection of partial/invalid clears. Repeated
texture replacement and descriptor recycling tests span multiple submissions.

### Broader validation

Render-pass grouping follow-up: the same private H001 probe recorded 9,068
passes for 146,618 draws (previously one pass per draw). Fence waits fell from
1,483.8 ms across 564 presentations to 1,075.5 ms across 563 presentations:
2.63 to 1.91 ms per presentation, about another 27% reduction. Command recording
time fell from 602.9 to 395.2 ms in those reporting windows. Window FPS remained
30 at seconds 16–22, and the captured output was inspected. These are sequential
scene-specific measurements with ordinary runtime variance, not a cross-device
performance guarantee. Artifacts are in `build/vulkan-speed/grouped-passes`.

The fixture asserts that 80 compatible draws use at most three passes while
crossing multiple early submissions, and verifies repeated half-alpha red/blue
blend ordering within and across those passes. Existing texture replacement,
uploads, clears, depth ordering, CPU edits and final publication tests also pass.

`tests/Check-VulkanRaster.py` exercises real-driver offscreen pixels: depth,
blending, alpha rejection, decoded/filtering/multistage textures, indexed input,
scissor, channel masks, guest publication, CPU edits, swizzled surfaces, target
switches, cache invalidation, descriptor retirement, unavailable loader and
shutdown/reinitialization. `tests/Check-RendererOptions.py` tests the debug-only
gate and shader-source digest. The D3D11 hardware raster regression also passes.
The Vulkan fixture additionally checks memory-type preference, forced uncached
and noncoherent publication branches, queued old texture versions and survival
of a static cache entry across 300 updates. Forced branches on coherent hardware
do not establish another driver's physical cache behavior.

Portable LLVM-MinGW and MSVC builds were validated. Private hidden-window probes
reached the title screen and H001; captured images were inspected. The initial
45-second H001 probe submitted over 15,000 Vulkan batches with no rejected
batches or D3D11 initialization. These are startup/rendering smoke tests, not a
playthrough, synchronized API comparison, or proof of performance improvement.
The repeat 35-second H001 probe with texture caching recorded over 26,000
batches and 127,000 texture-cache hits without rejected batches. A separate
15-second launch without `--vulkan` initialized D3D11 through the dispatch API.

The backend uses the existing CPU/GDI window presentation, including FMV
overlays and host menus. D3D11's resident target pool is not ported. Full attachment clears
are accelerated only for the currently loaded Vulkan target.
Geometry is currently expanded for upload. Conservative synchronization and
remaining allocation/transfer work can be expensive. A direct Vulkan swapchain,
GPU UI composition, further batching/residency and Linux platform integration
remain future work. Linux support, full scene/effect parity, long gameplay and
validation-layer coverage have not been established.

## Internal resolution

Vulkan now honors Recomp Settings' Internal scaling mode and resolution preset.
Previously it always allocated guest-sized attachments and returned no physical
presentation image, so the setting could only affect D3D11. Linear 640-wide
targets up to 480 rows now scale to the selected preset (partial heights scale
proportionally); swizzled and other-sized intermediate surfaces remain native.
Vertex shader coordinates retain guest dimensions while viewport, scissor,
attachments and transfer sizes use physical dimensions. `[GPU-VULKAN-TARGET]`
reports main target guest/raster dimensions when the selected size changes.

Guest publication samples the physical color/depth image at each native pixel's
center. Full 640x480 targets also retain a physical color snapshot for the
existing locked framebuffer publisher. Native content validates the snapshot
before presentation or reload; CPU edits reject it. Physical depth is retained
separately and reused only when its native depth snapshot, pointer, stride,
dimensions and depth scale match. Full clears invalidate affected snapshots.
Settings changes resize attachments on the next draw and reject stale-sized
presentation images. Disabling Internal mode returns to guest-sized rendering.

The real-driver fixture verifies distinct subpixel samples at 1280x960, depth
occlusion across publication/reload, CPU color/depth edits, 1920x1080 and
2560x1440 target sizes, scaled scissor coverage, and returning to native size.
These are backend pixel tests, not a gameplay visual comparison or performance
claim. Larger attachments increase readback/copy cost; this implementation
retains conservative synchronization and CPU presentation rather than adding a
Vulkan swapchain or GPU compatibility resolve. Physical snapshots can be lost
when CPU writes require reconstruction from native guest data.
