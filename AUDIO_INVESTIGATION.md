# Gameplay audio investigation

The reported symptoms are intermittent pops/cracks and longer-running corruption or loss of audio during gameplay, especially E007. Monster sounds are definitely affected; overlapping sounds make the scope unclear. Loading E007 alone has not reproduced this, so a level-loading test cannot validate a fix.

## Pipeline and current evidence

`apu_vp.c` reads guest voice/stream data, resamples and filters voices into mix bins. DSP/monitor processing and the optional software mixer feed 256 stereo frames at 48 kHz. `apu_core.c:mcpx_apu_monitor_frame` groups four chunks into each 1024-frame XAudio2 submission. The backend retains three FIFO buffers and starts after two are queued; waveOut is the fallback.

The backend counts underruns and rejected full-queue submissions, which can produce gaps with otherwise valid PCM. Voice reuse already resets interpolation, ADPCM cache and filter history in `voice_reset_filters`; reuse corruption cannot be assumed merely from the symptom. Stream/block geometry, starvation/completion and persistent filter state remain candidates if the mixed PCM is corrupt. No cause of the E007 failure has been established.

## Capture the gameplay failure

Run `DogGame-AudioDiagnostic.exe --debug` in the installed game folder and play normally. Do not use `--noaudio`. When corruption is audible, press F8 before pausing/leaving, then continue for at least ten seconds. Supply the WAV, matching TXT and `debug-render.log`. The WAV retains up to ten seconds before F8 and ten afterward, before Windows playback. Recording performs file I/O on the APU thread; the pre-trigger samples help distinguish the original failure from recording overhead.

The companion report now includes output queue depth, samples played, submissions, underruns, full-queue drops, submission/start failures, last HRESULT and mixed-frame peak/saturation counts, alongside active VP/software voices. Reporting runs on the APU thread without changing samples, resetting playback or flushing buffers. XAudio2 consumes asynchronously; these observations are not atomic with guest voice state. Last HRESULT is not complete engine/device-error reporting.

Capture UI: F8 and the Debug menu's audio-trace action now share an atomic APU
progress snapshot. The menu action arms while paused and posts its request only
when the existing gameplay-mode gate resumes. The window draws requested,
recording/countdown and saved/failed notices without reading APU buffers or
touching its files. Completion follows actual recorded frames and file closure;
the recorder retains the existing ten-second history plus ten following seconds.
`tests/Check-AudioCapture.py` exercises WAV length, countdown, duplicate requests,
deferred triggering, write/open failures and status text with synthetic mixed
frames. Voice diagnostics and manual keyboard/controller/menu interaction are
outside that fixture's coverage. This adds capture usability, not an audio fix.

Corrupt mixed PCM points toward guest voice data/decoding/filtering/mixing. Clean PCM with bad live playback points toward the output/device/backend path. Waveform discontinuities alone do not establish corruption: the original effects may contain sharp transitions.

## Captures from 18 October 2026

Five captures from process 42248, starting at 17:24:56, 17:26:17,
17:26:43, 17:27:20 and 17:27:57, were inspected numerically alongside
their companion reports and `debug-render.log`. The first retains 3.68 seconds
of history and ten seconds after the trigger; the others contain ten seconds
on each side. All are 48 kHz stereo signed 16-bit PCM.

The captured intervals report zero XAudio2 underruns, full-queue drops and
submission/start failures. Later queue drops in the session log are outside
these intervals. This does not rule out device-side problems, but the captured
counters provide no evidence of output starvation during these recordings.

The WAVs contain full-scale samples and large adjacent-sample changes. The
last capture has a channel change of 59,264 PCM units near 9.623646 seconds,
before the trigger at ten seconds. Recording file I/O cannot explain that
pre-trigger feature. The largest changes do not consistently land on 32-frame
voice-processing boundaries. No listening comparison with original game audio
has established which of these features are unintended, and no cause or audio
fix is claimed from these measurements.

