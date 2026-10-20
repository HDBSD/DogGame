# Vulkan renderer

Launch from the installed game's directory:

```powershell
.\DogGame.exe
# Override the saved renderer for one launch:
.\DogGame.exe --dx11
# Optional direct gameplay probe:
.\DogGame.exe --debug --vulkan --level H001
```

Vulkan is the default for ordinary launches. Recomp Settings includes a
Renderer row which confirms and saves a change for the next launch. Restart
after saving progress to apply it; live backend resources are not migrated.
The preference is `[Display] Renderer=Vulkan` or `Renderer=DirectX11` in
`display.ini` beside the executable. Missing or invalid values select Vulkan.
`--vulkan` and `--dx11` override that preference for one launch without requiring
`--debug` or overwriting the saved choice; the last renderer argument wins.
Preview authentication applies only when built with preview locking.

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
overlays and host menus. Vulkan now has its own bounded resident-target pool;
compatible full attachment clears can also select a cached target.
Geometry is currently expanded for upload. Conservative synchronization and
remaining allocation/transfer work can be expensive. A direct Vulkan swapchain,
GPU UI composition, further batching and Linux platform integration
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
2560x1440 and 3840x2160 target sizes, scaled scissor coverage, and returning to native size.
These are backend pixel tests, not a gameplay visual comparison. Larger attachments
increase readback/copy cost. Native GPU resolves and residency reduce that cost
as described below; a Vulkan swapchain is not implemented. Physical snapshots can be lost
when CPU writes require reconstruction from native guest data.

## 4K / 30 FPS work

Only Vulkan exposes the new 3840x2160 preset. Its default is Internal scaling,
the original 640x480 resolution and the 30 FPS cap enabled. 4K is opt-in.
These three settings save independently
under `[Vulkan]` in `display.ini`, using `InternalScaling`,
`InternalResolution=0` (native; 8 selects 4K) and `FpsCap30`. Existing `[Display]` values and DirectX
defaults remain unchanged. Window size/aspect and other shared preferences
retain their existing storage. An explicit saved Vulkan preference overrides
these defaults; selecting Window scaling or another resolution remains possible.

The 128 MiB upload arena fits a 4K color/depth initialization with room left
for geometry, descriptors and textures. The old 64 MiB arena was nearly
exhausted by that initialization alone. Readback ranges now accommodate 4K.

Vulkan keeps its active target plus up to six fence-complete cached targets.
The cache key includes guest color identity/layout, native dimensions and
physical dimensions. Native color/depth snapshots validate CPU ownership before
reuse; depth pointer/stride/scale changes require a new depth upload. Explicit
CPU clears invalidate snapshots even when their values match old native pixels.
Eviction and shutdown destroy cached framebuffers and attachments only after
submitted work is complete. This is a runtime-owned cache, not a recovered
replacement of title rendering routines.

Where BGRA8/D32 blits are supported, intermediate compatibility reads resolve
color and depth on the GPU to native dimensions using nearest sampling. The
physical target remains resident. Drivers without these format capabilities
retain full-size readback. Full physical color is materialized for presentation
through the existing copied window image, including when an intermediate target
was most recently bound. This avoids repeatedly copying 4K depth/color to the
CPU for native-only consumers. Guest synchronization and texture-read order
remain intact; no guest memory is assumed unread merely to save time.

Vulkan window publication uses three slots with read leases. The title thread
fills a free slot while GDI consumes another, then briefly takes the index lock
to publish. A leased image cannot be resized or overwritten. The DirectX path
retains its original global reader/writer lock. `Check-VulkanPresentation.py`
checks coherent publication during a deliberately stalled reader, immutable
leases, nested acquisition rejection and the retained DirectX locking behavior.

`Check-VulkanRaster.py` additionally checks 4K subpixel output, target reuse
without repeated uploads, native GPU readback, unsupported-blit fallback,
pooled-target clears and lazy physical presentation. `Check-VulkanSettings.py`
checks renderer defaults/persistence/isolation. `Check-RendererOptions.py`
checks the Vulkan-only preset and the existing command-line gate. Portable
Clang and MSVC Release builds are validated separately.

### Measured progress and limits

The isolated MSVC Release probes used an NVIDIA GeForce RTX 5090, H001/E006
spawn scenes, muted audio (`--noaudio`, with the APU still running), and
`--debug --vulkan --level <level> --profiler`. These were stationary probes,
not playthroughs. Baseline/residency/resolve runs were 25 seconds, with internal
4K, a 640x480 window, and the host cap disabled. Capped/pacing probes were 35
seconds with internal 4K, a 3840x2160 window, cap ON and focus-loss pause OFF.
FPS means below use one-second window samples from second 15 onward.

| Probe | Mean sampled FPS | Sample range |
| --- | ---: | ---: |
| H001 initial 4K | 10.73 | 10–11 |
| H001 resident targets | 16.09 | 15–17 |
| H001 native GPU resolves + cached clears | 30.00 | 29–31 |
| H001 capped 4K window | 29.90 | 29–30 |
| E006 capped 4K window before snapshot leases | 29.62 | 28–30 |
| E006 capped 4K window with snapshot leases | 29.91 | 29–30.5 |

