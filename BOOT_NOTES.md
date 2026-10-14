# Boot investigation

## Runtime restructure (14 October 2026)

Current build instructions are in BUILDING.md and Setup.ps1. Runtime customizations
now live in runtime_overrides; xboxrecomp is pristine pinned upstream. CMake
assembles build/runtime/xboxrecomp and never patches the upstream source tree.
The generation seeds are in config/seed_functions.json. Setup scans all persistent
title sources, not only title_overrides.c. Historical commands below use the old
layout. The pre-restructure generated sources are retained locally in
build/pre-restructure-gen for comparison; they are not distribution inputs.

## Current continuation (11 October 2026)

Use `build/Release/sd100f.exe` for all game tests. The CMake target remains
`your_game_recomp`, with output name `sd100f`. Stop that process after each
test, before relinking. Historical executable names below describe old tests.

The fixed-function vertex path now applies the composite transform and viewport
offset, uses diffuse/specular attributes 3/4, and supports texture matrices,
weighted model-view transforms and fog generation. Hardware mappings were
checked against xemu's `hw/xbox/nv2a/pgraph/glsl/vsh-ff.c`. Fixed-function lighting
is still incomplete; it was disabled in the captured title draws.

GPU completion now follows the executed semaphore-release packet rather than
copying the CPU submission counter. This title uses semaphore offset zero and
one registered completion target; other DMA semaphore bindings remain unsupported.
The FIFO walker follows actual jumps across wraps and starts at the title's
registered ring base (`device+0x24`), preserving initial setup commands.

Pixel statistics are accumulated per worker. Common textured and diffuse-only
combiner programs have exact-match fast paths, checked against the general
implementation with 65,536 randomized cases. Audio uses a streaming linear
resampler with persistent decoded ADPCM blocks, and includes the inclusive final
sample of each source segment. Cache invalidation covers new voices and reused
blocks. This removes repeated block decoding without changing source order.

Focused native checks: `analysis/Check-FixedTransform.py`,
`analysis/Check-GpuFence.py`, `analysis/Check-PushbufferRing.py`,
`analysis/Check-AdpcmCache.py`, and the combiner test all passed. These checks
exercise extracted production functions or compile the production combiner.

The texture sampler also caches Morton address contributions per worker (four
dimension pairs, 4096 coordinates per axis), without caching texture contents.
Power-of-two wrapping uses a signed-equivalent bit mask. The native
`Check-SwizzleCache.py` compared 692,224 sampled addresses and 130,000 wrapped
coordinates against the original formulas, including negative coordinates,
rectangular dimensions, replacement and fallback. Opaque pixels in ordinary
source-alpha blending skip destination unpacking.

`continue6.stderr.log` records a six-minute canonical-executable test that
reached the main menu without an observed access violation. The watchdog ended
the test. `continue6.bmp` shows the control-room background and menu text.
Title rendering improved to roughly 15 fps in earlier runs with eight raster
workers; the main menu remains about 6.5 fps. A reaches the raw and logical pad
states, but New Game did not activate in this run. Audio pitch, stutter and world
rendering therefore still require gameplay validation. Title characters also
retain incorrect dark shading.

`continue9.stderr.log` subsequently loaded H001 and cin1, and rendered Scooby,
the manor, terrain, pickups and HUD (`analysis/gameplay-continue9.bmp`). A
movement/pause test then hung. The watchdog identified `sub_001041A0`, waiting
for the low five bits of `NV_PGRAPH_PATT_COLOR0` to match the memory fence.
The back-end fence was 0x13CB; the skipped front-end pattern register was zero.
The executor now handles NV044 `SET_MONOCHROME_COLOR0` (subchannel 5, method
0x310) in order, writing PGRAPH offset 0x400B10. The marker encodes the command
address in its high bits and the fence's low five bits in its low bits.
Other non-3D subchannel methods
and dynamic object bindings remain incomplete. This update is covered by the
native fence check. `continue10` subsequently rendered pause and resumed, then
crashed in `sub_000B5632` reading a corrupted PRCB pointer through `fs:[0x20]`.

Thread TLS initialization was inconsistent: `fs:[0x28]+0x28` reached a shared
stand-in while the negative `fs:[4]` index reached another allocation, whose
payload was only 64 bytes. The title's XAPI startup initializes a self-pointer
followed by its full 144-byte TLS payload. The corrected layout gives each
thread its own thread structure and a single 148-byte TLS block, with both
access paths reaching that block and slot zero pointing four bytes into it.
Initialized image data also starts after the self-pointer. The native
`Check-ThreadTls.py` verifies the SDK's complete payload write cannot affect
the next thread's TIB or TLS. `continue11` tests this correction in-game; the
previous crash's exact overwrite source has not yet been established.
The old bootstrap PRCB (`0x761000`), thread stand-in (`0x760000`) and TLS
template (`0x770000`) also fell inside the title's registered GPU command ring
(`0x80740000..0x807C0000`, using the same physical offsets). The PRCB and bootstrap
thread structure now occupy reserved low addresses `0x5000` and `0x6000`;
the bootstrap TLS template uses the normal allocation bookkeeping. This
avoids those offset assignments. However, the runtime backs low memory and
the contiguous GPU window with separate sections: matching physical offsets
do not establish an actual overwrite. The relocation is not a proven crash
fix. `continue12` tests the combined memory-layout changes.

Performance also affects input: the guest main loop discards frame deltas over
0.1 seconds (`sub_0006CEB0`, 0x6D04F). Repeated menu reads in the slow runs
showed `0x1A4F14 == 0`, preventing transition animations from enabling input.
This is a reason to improve rendering rather than bypass the menu state.
Earlier gameplay results below are historical, not validation of this build.

The original disassembly cache used `text_only: true`. SDK entry points
referenced by game code consequently became unresolved stubs. In particular,
`sub_00136B43` (called by `sub_00136DF7`) only advanced the guest stack by four
bytes instead of executing its body and consuming its arguments. The audio
initialization loop at `0x000AC531` then repeatedly allocated 294912-byte
buffers and exhausted the contiguous arena.

Regenerate from all executable sections, without `--text-only`. From the
`xboxrecomp` directory:

```powershell
python -m tools.disasm ../SD100F/build/Release/game/default.xbe -o ../SD100F/analysis/disasm --force --seed-functions ../SD100F/analysis/seed_functions.json
python -m tools.recomp ../SD100F/build/Release/game/default.xbe --all --disasm-dir ../SD100F/analysis/disasm --split 250 --gen-dir ../SD100F/src/recomp/gen --output-dir ../SD100F/analysis/recomp --game-name SD100F --exclude-manual ../SD100F/src/title_overrides.c
```

Python requires `capstone`. The investigation installed a workspace copy in
`analysis/python-deps`; set `PYTHONPATH` to that absolute directory to use it.
The previous generated files are preserved in `analysis/original-gen`.

The host now uses a local `saves` directory for partition images. The default
AppData location returned access denied for `Partition0.img` in the diagnostic
environment and caused a dashboard reboot. Run the executable with its Release
directory as the working directory so that `game` and `saves` resolve correctly.

The kernel import log was also corrected to stop at the loaded title's import
count and exclude zero-filled fallback slots. This title imports 108 exports;
all 108 resolve. The previous 231 unresolved-ordinal-zero warnings were spurious.

Runtime-observed indirect-call targets are recorded in
`analysis/seed_functions.json`. Pass that file when regenerating disassembly.
The host also registers the title's GPU completion fence: device pointer at
`0x0010E648`, submitted counter at `device+0x2C`, completion pointer at
`device+0x30`. Without acknowledgement, `0x001045A0` spins forever despite the
NV2A DMA GET pointer catching up with PUT.

## Latest verified result

The independent PFB cache-flush worker is now disabled by default after the
user reported startup and gameplay freezes following that loading change.
It is retained only behind RECOMP_INDEPENDENT_PFB_FLUSH=1 for investigation.
The original rendering-worker acknowledgement remains active. Native flush
handshake tests passed but did not establish title-level correctness.
The first rollback test (visible PID 49132, interactive-flush-rollback logs)
progressed to cin1.xmv; the preceding run froze in D3D BlockOnTime before it.
Playable movement and subsequent level transitions still need confirmation.
The ordered software completion notification and bounded menu timing fixes
remain enabled. No loading-speed improvement is currently claimed.

2026-10-11 save-dialogue investigation: interactive-save-state recorded mode
6 with input transitions, then exhausted the contiguous arena and crashed in
sub_000B5632. Dump: sd100f-crash-20261011-181437-2908.dmp. The save frame loop
0x740D0 discards elapsed time above 0.1 seconds, preventing its input cooldown
from advancing at slow frame rates. src/title_overrides.c preserves the original
loop but caps those deltas instead. Regeneration MUST use the documented
--exclude-manual argument above to avoid duplicate definitions. Check-SaveTiming.py
passes normal timing and cooldown completion at 7.5 FPS. The user confirmed
saving returns control in interactive-save-fix, and subsequently confirmed
the map dialogue completes correctly.
The same run subsequently crashed while loading F003.HIP at 18:32:10:
interactive-save-fix.stderr.log recorded an unresolved indirect call to
0x00039D80 immediately before an invalid-pointer read in sub_000791B0.
Dump: sd100f-crash-20261011-183210-48712.dmp. The missing callback is an
834-byte original function (208 instructions), now persistently seeded and
translated. Its nine direct callees all resolve, and its generated body has
no unimplemented instructions. Regeneration recovered 6291 functions;
6290 are generated and the save-frame loop remains manual. Retrying the F003
transition is required to verify whether recovering this callback fixes it.
The canonical Release build succeeded and was launched on the Default desktop
as PID 47544, logging to interactive-f003-fix.stderr.log/stdout.log. The
framebuffer window opened. This is an active user test; leave it running until
the user finishes or a fault occurs.
RECOMP_CONTIG_RECLAIM enables contiguous-buffer reuse independently
of general heap reclamation; Check-ContiguousReuse.py passes 10000 cycles,
alignment, merging and invalid/double-free checks. The SD100F build enables
this by default; setting RECOMP_CONTIG_RECLAIM=0 retains the old bump allocator.