Two limitations were established by reading the reporting implementation:

- `apu_vp.c:voice_process` does not populate several fields printed by
  `apu_core.c:audio_capture_snapshot`, including source addresses, offsets,
  stream/loop flags and block geometry. Their reported zeros are missing
  observations, not evidence of invalid guest data.
- The saturation counter checks -32768 and +32767, while the float DSP
  mixdown clamps to -32767 and +32767. Negative DSP clipping can therefore
  escape that counter. A zero saturation count does not establish headroom.

The next diagnostic boundary is valid per-voice source/format state and
measurements before and after filtering and mixdown, so a discontinuity can
be attributed to a stage rather than guessed from the final waveform.

## Follow-up implementation

The maintained PCM reader changed the container selector when reading a stream
segment, but retained the byte width calculated from the voice's initial
selector. A synthetic byte-container voice followed by a 16-bit PCM segment
reproduced incorrect samples. Recomputing the width after the existing segment
override fixes that fixture. Mono/stereo tests check sample/channel order,
offset advancement, end-of-buffer completion and exactly one stream-segment
notification. This is evidence from the runtime source and a reproducible
fixture, not a fresh Ghidra recovery or proof that these captures used that
format transition. The segment's separate channel/block fields are still
decoded but not applied; their intended interaction with voice format remains
unverified.

The capture reports now populate source layout from sample-reader observations
and clear the processed-voice membership at each VP frame. Paused or multipass
voices can retain their last source observation; the fields are not an atomic
guest snapshot. For streams, `ba` is the observed segment address and
`container` is the effective format selector, not a byte count.

With `RECOMP_DEBUG=1`, observation-only float meters accumulate per voice:
`source` after resampling/before filtering, `filter` before the low-pass clamp
when enabled, and `output` after filtering/HRTF but before gain and bin routing.
The DSP reports its stereo sum before final clipping. Each meter records sample
count, maximum finite magnitude, values outside [-1, 1], and non-finite values.
Reports reset these windows, retaining measurements for voices that ended
between reports. The first window starts at startup/reset and can exceed the
WAV history; subsequent windows start at the previous report. These aggregates
do not locate a defect at an exact WAV timestamp, distinguish different uses
of the same voice handle, or detect every discontinuity below full scale.
Outside debug mode, the float sample meters are disabled.

The final PCM rail count now includes -32767, used by the DSP's float-to-PCM
conversion. This count reports rail occupancy, not proof that every counted
sample clipped. Meter collection does not apply gain, limit samples or repair
non-finite filter state. In particular, the inherited DSP sums all even/odd
bins by default without running the original DSP program; duplicate routing
or insufficient headroom remains a possible source of distortion.

Another code-level candidate is discarded partial output: `voice_process`
returns if sample fetching deactivates a voice, even when that block already
contains decoded output. Changing completion/tail behavior requires its own
boundary tests and is not included in the PCM stride fix.

`tests/Check-AudioVoiceSamples.py` exercises the real maintained sample reader
with mocked guest memory. `tests/Check-AudioSignalMeters.py` exercises the real
float DSP mixdown with synthetic bins, including clipping on both polarities,
stereo routing, meter reporting/reset, non-finite observations, and identical
PCM with meters enabled/disabled. Neither fixture validates gameplay audio or
the Xbox DSP reconstruction.

## Validation limits

Portable Clang and MSVC game builds pass with the reporting changes. No physical listening test, gameplay reproduction or E007 fix is claimed. Reporting alone does not change decoding, scheduling or guest completion; the separately tested PCM stride and host traversal corrections are described above and below.

## E006 capture and voice traversal correction

