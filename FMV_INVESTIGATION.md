# FMV playback investigation

The reported failure is smooth playback for the first few seconds followed by
stuttering, especially in the new-game movie. Playback continues; a decoder
failure, crash or stopped output has not been established.

## Evidence and presentation change (18 October 2026)

The supported title's new-game movie is `cin1.xmv` (identified by
`title_guest_movie_is_initial_cinematic`). The maintained playback wrapper
`title_guest_play_menu_movie` calls the translated guest routine `sub_000A3330`.
The original translated XMV decoder remains in use. These references come from
existing source/instruction translations, not a fresh Ghidra recovery.

Inspection of `fb_present.c:fb_thread` established that conversion and drawing
were followed by an unconditional `Sleep(16)`. Consequently every iteration
took its drawing time plus the sleep, rather than fitting that work into a
refresh interval. The window now waits against a QPC deadline at 60 Hz, carries
fractional ticks, and resets its deadline after a substantial stall. A Windows
high-resolution waitable timer has ordinary-timer and Sleep fallbacks. The clock
and timer belong exclusively to the window thread and are released there. This
does not change guest decoding, field counters, simulation timing or movie data.

`tests/Check-WindowRefresh.py` exercises the actual pacing helpers with 4 ms
of simulated drawing per iteration, deadline recovery after an 80 ms stall,
and missing-timer fallback. Thirty refreshes took approximately 500 ms.
The display-resolution fixture separately checks composition and input while
publication is blocked.

## Private reproduction and limits

An isolated `build/fmv-probe/game/fmv/wblogo.xmv` copy of `cin1.xmv` allowed
40-second startup probes without selecting New Game or creating a save. The
original movie files were untouched. Probes used hidden windows and `--noaudio`
(muted output, with audio emulation still running). This does not reproduce the
full new-game context or establish visible/audio synchronization quality.

With `RECOMP_OVERLAY_STATS=1`, the baseline reported about 27 observed content
updates per second after its first interval; deadline pacing reported roughly
28–29. This samples changes in PVIDEO surface contents, not decoded frame
counts: partial writes and static images limit interpretation. The improvement
is modest and does not establish that the user's stutter is fixed.

The existing opt-in `RECOMP_FRAMECOUNTER_CLOCK=1` path was also tested, alone,
with a precise clock experiment, and together with deadline presentation.
None established an improvement; the precise field-clock experiment was
removed and the default field-clock policy remains unchanged. Translated
`sub_000FF010` and `loc_000FD227` refer to the registered counter as part of
display/field handling, but its entire timing contract has not been recovered.

File reads continued beyond 13 MB during a baseline probe, which rules out a
complete initial-buffer refill hang in that run. It does not rule out uneven
streaming. A limited thread-instruction sample mostly found waits, without
sufficient attribution to establish decoder saturation or a synchronization
cause. Decoder changes would currently be speculative.

Remaining work is to correlate guest decode/publication boundaries, consumed
movie timestamps and visible presentation intervals during an actual affected
new-game playback. No complete FMV fix or full playback validation is claimed.

## Guest refresh scheduling correction (19 October 2026)

A second investigation sampled the current new-game movie through the private
cin1-as-logo setup, with audio enabled. The sample did not show the translated
decoder consuming the available CPU; reads continued and XAudio2 reported no
underruns. Sampling perturbs timing, so subsequent cadence comparisons ran
without it. No new decoder-layout recovery or decoder replacement was made.

`kernel_vblank_tick` scheduled guest refresh interrupts using `GetTickCount64`.
On this machine, a direct clock-resolution probe observed 15/16-ms uptime
increments, including after a successful `timeBeginPeriod(1)`. Scheduling a
16.667-ms period from that clock produces repeated roughly 31-ms gaps. The
baseline's opt-in clock report recorded 20 gaps over 25 ms in each five-second
window, despite averaging 60 dispatches per second. A correct average rate
therefore concealed uneven refresh delivery.

The guest interrupt scheduler now uses QPC and the tested fractional 60-Hz
deadline helper in `video_refresh_clock.h`. A late poll retains the original
phase; a genuine stall skips missed deadlines without replaying a burst of
interrupts. The existing guest ISR, thread, field-toggle policy and callback
order remain in place. QPC failure retains an uptime-clock fallback. This is
a host-clock correction applying to both graphics backends, not a change to
guest decoder timestamps or the registered device field-counter policy. The
previous precise-field-counter experiment was a different clock path.