The renderer now handles culling and alternating triangle-strip winding;
Check-TriangleCulling.py passes. Culling rejected 19050 triangles in the shared
run, but that alone did not deliver high FPS. RECOMP_PROFILE_GPU measured
8276.7 ms spent rasterizing in 9981.5 ms wall time. Hardware acceleration or
further software-rasterizer optimization is still required for high FPS.
RECOMP_STATE_TRACE logs menu/input timing every half-second, including separate
save-screen timing. Routine draw progress logging now requires RECOMP_DRAW_TRACE.
The prepared pixel optimization selects the exact simple combiner mode once
per triangle and skips unused pixel inputs. It retains texture clipping and
the general evaluator when extra texture stages are active. The 65536-case
reference comparison passes for both regular and explicitly selected simple
evaluators. This optimization is included in the rebuild following the F003
crash; its performance benefit is unverified.

2026-10-11 interactive test: sandbox launches run on a separate Windows
desktop even with WindowStyle Normal. Launching on the Default desktop made
the canonical sd100f.exe visible, confirmed by the user. The manual run with
RECOMP_FRAMECOUNTER_CLOCK=1 reached manor gameplay, then crashed at 17:46:25.
The dump is sd100f-crash-20261011-174625-41860.dmp. Immediately before the
bad-pointer fault in sub_0002AD70, an indirect tail jump to 0x00041F0F failed.
That missing event-handler branch restores three saved registers and returns;
skipping it unbalances the guest stack. The target is now in the persistent
seed list and regenerated dispatch. Gameplay validation of this recovery is
pending. Intro timing, rendering performance, audio and USB stability remain
unfinished.

The next visible run, interactive-handler, reached I001 and the map dialogue.
The user reports A advances dialogue but control never returns. Its log also
records missing branch 0x00041FA1, followed by KeBugCheck(0), and no access
violation dump. The final frame shows Scooby facing the NPC. Both handler
jump tables (0x42150 indexed by 0x42164, and 0x42188 indexed by 0x421C4)
are now read from the original XBE; all 17 distinct branch targets are seeded
for regeneration. This recovers the event-result writes and stack epilogues
without forcing dialogue state or bypassing the control lock. Validation of
dialogue completion remains pending.

`build/Release/SD100F_controllercheck2.exe` reached the actual main menu, with
New Game, Load Game, Options, Promo Materials, and Credits visible. Start
input skipped the movies through the guest's USB/XAPI driver, then opened
the menu. A subsequent Down input changed the selection from New Game to
Load Game. The captured menu is `analysis/main-menu-verified.bmp`; the run's
log is `build/Release/controllercheck2.stderr.log`. No unresolved indirect
calls were observed in that run. Gameplay has not been verified.

`build/Release/SD100F_mainmenu.exe` is the separate build for this milestone.
Its fresh `Run-Menu.ps1 -AutoMenu` run also reached the same visible menu;
`build/Release/mainmenu.stderr.log` records all six Start presses and no
unresolved indirect-call failures during that boot.
The original running `your_game_recomp.exe` remains untouched. Diagnostic
builds use `/Od` for quick iteration and include debug symbols.

To rebuild this executable with the existing Visual Studio configuration:

```powershell
$env:_CL_ = '/Od'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' SD100F/build/your_game_recomp.vcxproj /p:Configuration=Release /p:Platform=x64 /p:BuildProjectReferences=false /p:TargetName=SD100F_mainmenu /m /verbosity:minimal
Remove-Item Env:_CL_
```

`BuildProjectReferences=false` reuses the runtime libraries; rebuild those
first with CMake when changing runtime source. `TargetName` keeps the original
executable separate while it is running.

## Runtime fixes required for this title

- AC97 readiness allows DirectSound creation to succeed. The earlier null
  buffer-list crash followed failed device initialization.
- Contiguous physical allocations reserve the loaded XBE range. Otherwise
  DMA address resolution reads image bytes instead of APU descriptors.
- The passthrough GP model acknowledges commands at scratch offset `0x810`
  through the programmed SGE table. GP/EP effects execution remains stubbed.
- The title's D3D field counter, vblank event, and video field parity advance.
  `IN AL,DX` now reads port `0x80C0`; previously it left a stale register value,
  preventing the XMV playback clock from starting.
- PVIDEO YUY2 overlays are composited into the framebuffer window, and
  PVIDEO STOP retires the active buffer so movie cleanup can finish.
- The host initializes OHCI and handles its MMIO faults. Recovered callbacks
  `0x0017C3A9` and `0x0017B955` complete controller enumeration and input reports.

These runtime options are enabled by the title host by default. The recovered
callbacks are retained in the seed file for regeneration.

## Running the menu build