The reported E006 capture `audio-capture-20261018-190743-601-50480.wav`
contains twenty seconds of mixed PCM. Its companion report has zero output
underruns, queue drops and submission failures. Per-voice meters report no
non-finite samples, no enabled low-pass processing and peaks below full scale.
The initial DSP window (startup to trigger, longer than the retained history)
has a pre-clamp peak of 2.34743 and 4,713 over-range channel samples. The ten
seconds after the trigger have no reported DSP over-range samples. The WAV has
1,710 samples at either +/-32767, all in its pre-trigger half. This confirms
some final-mix clipping, but does not explain every reported crackle.

The largest adjacent-channel changes occur at 4.654667 and 4.655333 seconds,
and another large pair at 5.444667 and 5.445333 seconds. Each pair is exactly
32 frames apart and aligned with VP processing boundaries. Numerical analysis
does not establish what each audible event represents, but suggests checking
block discontinuities rather than assuming compressed sample corruption.

Code inspection found `mcpx_apu_vp_frame` returned immediately after an idle
voice raised an FE trap. `apu_core.c:se_frame` still runs DSP processing and
advances the audio clock, so active voices later in that list and subsequent
lists contribute nothing to that block. The early return had protected the
first trap's method/parameter from being overwritten by later idle voices.

The corrected host policy protects that pending notification by deferring
further idle-method dispatch while FECTL is trapped, and continues processing
active voices in list order. Guest acknowledgement still releases the pending
method; later idle voices can then request their own notification. This is a
correction to this runtime's host scheduling, not a claim about original MCPX
hardware behavior or a fresh disassembly recovery. The capture report now
counts idle dispatches and deferred idle encounters in its existing report
windows.

`tests/Check-AudioVoiceTraversal.py` reproduced suppressed active voices with
the old traversal and passes with the correction. It checks pending ownership
across successive blocks, guest acknowledgement, dispatch order, all three
voice lists, VP monitor publication and empty lists. Portable and MSVC game
builds pass. The correction removes a demonstrated block-dropout path, but
an E006 listening comparison is still needed to establish its effect on the
reported sound; the clipping and partial-tail candidates remain separate.

## Live E009 corruption capture, 19 October 2026

The user reported ongoing corruption while preview process 5980 was running,
then identified the current level as E009. Evidence was preserved under
`build/audio-corruption-20261019/` without stopping, suspending or modifying
the process. Both 20-second WAV/report pairs from that session were copied:
17:54:55 and 18:26:02. The user has not explicitly confirmed which recording
contains the reported audible corruption, so neither is labelled a verified
healthy or corrupted reference solely from its timestamp.

A read-only live snapshot at 18:27:59 contains 1,544,712 bytes of native APU
state and all 64 MiB of guest RAM, with no unreadable chunks. Addresses were
resolved from the running preview's symbols and relocated image base:
`g_apu_state` RVA 01505480, `g_xbox_mem_offset` RVA 009C7E68. The state's size
was compiled from the matching portable runtime header. Metadata and the
temporary capture/inspection tools are beside the snapshots. These are
sequential copies while audio and gameplay continue, not an atomic snapshot;
differences across boundaries do not by themselves prove corruption.

The later report's output queue remains active with zero underruns and zero
submission failures. Its cumulative full-queue count stays at seven throughout
the recorded window, so no new full-queue drops are reported in that interval.
It contains 2,224 PCM samples on the +/-32767 rails; the earlier WAV contains
1,852. Neither rail occupancy nor the reported DSP over-range values establish
the cause of persistent corruption. The reported voice meters contain no
non-finite samples. No corrective code change or E009 fix is claimed from
this evidence; source/sample lifetime and voice-state analysis remain open.

### Debugger attachment and active source evidence

At the user's request a local Windows Debug API helper attached to process
5980, with kill-on-debugger-exit disabled. The first snapshot caught menu mode
7. Its explicit detach returned access denied; after helper exit a separate
`CheckRemoteDebuggerPresent` check confirmed no debugger remained and the game
was responsive. A subsequent gameplay snapshot captured the native APU state,
low guest RAM, the separate 64-MiB contiguous/physical mapping, image bounds,
allocation high-water mark and thread contexts while all threads were stopped.
That capture paused for 344 ms and detached successfully. The user confirmed
corruption persisted after resuming and supplied additional 18:32/18:33 captures.
All artifacts are under the same incident directory, with the second debugger
snapshot under `debug-playing/`. Debugger pauses can introduce their own audio
glitches and are not measurements of the pre-existing defect.