`Check-VideoRefreshClock.py` compiles the real helper, reproduces the old
coarse-clock irregularity, and verifies 600 fields over ten simulated seconds,
fractional carry, repeated polls, stall recovery and clock-domain fallback.
The fixture passes under MSVC and portable Clang. Both game builds pass.

The 35-second QPC run reported maximum dispatch gaps of 18-19 ms and no gaps
over 25 ms. An additional 70-second run retained the corrected cadence with
audio enabled and repeated streaming refills. Raw logs and derived counters
are in `build/fmv-current/` (`coarse-clock.log`, `precise-clock.log`,
`extended-clock.log`, `clock-summary.json`). The movie assets used for these
probes are private copies; original game files were untouched.

This removes a reproduced source of periodic judder. It does not establish
perfect visible frame delivery: PVIDEO content fingerprints still vary with
scene content and partial writes, and are not decoded-frame counters. The
probe also presents cin1 through the startup wrapper rather than the exact
New Game context. No full-movie visual or lip-sync correctness claim is made.

With `RECOMP_OVERLAY_STATS=1`, `[VIDEO-CLOCK]` now reports interrupt cadence
alongside the existing `[PVIDEO]` content observations and audio queue counters.
These are observation-only diagnostics with no coherent cross-thread frame
snapshot guarantee. Normal launches do not enable the extra clock reports.

## Competing field counters and writable video buffers (19 October 2026)

The subsequent user report narrowed the problem to video; audio was working.
New observers measure native image completion and window consumption separately.
The large repeated pauses came from two writers to the D3D field counter.
`title_host_initialize_devices` registered device pointer `0010E648`, offset
`1DD8`, with the runtime timer/flip fallback. The linked refresh path also reads
it at `000FD227` and increments it at `000FD23A` according to field parity.
`sub_000FF010` returns that counter to XMV; `00153309` through `001534DA`
compare decoder timestamps against it. The synthetic writer and native parity
adjustments both advanced movie time. Correct interrupt cadence hid this.

With both writers, the private cin1 probe averaged roughly 30 images/s but
paused for up to 359 ms, then caught up. During the gaps, the caller continued
polling roughly every 16 ms and native step calls did not exceed 50 ms. The
window missed 9-19 completed images in several five-second windows. Disabling
registration with `RECOMP_TITLE_FRAMECOUNTER` removed those bursts: 30 images/s
and zero skips. The default now uses the linked refresh path alone. The old
fallback requires explicit `RECOMP_SYNTHETIC_FRAMECOUNTER` diagnostic opt-in.

A separate ownership defect existed in `fb_overlay`: the window read live YUY2
pixels while the decoder could overwrite them. Observers in
`src/overrides/movie_frame.c` and `src/api/title_guest_movie_frame.c` delegate
the original translated routines exactly once. At `0015414F`, the decoder
locks its output; `00108D20` returns pitch and pixel pointer. Status 1 at
`001534FE` follows the conversion in `sub_0015413D`. Only then are pixels
copied into bounded host storage. The window selects a copy matching PVIDEO's
physical source and converts its own immutable copy. Contention retains the
last complete matching image, never writable guest pixels. PVIDEO positioning
and bank registers remain observation-only reads, not an atomic register
snapshot. Both backends use this video handoff; audio queues are unchanged.

CMake renames two generated symbols at compile time so maintained observers
can delegate to their original bodies. Generated source remains untouched.
Future regeneration must retain the delegated bodies as well as the maintained
entry points. The adapter supports this SD100F translation; it is not a new
high-level recovery of the XMV decoder.

`tests/Check-MovieFrameHandoff.py` compiles the actual adapter and snapshot code
with an ABI fixture. It checks single delegation, native stack consumption and
register outputs, no-image results, non-movie locks, ownership after guest
overwrite, mismatched sources and nonblocking consumption during publication.
Both game toolchains build. Before/after probes are in `build/fmv-current/`.
The 70-second Vulkan probe (`frame-fixed-extended.log`) retained approximately
29.98 completed images/s with zero skipped images in every reporting window.
A separate 15-second DirectX check (`frame-fixed-dx11.log`) likewise reported
30 images/s and zero skips. These runs used normal audio and no instruction
sampler. The earlier competing-writer run is `frame-native-counter-baseline.log`.
The native-only probe still contains occasional approximately 62-ms image gaps
measured with the coarse uptime clock. Zero skips and a 30-fps average do not
establish perfectly uniform presentation. This fixture substitutes cin1 into
the startup wrapper; exact New Game context, full-movie visual playback and
lip-sync still require checking.