`[GPU-VULKAN-PACING]` measures title-thread image-publication intervals, not
monitor scanout or input latency. It retains up to 2048 intervals and consumes
that sample window on report. In the last warmed report, H001 had 301 samples:
mean 33.334 ms, p95 35.490 ms, p99 36.425 ms, maximum 36.952 ms. E006 with
leases had 300 samples: mean 33.376 ms, p95 36.603 ms, p99 39.697 ms, maximum
47.419 ms; 15 samples exceeded 36.667 ms. Before leases, E006's warmed p99 was
46.087 ms, with maximum 51.859 ms. Startup/loading intervals contain much
larger stalls. These results **do not establish a locked 30 FPS playthrough**.

Logs and sampled CSVs are under `build/vulkan-speed/4k-*`; the summary is
`build/vulkan-speed/4k-summary.json`. Renderer timings still show CPU
bridge/hash/geometry work, compatibility barriers and CPU/GDI presentation.
Further frame-pacing work, moving/combative gameplay measurements, additional
levels, weaker GPUs, audio-enabled comparison and physical visual validation
remain necessary. No game-native rendering entry points have been replaced,
and no generated guest source has been edited. If later work replaces title
routines, those implementations belong in `src/overrides/` and require a
Vulkan-only gate.

### Gameplay trace with approximately 20 FPS stalls

The supplied E006 `debug-render.log` session selected 3840x2160 before its
ten-second capture. The capture contains 273 flips over 10,016 ms (27.3 FPS
overall), with a slower reporting window of 198 publication intervals: mean
37.661 ms, p95 48.588 ms, p99 49.340 ms, maximum 54.302 ms. These tail intervals
are consistent with the reported approximately 20 FPS slowdown even though the
whole capture also includes recovery. Vulkan rejected no draws.

Subtracting cumulative counters at the capture boundaries gives prepare
2,563.2 ms, wait 1,926.5 ms and publish 1,196.0 ms. Preparation includes nested
wait/publication, so those totals must not be added. The executor's bridge
bucket is 4,501.52 ms, vertex work 934.39 ms and geometry 390.07 ms. The bridge
includes hashing and backend preparation; it does not isolate hashing cost.
Attachment uploads increased by 204.75 MiB color and 1,796.92 MiB depth.
The original log and extracted window are preserved under
`build/vulkan-speed/user-20fps/`.

Code inspection showed changed native depth was expanded to physical size in
a CPU loop before upload. On devices supporting the existing BGRA8/D32 blit
capabilities, changed native color/depth now upload at native size and expand
on the GPU using nearest sampling. For a full main surface, each upload is
1.17 MiB instead of 31.64 MiB at 4K. Valid physical CPU snapshots restore via
bulk copies, without a redundant per-pixel loop. Devices lacking the blit
capabilities retain conservative CPU expansion/full upload. The paths are
Vulkan-only and preserve guest CPU-write invalidation and native readback.

The driver fixture checks exact native/full upload payloads, GPU-expansion
counters, changed color/depth pixels, preserved subpixel rendering and the
unsupported-device fallback. The nearest GPU blit uses pixel-center sampling;
the legacy CPU fallback retains its previous expansion mapping. A new trace
from the same moving/interactive gameplay remains necessary to measure how
much this change affects the reported stalls; the trace establishes an
avoidable cost, not a proof that it accounts for the entire slowdown.

The 35-second stationary E006 smoke run is under
`build/vulkan-speed/4k-native-upload-e006/`. Its last seven one-second samples
are 29–30 FPS. The last publication window still has mean 33.536 ms, p99
42.583 ms and maximum 56.059 ms, so this does not establish a pacing fix.
`upload-expansions` counts native GPU expansion; `[GPU-VULKAN-HISTORY]`
separately reports physical-history restore bytes. Restoring valid physical
history still transfers the full physical payload, now with a bulk copy;
the 27-fold payload reduction applies to changed native-surface reconstruction,
not every upload in the captured interval.

### Second gameplay drop trace: GPU depth history

The next supplied session captured 261 flips over 10,016 ms (26.1 FPS overall).
Its reporting windows include 40 consecutive intervals averaging 42.189 ms,
followed by 221 intervals averaging 37.703 ms. Physical depth-history restore
bytes increased by 1,835.16 MiB during the capture. Cumulative backend deltas
were prepare 1,901.3 ms, wait 3,200.1 ms and publish 902.4 ms, with the same
nested-timer caveat. This is a different path from rebuilding changed native
surfaces: a valid full-resolution history was still being read back, retained
in CPU memory, and uploaded again. The log/window are preserved under
`build/vulkan-speed/user-drop-2/`.