The separate physical mapping matters: `mcpx_apu_phys` chooses low RAM for
the loaded image range, contiguous storage below the allocation high-water
mark otherwise, and low RAM beyond that. An initial offline audit of only low
RAM reported zero active voices; that result is invalid because it read the
voice table from the wrong backing store. The corrected gameplay audit uses
the actual mapping policy and finds fourteen active voices with finite source
pairs and plausible resampler state.

Voice 6 in the debugger snapshot is mono S16 PCM, BA `00374000`, CBO 55227,
EBO 67015. Its scatter/gather descriptor at physical `005BDC70` resolves the
two buffered source samples to `0013FF72` and `0013FF74`. Those addresses lie
inside the executable DSOUND image section (`001340C0` through `001524E4`).
Their signed values are 5142 and 7959; dividing by 32768 gives exactly the
retained resampler pair 0.15692138671875 and 0.242889404296875. This establishes
that image-region bytes actually reached the source reader, rather than merely
observing a transient pointer. Several other effect/loop voices resolve to
entirely zero-filled source spans. The output queue alone cannot explain this
source data. The descriptor's origin, expected asset address, reuse lifetime
and physical-address resolution still need tracing before choosing a fix;
the section can contain constants as well as instructions, and no claim is
made that the two particular sampled words are decoded machine instructions.

### Root cause and heap correction

Further investigation established that this incident starts before DMA setup.
The live log records E009 loading, followed by
`xbox_HeapAlloc: out of memory (requested 5376016, used 49850384/50855936)`
and an `NtReadFile` into buffer zero for 5,375,968 bytes, rejected with
`STATUS_ACCESS_VIOLATION`. The following sound descriptors then cover low
addresses starting at 0x9000 through approximately 0x520000. This links the
NULL-relative sound-bank offsets to the previously observed image reads;
changing physical-address routing would have hidden the allocation defect.

The live host heap had 22,398,768 free bytes, but its largest free block was
5,208,080 bytes at 0x03A13000, ending exactly at the bump frontier 0x03F0A810.
Another 1,005,552 unused bytes lay above the frontier. The allocator searched
freed blocks and unused bump space separately, so neither individual span
satisfied the 5,376,016-byte request even though their union did. Earlier
page-aligned allocations also discarded their alignment padding from the
block table, leaving small untracked gaps between large freed regions.

The maintained runtime allocator now returns tracked free tail blocks to the
bump frontier and records new alignment gaps as free blocks. This remains
under the existing `RECOMP_HEAP_RECLAIM` policy, enabled by this title. It
retains the existing lock, never moves live allocations, never coalesces across
unknown gaps, and preserves real exhaustion failures. Generated game code and
audio sample decoding are unchanged. This is a host allocator correction,
supported by the captured heap and load log, not a new recovery of game source.

`tests/Check-HeapFrontier.py` reproduces the exact failed boundary with the
legacy frontier behavior and passes with the production correction. It checks
zeroing, live allocation ownership, repeated aligned allocation/free cycles,
frontier recovery and genuine exhaustion. When the private incident capture
is present it also replays the entire captured block table: the sound-bank
allocation succeeds without overlapping or clearing any of the live blocks.
Portable and MSVC game builds pass. The incident artifacts include
`heap-blocks.bin`, `heap-metadata.json`, and the replay results under
`build/heap-frontier-test/`. A long gameplay/load-transition comparison is
still required to establish absence of other audio defects or exhaustion
paths. The existing process cannot recover its failed sound-bank read merely
by placing a corrected executable alongside it; restart with the new build.