From the workspace root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\SD100F\Run-Menu.ps1
```

Focus the framebuffer window. Enter is Start (skip each movie and open the
menu), arrow keys navigate, Z is A/choose, and X is B/back. An XInput gamepad
is also supported. `Run-Menu.ps1 -AutoMenu` sends spaced Start presses for a
repeatable menu boot test. It does not choose New Game.

## Remaining limitations

The menu is readable and navigable, but its background rendering is visibly
incorrect. Intro playback can stall at its final frame without Start input;
automatic movie completion still needs investigation. Full regeneration
reports 45 unresolved direct-call stubs and 911 unsupported instructions,
many in sweep-decoded data. These counts are not evidence that every executed
path is implemented. Audio effects and gameplay are not verified.

## New Game investigation (2026-10-10)

Recovered callbacks `0x00060E90` and `0x00066DB0` bring the seed count to
21 and the generated function count to 6269. The optimized
`SD100F_newgame.exe` includes both, but selecting New Game still hangs.
The older optimized run's watchdog identifies the repeated GetStatus loop
at `0x000ACA68`: status remains PLAYING after an immediate StopEx.
The recovered run shows the same repeated critical-section calls without
further GPU progress. This is a reproduced hang, not a verified crash fix.

Natural intro playback in `movie-rate.stderr.log` showed roughly 1.4–2.8
observed content changes per second after the initial interval. Optional
`RECOMP_OVERLAY_STATS=1` samples overlay content; its rate is a diagnostic,
not the encoded movie frame rate.

Pending, untested changes request Windows 1ms timer resolution in the host
and add `RECOMP_APU_STATS=1` diagnostics for frame pacing, register state,
and voice notifications. Build these into a separate executable before
testing; the build was not executed because automatic approval review
could not complete after hitting the account usage limit.

Next verification: compare natural intro pacing, then select New Game and
trace VOICE_OFF notifications through the APU ISR and DirectSound DPC.
Keep the original running executable untouched. The menu launcher still
targets the earlier menu build, not these diagnostic changes.

## Audio and level loading investigation (2026-10-11)

The APU now preserves decoded VP/DSP samples through host audio submission,
mixes 256 samples per eight subframes, and collects those samples into the
XAudio2 or waveOut output buffers. Previously it cleared decoded samples and
advanced software voices by a full host buffer at each 256-sample interval.
`RECOMP_AUDIO_DUMP=<path>` captures raw 48 kHz stereo 16-bit PCM for checking
the output. Captures from the level-loading tests contain nonzero audio;
effects DSP execution and perceived audio quality still need verification.

Idle-voice traps stop the voice-list walk immediately, preserving the voice
number for the guest interrupt handler. Trap status uses an exact method-mode
comparison and clears when the guest releases the trap. This prevents the
same idle voice from being serviced twice after DirectSound removed it.
APU interrupts also respect guest IRQL throughout protected voice updates.
These changes pass the native interrupt harness in `analysis/Check-ApuIrq.ps1`
and allow New Game to advance into `H0/H001.HIP` loading.

The title host maps its virtual controller to physical root port 3, which
this XDK assigns to Xbox player one (root port index XOR 2). Heap reclamation
is no longer enabled by default: enabling it regressed menu initialization.
Windows 1 ms timer resolution is now enabled by default. An isolated test
with heap reclamation disabled booted through New Game and delivered about
1496 APU subframes/s (1500 required), compared with roughly 1100–1280/s before.
Set `RECOMP_TIMER_RESOLUTION=0` to disable it for comparison. XAudio2 prebuffers
two 1024-sample buffers, restores that cushion after an underrun, and reports
queue/underrun counters under `RECOMP_APU_STATS`. The APU thread runs above
normal priority and catches up short scheduling delays instead of immediately
discarding its timing schedule.
The final audio test held 1499–1501 subframes/s with one or two buffers queued,
no submission failures, and no buffer-full drops. Several underruns remain at
initialization/loading/video transitions. Intro video playback is still slow;
the timing fix does not complete the video or effects-DSP implementation.

The next crash was traced to a zero-size RWTX asset at index 16 of H001's
texture layer. Its conditional branch reached unresolved stub `0x000117FB`,
which popped only a return address rather than restoring saved registers and
the 0x14-byte local frame. The remaining textures were left unconverted and
the next layer read reused their buffer. RenderWare then treated bytes from
that read as the list links of texture `ogsh0000` and crashed in `0x000B9A50`.
Seeds `0x000117FB` and `0x0001180A` recover the shared cleanup and success
continuation. Runtime testing confirmed every remaining texture converted,
H001 finished loading, and the opening `FMV/cin1.xmv` cinematic played.
`analysis/Check-TextureCleanup.py` executes the generated cleanup block and
checks the guest stack, saved registers, output size, and null result.
The following scene-update crash occurred after four unresolved callbacks:
`0x0003A790`, `0x00054810`, `0x000647F0`, and `0x00066350`. Recovering their
original code allowed `SD100F_gameplayfix.exe` to enter the scene, respond to
movement input, and pause/resume without that crash. Rendering remains
incomplete: the player/HUD appear, but much of the environment is missing.
The new direct-stub diagnostics also identified missing initialization at
`0x00068520`; it is recovered in `SD100F_newgame.exe`. There are now 28 seeds,
6276 functions, and 43 unresolved direct targets. `tools.seed_from_log` now
recognizes executed direct stubs using the same code/body validation gates.
The final `SD100F_newgame.exe` test also entered the scene after the cinematic,
with no unresolved indirect calls or executed direct stubs. The preceding
`SD100F_gameplayfix.exe` test ran to its five-minute watchdog without an access
violation and exercised movement plus pause/resume. These are first-scene
checks, not verification of the entire game.
The final run also completed its 300-second watchdog interval without an
access violation, including movement/jump input. Its last XAudio2 counters
were 13720 submitted, 8 underruns, 0 full-buffer drops, and 0 failures.
See `analysis/newgame-validation.json` for the recorded checks and
`analysis/newgame-audio-preview.wav` for the final 12 seconds of PCM output.

Run the current build with `powershell -File SD100F/Run-Menu.ps1`, or add
`-AutoNewGame` to automate the menus. The launcher creates separate timestamped
logs for each run. Automatic menu timing is a convenience and may need manual
input on slower machines.

The kernel's import lookbehind scan now rejects marker words outside the
kernel ordinal range, avoiding writes into unrelated image data. Read logs
include their guest buffer addresses; crash reports retain indirect-call
history even when the guest stack scan finds return addresses.

## 2026-10-11: black-screen startup after independent PFB flush
Both live instances waited in D3D BlockOnTime (0x104470), on device+0x1DBC. The executor had received method 0x100 parameter 5 but counted it as unhandled. The faster flush acknowledgement exposed this missing software completion notification.
Added explicit per-title completion-event registration and ordered notification handling in nv2a_pb_exec_method. The host event helper signals the guest event without entering guest code or altering guest registers. Notification occurs when the FIFO executor reaches the command, after preceding draws; it does not acknowledge submitted work early.
Release runtime and canonical sd100f.exe builds passed. Visible PID 37432 progressed through WBLogo, THQLogo, HILogo, RW4 and Intro within the initial observation, with mode 2/sub 2 and no recorded exception. Left running for user testing. Level-transition performance remains unverified.

## 2026-10-11: menu appears frozen after startup completion fix
Live PID 37432 continued rendering roughly 12.8 FPS, with active raster workers and successful BlockOnTime event waits. Menu state mode 2/sub 2 had dt=0 and idle delay fixed at 120 seconds. The original menu loop sub_0006CEB0 discards elapsed time above 100 ms, as the save loop did. Moved the original menu function into persistent title_overrides.c and reused save_dialogue_delta to cap positive long frames rather than discard them. Negative/nonfinite deltas remain zero. Regeneration must continue excluding title_overrides.c (now three functions).
Native bounded-time test passed; Release canonical game build passed. Restarted visibly as PID 45180. Runtime logs show dt=0.1 on slow frames and idle countdown decreasing (119.900 -> 119.813 -> 119.344, with resets during controller input). Input responsiveness awaits user confirmation. Software rendering performance remains approximately 13 FPS; this change fixes timer starvation, not renderer throughput.
User confirmed the rollback run is significantly more responsive. Keep independent PFB flush disabled; revisit loading speed with command-ordering diagnostics before enabling it again.

## 2026-10-11: pause background and missing Scooby portrait
Frame tracing identified a full black camera clear from 0x77BCE during mode 7, immediately before the pause overlay. The persistent sub_000BDD70 override removes only the image-clear flag for that caller and mode, retaining the depth clear. The user confirmed the world is now visible behind the pause text. This is a scoped title compatibility fix.

The missing Scooby portrait submits valid screen coordinates but zero output W, so the software renderer discarded its triangles. Programmable screen-space vertex output now bounds W to signed 2^-64 through 2^64, matching the NV2A handling used by xemu. Clipping uses that lower bound and preserves it explicitly at intersections. Check-ScreenVertexW.py passes zero-W portrait geometry, negative-zero rejection, finite/infinite bounds, and geometry crossing the clipping plane. The Release runtime and canonical sd100f.exe build passed. The user confirmed the visible pause-menu test works; the subsequent framebuffer capture also shows the restored Scooby HUD portrait over the world after resuming. Logs: interactive-pause-w-fix. Diagnostic PID 7112 was closed after verification.

Regeneration must exclude src/title_overrides.c with --exclude-manual; it now contains four manual functions (0x740D0, 0x109530, 0x6CEB0, 0xBDD70).

## 2026-10-11: world rendering investigation
Captured the user-reproduced scene with overbright white surfaces and stretched dark geometry. One-frame trace is in interactive-world-render.stderr.log. Eye-crossing geometry was clipped only to W=2^-64 then projected to enormous coordinates, overflowing integer raster bounds. Added homogeneous screen-space clipping to the four viewport boundaries before division. Extended Check-ScreenVertexW.py to reject nonfinite/out-of-viewport output, including an extreme offscreen triangle; tests and Release builds pass. Visible comparison is pending in interactive-world-clip (PID 45088). Incorrect colour/bloom investigation remains open; this geometry change is not yet a verified colour fix.

User reported no visible difference with the viewport-clipping change. It bounds extreme coordinates but is not a verified fix for the reported defects. Added opt-in RECOMP_FRAME_TRACE_IMAGES: records the framebuffer and bound texture after each draw of the single triggered frame, along with fog parameters and palette address. Diagnostic interactive-world-images PID 16328 awaits the reproduced scene; no rendering correction is claimed yet.

Per-draw snapshots identify the pale surface at image 36, before bloom: FIX draw n=144, three combiner stages, stage-1 texture matrix enabled. Raw command capture contains SET_TEXGEN S/T/R NORMAL_MAP (0x8511), which was not implemented. Added per-stage/component texgen state, normal transformation through inverse model-view (including skin weights), optional normalization, and normal-generated S/T/R before texture matrices. Other texgen modes remain unimplemented. Check-NormalTexgen.py extracts the production transform and passes disabled-coordinate preservation, normal mapping, inverse matrix, Q preservation and normalization. Runtime and canonical Release game builds pass. Visible comparison awaits user feedback in interactive-world-texgen PID 18256; world rendering remains unverified.

User reported normal-texgen comparison is "looking good so far". Captured world-normal-texgen-verified.bmp shows darker metallic swing framing and reduced overbright surface compared with the prior snapshots. This verifies a visible improvement in this scene, not correctness of all geometry or colours across the game. Closed completed diagnostic PID 18256. Remaining texture-generation modes, lighting and broader world-rendering validation remain open.

## 2026-10-11: Load Game polygons and first-level floor gap
Captured stray polygons over the Load Game TV panel. One-frame snapshots/logs are loadtri-* and interactive-load-triangle.stderr.log. The title programs CLIP_MIN=0, CLIP_MAX=16777215 and ZMIN_MAX_CONTROL=1, but these methods were ignored. Added tracked near/far depth clipping to homogeneous polygon clipping, with depth clamping for the explicit clamp mode. Extended Check-ScreenVertexW.py with far-plane rejection, near-plane crossing and bounded intersection depth; tests and Release runtime/canonical game builds passed. Visible test interactive-depth-clip PID 10008 awaits Load Game and floor comparison. No visible fix is claimed pending that comparison.

Depth-clipping comparison: user reported the Load Game triangle is still missing. Further inspection found ARRAY_ELEMENT32 (method 0x1808) was unhandled: the prior frame trace reports 34798 dropped commands, while the renderer handled only paired ARRAY_ELEMENT16. Single final indices of odd-length strips were therefore dropped, removing their final triangle. Added the single-index path with BEGIN and capacity guards. Check-ArrayElement.py extracts the production cases and verifies five-index strips, final triangle count and guards; native and Release game builds passed. Visible comparison is pending in interactive-single-index PID 34900. This is the leading explanation; Load Game/floor visual confirmation remains required.

Verified single-index fix: user confirmed "yes, that fixed both issues" for the Load Game triangle and first-level floor gap. Captured single-index-verified-7bc000.bmp and single-index-verified-8e8000.bmp from PID 34900; logs are interactive-single-index. Closed the completed test. Canonical build/Release/sd100f.exe contains the fix. This verifies those two defects, not all rendering throughout the game.

## 2026-10-11: striped floor shadows
Read live shadow render target 0x80B80080 from PID 23504. shadow-linear.bmp contains a coherent Scooby silhouette; shadow-swizzled.bmp shows the same scattered stripe pattern as the floor. The command ring programs surface format 0x08080224 (256x256 swizzled). Renderer writes and clears were always pitched linear, while the texture sampler correctly reads swizzled format 07. Added shared surface_pixel_offset for swizzled/linear layouts and used it for raster blending reads/writes, swizzled clears and diagnostic snapshots. Check-SwizzledSurface.py passes a full 256x256 write/read round trip, ordinary framebuffer offsets and rectangular 16-bit offsets. Runtime and canonical Release game builds pass; visible comparison awaits feedback in interactive-shadow-layout PID 40492.

User confirms no more black floor lines with the swizzled render-target fix, but says the remaining shadow is not as expected. Capture shows a coherent shadow near Scooby; exact remaining shape/colour/placement issue awaits clarification. Keep diagnostic PID 40492 running for this ongoing comparison.

Remaining shadow issue: user expects a top-down Scooby silhouette, currently sees a large black region with a triangle cut out. Live corrected shadow-fixed-swizzled.bmp contains a clean Scooby mask, confirming generation/layout correction. Added one-frame [SHADOW] vertex output diagnostics for the projection using 0x80B80080; visible interactive-shadow-projection PID 23472 awaits reproduction. Shadow shape is not yet resolved.

Shadow projection trace: texture coordinates extend beyond [0,1], texture address=0x10303 (clamp-to-edge). The corrected silhouette texture retains white edge pixels because a full-screen clear quad at 0.53125 misses pixel centres at 0.5. NV2A screen coordinates truncate to four fractional bits; renderer omitted this step. Added truncation after fixed/program transforms, mapping 0.53125 to 0.5. Check-ShadowCoverage.py extracts production rounding and verifies all 256x256 clear pixel centres and negative-coordinate truncation; native and Release builds pass. Visible test interactive-shadow-coverage PID 16020 awaits confirmation. Large-area shadow shape remains unverified.

User confirmed "shadows fixed" with screen-coordinate quantization plus swizzled render-target addressing. Captured shadows-verified-7bc000.bmp and shadows-verified-8e8000.bmp; logs interactive-shadow-coverage. Closed completed PID 16020. Canonical sd100f.exe contains both shadow fixes.

## Guest virtual-memory free ABI correction
NtFreeVirtualMemory warnings with concatenated 64-bit addresses (e.g. 00F7EDC0024EE000) came from passing 32-bit guest BaseAddress/RegionSize slots to the native x64 routine. The bridge now reads/writes guest DWORDs and handles tracked heap ranges without calling VirtualFree on the shared Xbox RAM mapping. MEM_DECOMMIT zeroes a validated page-rounded range; invalid requests return guest status codes. MEM_RELEASE retains the existing opt-in heap reuse policy and clears the guest output slots. Extended-VMA handling remains first. Check-FreeVirtualMemory.py extracts the production bridge and passes adjacent-slot guards, rounded/bounded decommit, release and null argument handling. Runtime and canonical Release game builds pass. Visible loading/gameplay verification awaits interactive-memory-free PID 42288.

Memory-free verification (2026-10-12): user played through a few levels and reported it looks good. Fresh xbox_kernel.log contains zero NtFreeVirtualMemory failure warnings; interactive-memory-free.stderr.log contains no recorded exception/access-violation/crash messages. Other warnings remain, notably ExQueryNonVolatileSetting index 0xFF. Closed completed diagnostic PID 42288.

## 2026-10-12: level-transition crash investigation (not resolved)
- Preserved dump 20261012-000624-34664 and matching executable/PDB/kernel log under analysis/crash-20261012-000624 before rebuilding.
- Dump exception: read access violation 0xC0000005 at native 0x7FF79ADE791B, read address 0x7F010000. Matching PDB resolves sub_00067D50+0x78B, recomp_0004.c:21643: collision-grid traversal dereferencing object flags at guest label 0x67EC0. No missing-function call at the fault; pointer corruption origin remains unknown.
- Normal minidump lacks guest RAM. main.c now additionally writes a sparse .dmp.guest sidecar on crash: SDGUEST1, nine little-endian DWORDs (eax/ecx/edx/esp/ebx/esi/edi/seh_ebp/RAM size), then readable 4096-byte page records (guest address, byte count, payload). Uses ReadProcessMemory to skip inaccessible pages. Register globals do not include all generated local temporaries; retain native dump and matching PDB.
- Release canonical sd100f.exe build passed. Visible user reproduction PID 18728, interactive-level-crash stdout/stderr logs. No gameplay fix claimed; next evidence needed is another level-transition crash.

## 2026-10-12: repeated level transitions exhaust the heap
New crash 001303-18728 followed piercing audio reported by user. Stderr contains allocation failures requesting 2177680, 1286544 and 1694720 bytes (heap used ~50.1MB of 50.9MB), plus a null-buffer level read rejected by NtReadFile. Crash resolves sub_00091B70+0x581, guest collision insertion path. Sparse guest RAM shows grid 1BD8A0+30 points to 01518EA0, whose list records contain mesh-like float values instead of counts/pointers. This establishes invalid collision data following memory exhaustion, but does not establish which write damaged it or prove the audio shares the cause.
Preserved matching exe/PDB and run logs under analysis/crash-20261012-001303; original dump and .guest remain in build/Release. Inspect-GuestCrash.py produces guest-summary.json.
Title now enables RECOMP_HEAP_RECLAIM=1 by default: split/coalesced allocations and returning released NtFreeVirtualMemory reservations to heap. Runtime opt-in remains unchanged for other titles; explicit RECOMP_HEAP_RECLAIM=0 now correctly disables it. heap_insert_free consumes later coalescing tombstones instead of continually growing bookkeeping on splits. Production allocator test passed 10000 varied level-sized batches, live-byte integrity, aligned split-tail reuse and coalescing; table stays at 15 records rather than growing beyond 33000. Extended free bridge test confirms reclaim-enabled release calls HeapFree and preserves adjacent guest slots. Release runtime/canonical exe builds pass (existing runtime compiler warnings).
Visible user verification PID 50000, interactive-heap-reuse stdout/stderr logs. Interactive level-hopping/audio verification remains pending; no claim that all corruption or crashes are fixed.

## 2026-10-12: FPS investigation (60 FPS not reached)
User confirmed previous memory fix appears effective, reported gameplay 7-15 FPS, same scene 10 FPS. Live sample PID 50452: guest main thread mostly sub_001042A0 at recomp_0019.c:2639, waiting for PFB flush; renderer executor busy in software raster/pixel paths, raster workers mostly waiting. Sample-Game.py now resolves source locations to avoid mislabeling optimized static functions as nv2a_pb_exec_report.
Added per-draw fixed-function vertex transform cache. Cache keys 16-bit guest indices, uses per-batch generation and copies complete transformed outputs; programmable path bypassed because shaders can write constants. RECOMP_VERTEX_CACHE=0 disables it. Check-VertexCache.py passed byte-identical outputs across changed draw state, full 16-bit indices, cache-disabled mode and programmable bypass.
User-approved restart PID 24228 used 8 workers instead of manual default 16 and RECOMP_PROFILE_GPU=1. User still observed 10 FPS. GPUPERF gameplay intervals: raster 8103.8/10078.4ms and 8517.4/10016.1ms; vertex transforms only 483.1ms and 493.2ms (included in raster). Thus no measured FPS gain from this first pass.
Second change: immutable exactly-rounded byte/255 lookup, inline nv2a_rc_unpack_fast used for texture samples, preserves external conversion API. Check-TexelConversion.py passed 2000000 color comparisons and all channel bytes exactly; isolated 10M conversion benchmark 21.10ms original versus 10.13ms lookup (2.08x), not a gameplay FPS result. Runtime rebuilt with /Z7 source info for renderer profiling; Release canonical exe relink passed (existing warnings).
Visible second FPS test PID 19784, interactive-fps-texel stdout/stderr logs. User scene/FPS confirmation pending. Heap reclamation and ordered PFB flush fixes retained. Do not claim 60 FPS or a gameplay gain from the conversion microbenchmark.
Second FPS test user reports 11 FPS. GPU timing still ~85% wall time rasterization (8575.5/10012.7ms; vertex 499.5ms included). Source sample in gameplay: xf_rows coverage/pixel loops, rc_texel bilinear interpolation, sample_tex and sample_swizzle_offset, generic combiner read_rgb/map_in and blending. No longer samples external nv2a_rc_unpack on pixel path. This is a modest observed 10 to 11 FPS difference, not a controlled benchmark or 60 FPS success. Closed owned diagnostic PID 19784 after measurements per standing cleanup instruction.
User asks why no GPU rendering: current SD100F linked XDK submits NV2A commands to software nv2a_pb_exec; CPU recompilation does not translate those commands into host GPU shaders automatically. Existing src/nv2a/nv2a_pgraph_d3d11.c generic path explicitly binds no texture and uses vertex color; it is a basic menu prototype with title-specific font-atlas mapping behind GAME_HAS_FONT_ATLAS. No ready switch for this game. Future substantial FPS work should integrate GPU pixel rasterization with existing verified command decoding/transformed vertices, preserving guest render-target identity and read/write synchronization, swizzle layouts, register combiners/alpha/depth/blending, shadow rendering and framebuffer presentation. Avoid enabling the independent PFB-flush acknowledgement regression. FPS objective remains outstanding; do not keep doing micro-optimizations without measuring their contribution.

## 2026-10-12: experimental D3D11 raster backend
User authorized porting rendering to GPU. Added kernel/nv2a_gpu.h/.cpp and Windows D3D11/D3DCompiler build dependencies. RECOMP_GPU=1 enables the experimental backend; default remains software. Existing command decoding, vertex transformations, clipping and culling remain on CPU. Hardware performs covered triangle rasterization, depth, blending, alpha tests, fog and register combiners with four texture stages, palette decoding and clip-plane texture modes. Cube textures, swizzled/16-bit render targets and query/diagnostic draws retain software fallback. Synchronize GPU color/depth to guest/CPU memory before software writes, clears, target switches, overlapping texture reads and presentation. This is a hybrid backend, not a finished full-GPU port.

Visible same-scene comparisons: initial GPU and general single-texture combiner builds remained about 10 FPS; multistage build improved to user-estimated 25 FPS average. Logs: interactive-gpu, interactive-gpu-combiner, interactive-gpu-multistage. Completed owned sessions were closed. Multistage counters showed no cube or texture-decode fallback in this scene; special surfaces still use CPU rendering. Do not claim 60 FPS or exact benchmark results from user estimates.

Next build retains the same depth-buffer identity across depth-test toggles and reuses larger targets for smaller clip regions. Zeta changes synchronize before CPU depth-buffer reuse. Visible owned PID 50680, interactive-gpu-retained logs, awaits user FPS/rendering comparison. Its logs still show approximately 570 texture uploads/second and 150 readbacks/second in the sampled scene. Texture cache now replaces changed contents for the same guest texture key instead of filling all 128 entries with historical versions. Hardware regression passes 140 animated updates while retaining static textures; runtime library rebuilt. This cache change has not yet been linked into the running canonical executable or measured in gameplay.

Check-GpuRaster.py uses actual hardware D3D11 offscreen rendering. Passed color coverage, ordered depth, alpha blending/discard, decoded texture quadrants, CPU/GPU synchronization, 100 CPU/GPU combiner comparisons, two-texture modulation, clip-plane discard, target retention across depth/clip changes and animated cache updates. HLSL compiler still emits X4000 about potentially uninitialized combine data despite initialization and passing parity cases; investigate before declaring broad shader correctness. Remaining work includes synchronization/target allocation costs, state caching, GPU-only render textures/presentation, coverage beyond this scene and measured FPS. Keep independent PFB flush disabled and preserve existing memory, shadow and dialogue fixes.

Retained-target comparison: user reports about the same FPS. Closed owned PID 50680 after that test; linked canonical sd100f.exe with the animated texture-cache fix and restarted visibly in interactive-gpu-cache. Gameplay cache/FPS comparison remains pending.

Texture-cache comparison: user estimates 26 FPS average. Sample intervals show about 127 uploads/second versus roughly 570 previously; readbacks remain about 178/second. Closed owned PID 30692 after measurement. Next change reuses matching color/depth targets and staging resources after synchronization, uploading refreshed CPU depth through writable staging plus CopyResource (UpdateSubresource is forbidden for depth-stencil targets). Hardware regression and canonical Release build pass; resource identity is retained across synchronization. Visible interactive-gpu-resource-reuse test launched; FPS/rendering confirmation pending. Existing shader X4000 warning remains.

Resource-reuse comparison: user estimates 27 FPS. Sample gameplay interval: 5956ms raster including 1457ms vertex transforms per 10015ms wall; approximately 5870 batches and 178 readbacks/second. Main-thread IP sampling still mostly recomp_0019.c:2639 (renderer flush wait). Added bounded 128-entry descriptor caches for sampler/blend/depth state objects, retaining independent references for current bindings. Hardware regressions pass unchanged outputs and stable cache sizes across 140 repeated draws. Added cumulative GPU-TIMING prepare/submit/sync counters (prepare includes nested sync; do not sum blindly). Closed owned PID 4448; canonical build passed and interactive-gpu-state-cache launched visibly. Gameplay comparison and timing breakdown pending; no claim of 60 FPS.

State-cache comparison: user estimates 28 FPS, occasionally 29. Stable sampled interval: ~5963ms raster including 1422ms transforms per 10013ms wall; backend prepare delta 1459ms, submit 126ms, sync 781ms (sync partly nested in prepare). Cached states stable at 4 sampler / 7 blend / 4 depth. Moved CPU perspective texture interpolation, stage-coordinate/fog preparation and depth lookup below the GPU early return; software behavior retained. Added zero-dimension guard for unused GPU stage-0 coordinates. Coverage harness had a stale extraction boundary that included vertex-cache bookkeeping; corrected it to extract only the two quantization statements. Coverage and screen-W/clipping tests pass; Release runtime/canonical exe build passed. Closed owned PID 34788; interactive-gpu-triangle-prep is the next visible comparison. FPS gain from this change remains unverified.

Triangle-preparation comparison: user reports 28-30 FPS. Sample raster remains ~5945ms including 1443ms transforms per 10020ms wall. Added clipping position-only classification before full attribute copies, common-plane rejection and existing crossing-polygon fallback. Expanded clipping test with wholly left-of-screen triangle; tests and Release build pass. Added interval GPU-CPU bridge/hash and geometry/fallback timing to locate remaining CPU cost. Closed owned PID 17352; interactive-gpu-clip-fast launched visibly, comparison pending.

Clipping comparison: user confirms rendering looks good and reaches 30 FPS while moving, but reports transient 1 FPS for about one second, possibly also stationary. Sample raster ~5031-5178ms/10s, transforms ~1316-1353ms, bridge/hash ~2108-2183ms, geometry/fallback ~1509-1566ms. Added GPU-STALL individual >=50ms prepare/submit/readback durations and FRAME-STALL >=200ms presentation gaps with GetTickCount64 timestamps. Removed unnecessary nv_gpu_sync from dump_surface_bmp when RECOMP_FB_DUMP is absent. This eliminates a diagnostic-only readback but is not established as the hitch cause. GPU harness and Release builds pass. Closed owned PID 42708; interactive-gpu-stalls launched visibly for reproduction. Hitch diagnosis/fix and 60 FPS remain outstanding.

Stall reproduction: gameplay FRAME-STALL gaps 485,453,469,469ms at 655992375,656002359,656012390,656022406ms respectively, matching 10-second reports; no GPU-STALL operation >=50ms accompanying them. This points to synchronous detailed reporting before DMA GET acknowledgement, not proven GPU readback stalls. GPU reports now return after compact counters/timings and explicit peek/find/screenshot diagnostics unless RECOMP_GPU_VERBOSE is set; software detailed reporting remains unchanged. Release builds pass. Closed owned PID 25536; interactive-gpu-compact-report launched visibly. User must verify recurring hitches gone; no completed hitch fix claim yet.

Compact-report verification: user confirms the specific recurring FPS drop appears resolved; occasional dips to ~25 FPS remain. Sample workload varies with transforms ~1325-1612ms and geometry/fallback ~1691-2344ms per 10s, bridge/hash ~2105-2205ms. Expanded GPU coverage to 32-bit swizzled targets through production surface_pixel_offset callback: uploads linearize guest contents and readbacks restore Morton order; target identity includes layout, overlap barriers use full swizzled extent. CPU clears and 16-bit/cube/query fallback remain. Hardware GPU tests verify all checker pixels in Morton order plus CPU-written masked target preservation; existing swizzled-layout test passes. Initial new checker assertion used a disabled texture stage; corrected harness stage_program=1, then all passed. Release runtime/canonical builds pass. Closed owned PID 9060; interactive-gpu-swizzled launched visibly, shadow shape and FPS verification pending. Not a direct GPU shadow-texture sampling path yet: guest synchronization remains at target switches.

Swizzled GPU verification: user confirms correct Scooby shadow, range 28-31 FPS. All reported fallback categories zero in this scene. Sample raster ~4968ms including transforms 1347ms, bridge/hash 2575ms, geometry 958ms per 10008ms wall. GPU sync ~1014ms/interval, prepare ~1782ms, submit ~82ms (nested sync caveat). Added bounded pool of eight idle target resource groups keyed by dimensions, synchronized before pooling and CPU color/depth contents refreshed on reuse. Hardware regression passes ten alternating 32/64 targets, correct color output and original 64 target resource identity; existing Morton/CPU-GPU checks remain passing. Release builds pass. Closed owned PID 18248; interactive-gpu-target-pool launched visibly, FPS/rendering result pending. CPU transforms, command handling and guest readbacks still limit progress toward 60 FPS.

Target-pool comparison: user sees lows ~27 FPS in prior areas and ~15 FPS at a new view in the scene; user held that view for sampling. Interval raster ~6050ms (transforms 1627ms), bridge/hash 2200ms, geometry 2036ms per 10s; no reported fallback. Native samples main thread mostly recomp_0019.c:2639; renderer source locations absent and nearest public symbols ambiguous. Sample-Game.py now separately lists active samples so idle worker threads do not hide hotspots. Added Release /Z7 source properties for nv2a_pb_exec.c, nv2a_pb_scan.c, nv2a_vsh_interp.c and nv2a_gpu.cpp while retaining /O2, so future linked PDB supports source profiling. Runtime and canonical link passed. Closed owned PID 15188; interactive-gpu-source-profile launched visibly. This is diagnostic instrumentation, not an FPS fix; new 15 FPS view source sampling pending.

Held low-FPS source profile: renderer thread 30724 had 86/376 samples attributed to nv2a_pb_scan.c:241 (walker/inlined decoding), followed by clipping (25+22 samples), attributes (21), interpreter (20), triangle processing (16), textures (16); main thread 331/376 at recomp_0019.c:2639. Walker called note() for every method even without RECOMP_PB_SCAN, doing a linear diagnostic inventory lookup. Gate note on cached s_scan_enabled; command execution and health totals remain active. Extended FIFO test verifies survey disabled emits all commands with zero inventory calls, enabled counts methods through CALL/RETURN; wraps/corrupt bounds tests pass. Runtime/canonical builds pass. Closed owned PID 24456; interactive-gpu-fifo-survey visibly running with RECOMP_PB_SCAN cleared. FPS gain not yet measured; survey overhead is a concrete optimization candidate, not proof that all sampled walker cost was inventory.

FIFO comparison: user reports first-level lows 28 FPS, not explicit confirmation of the old 15 FPS view. Follow-up sample walker no longer in top active hotspots. Main loop now has substantial samples at recomp_0004.c:2658/2667: original sub_0005DA70 waits until refresh counter 0x214B2C minus last 0x1B8AD8 >=2. Counter is incremented by callback sub_000AD9E0. At emulated 60Hz this enforces roughly 30 FPS pacing even when rendering finishes sooner. Added opt-in RECOMP_TITLE_60FPS=1 selecting one tick; absent/0 retains original two. Moved sub_0005DA70 into persistent title_overrides.c, preserving body except threshold and host opt-in initialization/logging. Regeneration --exclude-manual must retain this fifth override (0x5DA70 in addition to 0x740D0/0x109530/0x6CEB0/0xBDD70). Initial link encountered still-running owned exe; closed owned PID 24688 then link passed. Visible interactive-gpu-one-tick launched with flag1; FPS, simulation speed, audio/dialogues and transitions need user verification. Do not claim native title is 60 FPS or boost refresh clock to fake FPS; this is explicit higher-FPS title pacing override.

One-tick pacing comparison regressed to user-estimated 25 FPS. Backend submit delta rose to ~2845ms/10s versus prior ~160-190ms; raster ~8908ms including transforms 1688ms, bridge2300ms, geometry2050ms. This indicates submission backpressure, not a successful 60 FPS unlock. Changed vertex upload to a dynamic streaming buffer (minimum32768 vertices), WRITE_NO_OVERWRITE appends and WRITE_DISCARD only on wrap/growth; Draw uses the correct start vertex. Hardware regression verifies 6000 queued draws across wrap without readback, retains earlier left-half red geometry and final right-half green; all existing checks pass. Release builds passed. Closed owned PID 42304; interactive-gpu-vertex-stream visibly launched with RECOMP_TITLE_60FPS=0 restoring original two-tick pacing. Streaming gain and renewed higher-FPS pacing viability remain unverified. Do not ship one-tick pacing as default.

Streaming baseline: user reports same area 30 FPS with original two-tick pacing. Submit delta ~196-201ms/10s; raster ~6949-7050ms, transforms1841-1862ms, bridge2631-2682ms, geometry2269-2292ms. Cannot attribute recovery to streaming alone because pacing was also restored. Closed owned PID 27548; launched identical canonical executable as interactive-gpu-stream-one-tick with only RECOMP_TITLE_60FPS switched to1 for controlled comparison. No rebuild or alternate executable. FPS, submission time and speed validation pending; two-tick default remains intact.

Controlled one-tick streaming comparison: user reports 41 FPS stationary but gameplay feels wrong, likely coupled to 30 FPS updates. This establishes higher rendering throughput in this view but rejects naive one-tick pacing as a gameplay-compatible high-FPS fix. Closed owned PID 39872; restarted visibly in interactive-gpu-original-pacing with RECOMP_TITLE_60FPS=0 and state trace disabled. Original two-tick pacing remains default; GPU optimizations retained. Higher display FPS requires investigation and decoupling of simulation/rendering, possibly interpolation, rather than running all title updates more often. Do not assert native60FPS based on initial user expectation. Correct-speed confirmation pending.

Original pacing verification: user confirms movement and FPS look good after restoration. User wants to transition to another level for FPS coverage. Preserve owned visible PID 35012 (interactive-gpu-original-pacing) during this ongoing user test; do not close/restart until level measurements conclude. Next-level FPS, fallback, correctness and loading measurements pending.

Next-level test: user reports 25-29 FPS. No reported software fallback. Sample interval raster8493ms including transforms2531ms, bridge3097ms, geometry2635ms per10017ms wall. Active source sampling shows vertex interpreter26/237, clipping19+17, bridge14 and triangle12. Added nv2a_vsh_cacheable(): CXT writes disabled permits cache; enabled scans reachable instructions up to FINAL and rejects any constant output mask; unterminated program conservative when writes enabled. Per-draw transform cache now includes pure programmable mode2, retaining constant-writing/mode1 bypass and generation invalidation. Check-VshCacheability.py executes an actual constant-writing shader and verifies classification, start-slot changes and instruction replacement. Extended Check-VertexCache.py verifies byte-identical pure shader caching and disabled mode; corrected extraction to stop at cache function end rather than including new GPU helpers. Tests and runtime Release build pass. Current user session PID35012 preserved in next level; canonical executable not yet relinked because running session locks it. Restart approval pending to avoid disrupting ongoing play. Original two-tick pacing retained; FPS gain from pure shader cache pending.

User authorized restart for shader-cache comparison. Closed PID35012, canonical sd100f.exe relink passed and interactive-gpu-shader-cache launched visibly with GPU1, original pacing (TITLE_60FPS=0), vertex cache enabled and compact timing logs. Next-level FPS and visual correctness await user reproduction.

Shader-cache user verification: same previously 25-29 FPS level now 29-30 FPS. Logs show transforms1546-1600ms/10s versus prior2531ms; scene/workload not a strict benchmark, but timings support observed gain. No reported fallback in tested view. Closed owned PID33872 after measurements. SD100F main now defaults RECOMP_GPU=1 only when unset; explicit0 preserves software path, generic runtime remains opt-in. Original two-tick title pacing remains default. Canonical Release build passed. Visible default startup smoke test PID2104 with GPU/pacing overrides unset confirmed hardware initialization and two-tick title pacing; closed owned smoke test after checking. Logs gpu-default-startup. Higher FPS gameplay decoupling, broader level validation and shader X4000 remain outstanding; do not claim 60 FPS reached.

User now reports subtle backward movement jolts/stutter, uncertain whether original-pacing run or earlier41FPS. Treat movement correctness as unresolved despite previous positive verification. No game process was running at start of investigation. Launched visible interactive-movement-cache-off with GPU1, original pacing TITLE_60FPS=0, VERTEX_CACHE=0; isolates cache with lower-FPS confound noted. No source rollback yet; current canonical shader cache remains default. Need user classify affected run and compare movement before attributing cause. Test remains open pending feedback; close owned process after comparison.

Movement comparison result: user unsure which recent run changed movement and whether it differs from console; cache-disabled test reports28-30FPS, no definite disappearance or reproducible regression established. Do not claim cache regression or console-normal motion. Original two-tick pacing stays default, shader cache/GPU changes retained. Closed owned PID28468 after comparison. Any further movement diagnosis should measure ordinary 33/50/67ms frame cadence or compare a repeatable motion clip/reference; current >=200ms stall logging cannot exclude smaller cadence jitter. Canonical executable unchanged by cache-off launch and still defaults GPU1, original pacing and cache enabled.

Debug/testing options implemented: canonical sd100f.exe accepts standalone --nofmv and --noaudio; --debug gates --level <four-character scene ID>, --loadsaveslot <1-3>, --trace, --profiler and --fpsgraph, independent of argument order. Debug title suffix and pause overlay register before window startup. Pause F1/controller Back/click Debug opens level selection (87 installed HIP scenes), 15 title power-up bit toggles, one-frame tracing, ten-second renderer profiling and FPS/frame-time graph. Window callbacks queue guest actions; title-thread hooks preserve guest register/XMM/x87 state. Native cleanup/load/setup used for warps, clock reset and gameplay substate1 restored; state0 was found to exit to menu and corrected before final validation. Power-up bits survive debug warps. --noaudio now silences monitor output after mixing, preserving completion/event timing instead of suppressing voice playback.

Validation: production parser checks pass for debug gating/order, save+level combinations, strict slots/IDs and invalid input. Existing GPU hardware raster and voice resampling regressions pass. Visible canonical runs verify H001 direct startup without cin1, W020 direct startup, real save slot1 restore alone and combined with W020, pause power toggle and graph, H001->H002->H003->I001 warps retaining power bits and gameplay mode11/substate1, one-frame trace rearming after a startup trace, and complete ten-second renderer profile interval. Actual debug window title reads "Scooby Doo - Night Of 100 Frights [DEBUG] | FPS: 30.0 | draws: 299". Standalone --nofmv reaches New Game/Load Game choices without keyboard input or opening XMV files; a short Start pulse through the normal input backend bypasses the title prompt after UI initialization. Earlier immediate menu state1 change was ineffective and removed. Muted output dump contained 58,953,728 zero bytes; owned PCM test artifact removed. Test processes closed after checks. Final captures/logs retained under analysis/debug-*-final.png and debug-final-test.log/debug-combined-test.log. Controller navigation and the full 87-scene catalog were not exhaustively play-tested. Original two-refresh-tick gameplay pacing and default GPU/cache behavior remain unchanged.

Regeneration: preserve SIX functions in src/title_overrides.c using --exclude-manual: 0x740D0, 0x109530, 0x6CEB0, 0xBDD70, 0x5DA70, and new movie wrapper 0x5B620. Its generated definition was removed from recomp_0003.c; declaration/dispatch remain. Startup/menu/frame hooks are in the persistent overrides. Usage documented in DEBUG_OPTIONS.md.

Football-helmet report: user confirms charge animation works but monsters are not hit/killed. Static audit found the player charge selects EBX=0x24BF0 and calls sub_0001A880's frontal-contact filter, but 0x24BF0 was absent from generated declarations/dispatch/body. Recovered all 148 original bytes (0x24BF0-0x24C84) with the existing translator into persistent title_overrides.c; recomp_lookup_manual resolves that VA. Original callback delegates type0x24 contacts to sub37500, otherwise calls native monster damage sub4CC20, updates player position and hit flag1BF05C, slows charge speed, and runs hit feedback. Added a static seed for future recovery. --exclude-manual must now preserve SEVEN overrides, including new 0x24BF0; manual lookup registration remains necessary until regenerated dispatch includes the seed. No new game-speed changes.

Check-HelmetContact.py extracts actual production frontal filter, restored callback and manual lookup. Tests pass for frontal damage delivery/hit response/feedback, behind/vertical/degenerate/non-charge rejection, damage refusal, special-type delegation, guest stack/x87 balance, and opt-in logging wrapper. Native damage implementation is mocked in this check, so this does not prove live monster death. Canonical Release build passes. RECOMP_COMBAT_TRACE=1 logs [COMBAT] target/contact/type and hit flag transition on restored callbacks. Visible owned PID24844 launched --debug --loadsaveslot1 --nofmv; fresh log confirms saved H001 gameplay mode11, saved powerbits0x2000. Real monster encounter verification pending; do not claim the reported bug fully fixed yet. Preserve this run during the requested user test, close it after test conclusion. Logs overwrite debug-render.log on each new debug launch.

User confirms football-helmet attack fix works. Future gameplay/visual checks should be performed by the user; request targeted checks rather than navigating gameplay automatically.

Window close fix: framebuffer WM_CLOSE/WM_DESTROY now transition to terminal state -1 instead of restartable state0. Subsequent display/flip startup requests cannot create another window during cleanup. The message loop checks closure before drawing again, releases its DC/window/RGB buffer and exits the host process with code0, matching firmware shutdown behavior. Check-WindowClose.py exercises the production handlers and 10,000 restart requests without running gameplay; passed. Canonical Release sd100f.exe rebuilt successfully. Actual X/Alt+F4 verification left to the user.

Display resolution Options entry: title_display.c registers an ordinary menu overlay independently of debug callbacks/title suffix. Native main Options is mode5 (original 5F2F0 mode switch maps to event0x7A); paused Options is mode7/substate1. Guest main/game frame hooks publish eligibility without guest mutation. Bottom-right Resolution entry opens via mouse, R, or controller Y; arrows/D-pad select, Enter/A apply, Esc/B return. Separate menu input capture prevents debug-overlay release from leaking input. Window presets640x480 through2560x1440 save Width/Height in display.ini beside canonical executable, load on startup and resize on UI thread. Fixed guest framebuffer dimensions retained; aspect-fit presentation scales game/overlays and inversely maps mouse clicks. Maximize/manual resize work; large presets may exceed a small desktop. Higher internal GPU rendering resolution remains future work, per user's accepted first scope. No gameplay/FPS/pacing changes.

Check-DisplayResolution.py passes exact client sizes for every preset, INI writes, aspect fit, 1280x720 pillarbox geometry, scaled click coordinates/black-bar exclusion, invalid index handling, independent input capture ownership, and main/pause Options gating/key interaction. Check-WindowClose.py still passes. Canonical Release sd100f.exe rebuilt. No gameplay automation or game launch performed; user should verify Options appearance, selection via controller/mouse, resize, return controls and persistence in the actual game.

Resolution entry visibility correction: live user main Options capture options-missing-resolution.png and guest memory show mode2/state0, not generic mode5. Main-menu UI uses asset visibility while retaining Title mode. Confirmed MNU3 OPTIONS VIBRATION/MUSIC/SFX UI-font entities (IDs6855A343/8F2D6816/41F6FF46) have xEnt visibility byte+0x18 bit1 set on this page; scene UI-font count+0x100, contiguous array+0x1AC, stride0x100. title_display_frame now checks these visible native buttons in mode2 alongside generic/paused Options. No cached entity pointer survives scene transitions. Extended production regression covers actual mode2/state0, every candidate row, hidden rows and Start Options exclusion; passed. Canonical Release rebuilt after closing inspected menu-only PID43532. Prior note assuming main Options mode5 was incorrect. User should verify corrected entry appearance/interaction in game.

Follow-up resolution visibility fix: prior byte+0x18 assumption was wrong. Captured live MNU3 OPTIONS VIBRATION header at guest014D1210 has byte+0x14=1 and byte+0x18=0. Original native event show/hide handlers at7237F/7238D set/clear bit1 of byte+0x14. Corrected main_options_visible accordingly. Regression now includes exact captured32-byte header and rejects visibility inferred from+0x18; passes. Canonical Release rebuilt after concluding inspected PID13960 menu test. Actual menu appearance awaits user check.

User confirmed standalone Resolution overlay works, then requested native main-menu integration. Native MNU3 hidden Sound Mode font row F7B264FC is reused as fourth Resolution row at assetY260 with original font, native focus and blinking cursor. TEXT asset ID comes from UIFont asset+0x80, not+0x84 (text color); native xSTFindAsset9CA10 helper preserves all guest registers/XMM/x87. Original text is replaced in loaded guest asset only; installed HIP files unchanged. Two reusable 1024-byte host-managed guest heap buffers hold copied links, preserving native Back/focus/sound links, disabling obsolete Sound Mode A camera/timer links, and adding SFX Down->Resolution and Resolution Up->SFX. Original Resolution Down links wrap to Vibration. Native cursor sound UI A13417C3 moves150px with the label. Ordinary Options row visibility governs new row; old main footer hidden when native installation succeeds. Resolution selector remains the host display page; paused Options retains footer. Manual wrapper7C490 intercepts A/event63 for this native entity, delegates all other callbacks, preserves cdecl guest return balance and queues selector opening on UI thread. No synthetic/out-of-code callback address.

Native production regression passes row text/position/cursor, copied/appended navigation, original A suppression/Back preservation, idempotent installation, visibility, event targeting and wrapper stack balance; prior helmet check passes. Canonical Release built. Startup live log confirms [DISPLAY] Native Options Resolution row installed. Visible PID18000 running --nofmv; user asked to verify native row navigation/A/open/change/B-return. Do not automate gameplay navigation; test still pending.

Native Resolution follow-up: user reported the row still said Sound Mode and A froze before pausing troubleshooting. Corrected title_display_find_asset to call asset-data lookup9CAE0(id,sizeOut=0), preserving guest registers;9CA10 is the asset-name lookup and was incorrect in the earlier note. Added native selection and selector-open logs to distinguish callback/queue failures. Production display regression and Release build passed. Visible canonical PID44224 launched --nofmv for user verification; label and freeze behavior still await confirmation.

Native Resolution input lock: diagnostic PID46820 received focus event79 but no A event. Live row014D0110 flags+F0 were0x36, missing native input-enable8. Native7B4A0 skips rows lacking8 even when focused2, so selection was trapped on the formerly hidden Sound Mode row. install_native_row now enables8 alongside visible+14 on Options, clears8 when hidden, preserves all other UI flags. Regression uses captured initial0x34, asserts input eligibility after focus and disable-on-hide; passes. Release rebuilt after concluding diagnostic session. Actual A/Back behavior awaits user verification.

User confirmed native Resolution A/apply/B works. Requested selector on the CRT rather than a black panel. Removed popup background, moved choices to CRT area with green Terminal text and updated click bounds. Guest frame temporarily hides four native Options font rows while selector is open, restores saved visibility before eligibility checks and on return; original scene/date/buttons remain. Hidden footer click region disabled when native row installed. Display regression and Release build pass; visual verification pending.

User confirmed CRT selector layout and requested native flashing cursor instead of >. Removed > prefix and guest frame now moves existing Sound/Resolution cursor image A13417C3 to100+25*selection while open, restores260 on close. Original animation and visibility remain native. Release build passed; user visual check pending.

User confirmed native flashing cursor works; requested true16:9 wider view. Added Aspect ratio toggle after resolution presets, A/Enter/click toggles and persists Display/Widescreen in display.ini. Presentation fits16:9 when enabled. Persistent A0260 main camera FOV override widens horizontal view-window4/3 before native BDE40 rebuilds projection/frustum, preserves vertical FOV and shadow/offscreen camera paths. Original FOV routine moved from generated chunk0009 to title_overrides for regeneration exclusion. Production test exercises both native aspect branches, on/off/on restoration without cumulative scaling, cdecl stack/x87 balance, saved setting and1280x72016:9 viewport. Tests and Release build pass. HUD/menu proportions and actual world framing await user check.

User confirmed widescreen works in the rebuilt game. Widescreen feature accepted; owned visual test PID14128 concluded. HUD-specific verification was not separately reported.

Keyboard controls requested: WASD left stick, left Shift RT, left Ctrl white/LB, Escape Start, Space A, J X, K Y, L B, U LT, I black/RB. Input now enabled by default unless RECOMP_KEYBOARD=0, focused framebuffer key states merge with pad. Window records left/right Shift/Ctrl from scancode/extended bit; focus loss clears state. Removed conflicting old A/S face buttons and IJKL right-stick mappings. Arrows remain D-pad, Enter Start, Z/X A/B aliases. Resolution/debug selectors accept WASD/Space/L; resolution capture release includes Space/L. Display checks and Release build pass. Gameplay verification left to user.

User corrected shoulder bindings: I now maps white/LB; left Ctrl maps black/RB. Source and keyboard documentation updated; Release rebuilt successfully. Previous keyboard test PID31544 concluded.

S005 post-cin8 soft lock: live debug run9572 hit direct stub135D0 and unresolved indirect53960 while loading S005. Screenshot s005-locked.png shows Mastermind Unmasked Part2 card. Recovered original135D0-136C0 generator initialization and53960-53A58 boss callback using translator seeded function boundaries. Both persistent bodies added to title_overrides,135D0 stub removed,53960 registered in manual dispatch, runtime seeds recorded. Original calls/link setup and jump-table branches preserved. Release build passes; post-FMV recovery awaits user check. Locked diagnostic run concluded.

User confirms S005 now loads after the FMV with recovered135D0/53960 routines. Post-FMV soft lock verified fixed by user. Owned test PID26632 concluded.

W028 direct debug launch PID43420 crashed after FMV: unresolved4DFD0 callback repeated before access violation in8B600 from549AD through entity+DC pointer949494FF. Original callback4DFD0-4E08D recovered with enum jump table and type-specific46950/43230 or46800/43050 calls, persistent title override and manual dispatch registered. Runtime seed added; Release build passes. Crash log archived analysis/w028-before-fix.log; dump43420 remains in Release. Fix hypothesis requires user post-FMV check.

W028 user confirmed post-FMV recovery, then kept run29052 for boss testing. O008 later crashed in1016C0 rep-movsd with corrupted stack/code-like registers. Log shows unresolved525B0 twice, thenE9E00/EA2C0 and null icall. Preserved analysis/o008-before-fix.log and Release dump20261012-171136-29052. Recovered original boss-state525B0-527BA and renderingE9E00-EA2B8/stateEA2C0-EA3BD callbacks into persistent title_overrides, manual indirect registrations and runtime seeds. Translation contains no unsupported markers; Release build passes. This is a candidate fix; actual post-FMV crash recovery awaits user verification.

User confirms O008 post-FMV crash is fixed with restored525B0/E9E00/EA2C0 callbacks. Owned verification run PID38556 concluded.

Lightning stun investigation: user confirms animation plays but monsters not stunned with saved/debug ability. Run35616 has no unresolved calls. Live powerflags1BF2E0 repeatedly3FFF (bit14 clear); user reports upgrade enabled, so do not assume cause from pause snapshot alone. Native1B340 checks bit14 before sets expanding-wave radius1BF09C; player update2B392 range/height filter calls4CF30 which sends event1E through46790. Moved1B340 and4CF30 to persistent overrides unchanged except [STUN] activation flags and hit target/duration logging. These overrides must be excluded on regeneration. Diagnostic build/test pending.

User reports lightning stun works in diagnostic run44140. Trace confirms activation flags00007FFF (bit14 ON), hit callbacks delivered to0153A6D0 and0153AE60. Earlier failing run35616 snapshots00003FFF lacked bit14; difference is consistent with upgrade activation, not proven logging fix. No gameplay behavior was changed by diagnostic build. Archived analysis/lightning-stun-working.log; owned test44140 concluded. Retain distinction between observed working and a confirmed code defect fix.

### Enemy mesh index capacity (2026-10-12)
- World-frame trace exposed an animated draw cut off at exactly 4096 indices. Raised the executor batch capacity to 65536 and added a warning if that limit is reached.
- Expanded Check-ArrayElement.py to verify 12001-index strips through both index methods and 8192-vertex DRAW_ARRAYS runs. Checks and Release build pass.
- User visual confirmation pending for missing werewolf/boss body parts and stun particles. Original trace archived in analysis/enemy-world-before-capacity-fix.log.

### Stun particle atlas UVs (2026-10-12)
- User confirmed index-capacity fix restores Black Knight body; stun particles remain broken across monsters.
- Targeted frame diagnostics show star/spiral texture 80BFF380 is intact, but incoming horizontal UVs are infinite/NaN.
- Original 71FDB is byte IMUL DL: signed AL*DL writes AX and preserves DL. Lifter incorrectly wrote EDX:EAX, zeroing the atlas column count used immediately by 71FE7 DIVSS.
- Corrected one-operand byte/word IMUL lifter register writes; moved 71E80 to persistent title_overrides with the correct byte multiply. No renderer UV sanitization workaround.
- Check-ParticleAtlas.py production arithmetic passes 64 grids; 3 translator width tests pass. Release rebuild passes. User visual verification pending.
- Before-fix trace: analysis/stun-invalid-uv-before-fix.log. Optional RECOMP_FRAME_TRACE_PARTICLES adds small blended-triangle raw vertex/UV diagnostics only during a frame trace.

User verification: stun stars/spirals look correct in rebuilt run27388 after the byte IMUL/particle-atlas initialization fix. Black Knight missing body parts and shared monster stun visual defect are now user-confirmed fixed.

### Gameplay mouse confinement (2026-10-12)
- Added generic display-thread cursor confinement/hiding request. Title frame hook requests it only in gameplay mode11, before the debug flag check. Verified live map menu mode7/state2; main menu mode2 and pause/map menus release the cursor.
- ClipCursor uses screen-space client bounds; WM_SETCURSOR hides the pointer without modifying ShowCursor counters. Focus loss, minimize, hidden window, menu/debug capture, move/resize and close release confinement.
- Check-CursorPolicy.py production backend checks and Check-WindowClose.py pass. Code compiled; final link awaiting current session close and visual user verification.
Cursor-policy Release executable linked successfully and restarted visibly for user verification. Particle diagnostics disabled for this run.
User verification: gameplay cursor hiding/confinement and release in Pause, Map/warp gate and Alt-Tab work as expected in run45564.

### Native mouse menu actions (2026-10-12)
- Captured pause font bounds and warp icon navigation links in the live game. Native UI events 79/80 select/unselect font rows; map events 94/95 also show/hide selection images. Activation uses event63, or the native event66 Back label.
- Window thread receives copied hit bounds and queues clicks; title thread validates current scene/page eligibility and dispatches original propagated events with guest registers preserved. Hidden warp selection icons are reachable only through the current page navigation graph.
- Check-MenuMouse.py passes focus/activation, stale-row and gameplay rejection, overlay priority and hidden nested-page isolation. Title sources compile; executable relink and user visual verification pending.
Release build linked successfully; restarted visibly as PID23768 for user mouse-menu verification.

### Mouse hover and nested warp correction (2026-10-12)
- User found Options selection inaccurate and nested warp pages limited to selected icon. Captured nested root55D10828: navigation links but no63 action, so prior hit filter excluded the root and stopped graph discovery. Navigation-only nodes now seed/extend the current page graph and clicks focus them without inventing activation actions.
- Options text boxes60px tall overlap50px row spacing. Font glyph height asset+96 (24px in Options) now determines row hit height with6px padding. Resolution native row overrides preserved event66 Back links with63 activation.
- WM_MOUSEMOVE feeds copied hit-map hover requests; title thread changes native focus only when target changes, without activating. Clicks take priority over queued hover. Back labels activate directly without focus events that can close map groups. Resolution selector follows mouse hover too.
- Check-MenuMouse.py expanded checks pass; Check-CursorPolicy.py passes. Release rebuilt and launched visibly asPID29908; user verification pending.

### Dynamic text button prompts (2026-10-12)
- Captured live warp-gate B prompt and identified rich-text token $A112024016~$C in MNU4. The common native text draw at8CE00 now temporarily substitutes keyboard labels for coloured controller glyph tokens; original assets and retained dialogue text are restored after drawing. Continuation return pointers map back to original offsets.
- Input layer tracks keyboard key-down serials and new player-one controller button/trigger/stick actions before synthetic/overlay early returns. Idle drift, releases, held pad states and the nofmv Start pulse do not reclaim controller prompts.
- Added Check-InputPrompts.py production-input/converter and full draw-wrapper checks: pass. Sources compile; final link awaiting session close and user verification. Replacement currently covers text glyph prompts; button icons baked into UI images are separate assets.
Dynamic prompt Release build linked successfully and restarted visibly as PID39080. Awaiting user verification of B/L, A/Space and controller restoration.
Prompt follow-up: initial manual lookup hook was bypassed by translated direct calls. Moved native 8CE00 into persistent title_prompts.c under title_prompt_draw_original; sub_0008CE00 now wraps both direct and indirect calls. Added integration assertion alongside draw/input regression checks; pass. Corrected Release executable rebuilt for user verification.
Back label follow-up: retained PS2 triangle-style text glyph was incorrectly labelled K/Y. Native menu event66 uses B (0x20000), and user confirmed Xbox Back should be L. Converted legacy menu Back glyph to [L]; added explicit Back regression case. Tests pass; compiled, awaiting active session close for relink.

### Preview Auth0/Discord role gate (2026-10-13)
- Ported example.ps1 authorization-code/PKCE browser login into C, requiring exact role1559332156709404683 from the namespaced Discord-role ID-token claim. Fixed tenant/client and loopback callback settings follow the user's example; no client secret or bot token embedded.
- Added RS256 verification with tenant JWKS, issuer/audience/nonce/iat/expiry validation, fail-closed role checks and exact JSON parsing. Exclusive loopback8765 listener validates state, bounds requests and times out after5minutes. Tokens stay in memory only.
- PREVIEW_BUILD_LOCKING CMake option defaultsOFF; authentication source/libraries and startup check exist only inON builds. Role denial displays whitelist-required error and exits3 before game initialization.
- Offline production-C tests with real locally generated RSA signatures pass approval/denial, tampering, token claims, duplicate/malformed role claims and callback parsing. DefaultOFF executable built successfully; ON canonicalbuild prepared for user login verification. Documentation: PREVIEW_BUILD_LOCKING.md.
Both OFF and ON Release builds linked successfully. Production tests also pass real loopback HTTP callback routing and the standard PKCE SHA256 challenge vector. Canonical build cache now has PREVIEW_BUILD_LOCKING=ON; launched for user browser-login verification.


### Expanded debug testing tools (2026-10-13)
- FPS graph now includes current level ID and Scooby world coordinates, copied on the title thread under an SRW lock for presentation.
- Removed flower pot and diving helmet selector rows, retaining explicit original bit mappings for the remaining 13 abilities.
- Added session-only warp availability, snack requirement/payment, monster damage, and collision-free flight toggles plus native +100 snack awards. Native checks are preserved in title_debug_overrides.c and routed for direct and indirect calls; seed_functions records regeneration exclusions.
- Noclip uses native model animation/end-update and bound updates with camera-relative keyboard/controller flight, no player physics/collision solver. Q/E or LT/RT controls height. Reset clears velocity and prior floor association; ordinary gameplay update remains intact when disabled.
- Separated ten-second detailed render tracing from timing profiling. Both menu actions arm until gameplay resumes and show capture status; render tracing stays active until the wall-clock interval ends at a complete frame boundary. Profile completion flushes the log.
- Validation: Check-DebugFeatures.py covers debug gating, unchanged warp activation state, insufficient-snack payment completion without charges, ordinary payment, monster-only filtering, keyboard/controller flight, focus/overlay suppression, velocity reset and guest-stack balance. Check-DebugCapture.py covers repeat one-frame traces, timed trace lifetime and exact profile delta/report completion. Check-DebugOptions.py passes. Gameplay/visual verification remains for the user.


### Monster-protection correction (2026-10-13)
User found monster protection ineffective. The initial filter incorrectly assumed xBase type 0x29. The native spawned attack constructor writes type 0x19 at 2E8EE; corrected that filter and added exact caller identification for NPC contacts and monster/boss attacks into player damage at 1D970. Attack sources may be records rather than entities, so known attack callers block independently of the source's header. Hazard/material and fall callers retain native damage. Updated focused checks validate the type fixture against original disassembly and cover all 13 monster caller sites plus retained hazard/fall behavior. User gameplay verification pending.

Live monster-protection trace: protection=1, caller2C28D/source01571160 blocked; caller28E8F with the same source bypassed, reducing health2->1. Read-only live header inspection shows type2 at source+4. Generic damage wrapper28E80 is shared, so it remains filtered by source type, now including native NPC body type2 as well as spawned attack type19. Added regression reproducing both routes with the same type2 source, plus toggle-off and non-NPC behavior.
# Audio corruption flight recorder (2026-10-13)

Debug launches retain ten seconds of mixed 48 kHz stereo PCM in a bounded
1.92 MB ring. F8 in the focused game requests a WAV containing that history
and the next ten seconds, plus active VP/mixer voice metadata. Recording and
file ownership stay on the APU thread; the game thread only posts an atomic
request. Debug launches enable existing five-second XAudio2 queue/underrun
reports. Release build passed. Deterministic capture harness verified all
960,000 frames across wraparound and a valid 20-second WAV. Actual corruption
has not yet been reproduced; this adds diagnostics rather than claiming a fix.