Depth history now lives in one bounded device-local D32 image. After rendering,
GPU image copies retain physical depth before publication. Native depth values
are still copied to the guest at the original synchronization boundary. The
native snapshot, pointer, stride, scale and dimensions validate a later GPU
history restore. CPU edits and explicit clears invalidate it. Recreating a
differently sized history occurs only after earlier work is synchronized;
shutdown releases the history image. Restores record GPU-to-GPU copies without
allocating/uploading a physical depth payload in the host arena.

Depth readback also packs only the physical rows selected by the existing
native pixel-center mapping. Horizontal sampling remains on the CPU using the
original exact indices. At 4K this is 3840x480 floats (7.03 MiB), instead of
3840x2160 (31.64 MiB). A nearest depth blit was rejected because the driver
fixture selected a different texel at a tie, changing a native depth result.
The row-copy approach preserves the old selected values, including on devices
without blit support. Color readback and physical presentation remain separate.

The driver test now evicts a 4K target, restores its subpixel depth from GPU
history, checks occlusion and verifies zero host depth-upload bytes for the
restore. Existing CPU-write/clear invalidation checks pass. Additional tests
exercise row-packed depth through the uncached-readback bulk-copy path and
check readback dimensions. Portable Clang and MSVC builds pass.

The stationary 35-second E006 smoke run under
`build/vulkan-speed/4k-gpu-depth-e006/` reported zero physical CPU depth-history
restores and 2,183.20 MiB of GPU depth-history copies. Final one-second samples
were 29–30 FPS; its last interval window still had p99 44.610 ms and maximum
50.923 ms. This proves the host round trip is gone for those restores, not
that the supplied interactive slowdown or all pacing spikes are fixed.

### Third gameplay drop trace: fallback clear ownership

The next capture contains 279 flips in 10,016 ms (27.9 FPS). Backend fence
waits increased by 2,634.2 ms, including 1,713.3 ms across 730 unsupported CPU
clear boundaries. Those boundaries published 855.47 MiB of color and 5,132.82
MiB of depth. Physical CPU depth-history restores remained zero. The original
log is preserved in `build/vulkan-speed/user-drop-3/debug-render.log`.

Inspection of `nv_gpu_discard_overwritten`, `clear_surface` and `zbuf_clear`
shows that the fallback synchronized the active target before every clear,
even a validated clear of another allocation. Vulkan now keeps recording when
a validated linear color-clear range is disjoint from the active linear color
surface and the clear's host depth buffer differs. Backend texture uploads
already own their source data; pooled attachments are fence-complete. Cached
snapshots of the cleared allocation are still invalidated. Unknown color
pointers, nonlinear layouts and possible overlaps retain synchronization.

When synchronization is necessary, it omits publication of an active attachment
that the imminent CPU clear completely overwrites. Partial color clears still
publish existing pixels; unrelated dirty attachments still publish normally.
The fence remains in this path, and the caller must perform the promised CPU
writes before another backend call. This changes host scheduling only; it is
not a newly recovered guest rendering routine. DirectX and generated source
are unchanged.

Driver tests check that an unrelated clear does not submit or wait, partial
clears retain untouched pixels, full overwrites avoid readback, and subsequent
draws consume the CPU-cleared values. Portable Clang and MSVC builds pass.

The executor uses the preparation result to suppress the CPU helpers' duplicate
sync while retaining every original CPU write. The isolated MSVC, audio-disabled
35-second E006 run at internal/output 4K is saved under
`build/vulkan-speed/4k-cpu-clear-e006/`. Final caption samples were 29-30 FPS;
the last 298 publication intervals averaged 33.702 ms, p95 37.609 ms,
p99 44.987 ms and maximum 48.342 ms. Clear waits disappeared, but some work
moved to later depth-attachment boundaries. This is not evidence of an overall
speedup or a locked 30 FPS on the supplied interactive route. That limitation
is material: removing one named wait category alone does not remove all of
the dependency's work. The preview deployment uses the portable compiler and
retains preview locking.

### Depth ownership implementation

Color publication now has an explicit API separate from CPU depth consumption.
Depth attachment changes no longer force full publication through the dispatch.
Vulkan retains pending depth in the bounded target pool, uses host allocation
generations and content revisions, and copies compatible depth between bindings
on the GPU. Software consumers, incompatible layouts and eviction materialize
native values. Separate color revisions preserve physical detail across depth
rebindings. CPU edits and clears invalidate stale versions. Color fences and
GDI presentation remain; this is not yet an entirely GPU-resident frame path.

The implementation, correctness checks and final before/after measurements are
recorded in `VULKAN_FRAME_TIME_ROOT_CAUSE.md`. In stationary 4K E006 runs, measured
raster-path CPU time fell from 24.89 to 21.83 ms/frame and depth readbacks from
21.83 to 0.51 MiB/frame. Both compilers and Vulkan/DirectX raster tests pass.
The corrected run ended with 30 FPS captions, but pacing spikes remain and the
interactive route still needs validation. Defaults remain native resolution at
30 FPS; this ownership change applies only to Vulkan.
