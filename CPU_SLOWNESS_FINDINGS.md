# Whole-process sampling: E006

## Finding

The title's highest recurring sampled location is a GPU flush acknowledgement
spin, not game arithmetic or scene traversal. A second recurring location is
the maintained gameplay frame's refresh-counter spin. The evidence supports
investigating these emulated hardware waits before replacing the game engine.
It does not yet establish the wall-time saving of replacing either wait.

Two isolated E006 runs used the current MSVC Release executable with its exact
PDB, internal/output 4K, audio enabled and focus pausing disabled. Neither run
replayed the user's interactive route. Raw samples are preserved under
`build/vulkan-speed/cpu-profile-e006/`:

- `stacks.csv`: 20-second context-sampling run, 563 recorded contexts.
- `stacks-with-lines.csv`: 15-second confirmation run, 469 recorded contexts,
  retaining host instruction addresses, symbol offsets and available PDB lines.
- `summary.json`: per-thread counts and interval CPU-time totals from the second
  run. CPU-time deltas cover intervals, not the cost of the sampled stack.

The user-mode fallback briefly suspends each sampled thread to read its context
and unwind it, then resumes it before resolving symbol names. Both runs finished
with zero reported context/resume failures. This perturbs execution and is not
an unbiased kernel CPU-sample profile. These runs identify recurring locations;
their FPS and sample proportions must not be treated as baseline performance.

## Source-confirmed locations

In the confirmation run, the title thread had 160 recorded contexts:

| Leaf location | Contexts | Meaning established by source |
| --- | ---: | --- |
| `sub_001042A0` | 109 | 108 contexts resolve to generated source line 1456, which reloads the flush register inside the loop at guest `0x001042E0`. |
| `title_guest_run_gameplay_frame` | 36 | Recurring lines 2290 and 2303 reload/compare the refresh counter in the loop at guest `0x0005DE25`. |
| Other leaves | 15 | Various generated game routines; no single arithmetic/scene-traversal leaf dominates these remaining contexts. |

The first run independently showed 132 of 185 title-thread contexts in
`sub_001042A0` and 33 in the gameplay-frame adapter. Thus both runs repeatedly
observe the same waits. The title thread accrued about 14.34 seconds of CPU
time over the confirmation run's sampling intervals, consistent with spinning.
This is not proof that 109/160 of frame wall time belongs to the flush wait.

`src/recomp/gen/recomp_0019.c:1456` corresponds to this sequence:

1. Load the device's register base through guest `0x0010E648`, field `0x1C18`.
2. Set bit `0x10000` at register-base offset `0x100410`.
3. Repeatedly read the same bit at guest `0x001042E0` until another thread clears it.
4. Continue the original command-publication sequence at guest `0x001042EC`.

The maintained runtime calls this the PFB cache-flush handshake.
`runtime_overrides/src/kernel/xbox_memory_layout.c`, `nv2a_flush_thread`, clears
that bit using `InterlockedAnd` and calls `Sleep(1)` between polls. The general
acknowledgement worker also services the acknowledgement table. Consequently,
this title wait depends on host worker scheduling. It is distinct from waiting
for a Vulkan fence. The runtime comment treats this handshake as CPU-memory
visibility, not DMA completion; that is existing adapter policy, not new hardware
documentation or a fresh Ghidra recovery.

`src/api/title_guest_routines.c:2290` is another explicit spin: read guest
`0x00214B2C` until its delta reaches the selected refresh count (two by default).
That identifies intentional frame pacing. Busy CPU use in this loop must not be
mistaken for game simulation cost. Its wake latency and relationship to actual
presentation still need separate measurement.

## Other observed work

The renderer/FIFO worker accrued about 11.75 seconds of CPU time over the
confirmation intervals. Its stacks repeatedly include packet execution, texture
decoding (`sample_tex`, `sample_swizzle_offset`), Vulkan preparation/publication,
driver calls and waits. A window/presentation thread accrued about 4.72 seconds;
its leaves include `NtGdiStretchDIBitsInternal` and `NtGdiBitBlt`, as well as waits.
These observations agree with the remaining CPU texture/presentation paths.

Not every runtime/system frame has private line symbols. A nearest public symbol
with a large displacement can be misleading (for example an unrelated CRT name).
Do not infer responsibility from those names alone. Generated title lines and
the maintained gameplay-frame adapter have matching PDB line attribution.
Audio was enabled, but this stationary workload does not reproduce active combat
or establish an upper bound on audio cost in the user's slowdown.

## Next fixes to validate

1. Isolate the title's flush handshake behind a runtime API and a Vulkan-only
   maintained entry-point override, preserving all surrounding guest register,
   stack, access-width and command-publication order contracts. If the existing
   coherent-host-RAM policy is validated, acknowledge that visibility operation
   synchronously instead of waiting for a periodically scheduled worker. Do NOT
   advance DMA_GET, release render semaphores, or report GPU completion early.
   A title-specific replacement belongs in `src/overrides/`; generated code stays
   untouched. Instrument request count and elapsed handshake time first so the
   change can be compared against a concrete dependency, not CPU utilization.
2. Replace the maintained gameplay refresh spin with a signalled host wait tied
   to the same counter/refresh source. Preserve two-refresh pacing and recheck
   the predicate after waking. Do not replace it with an arbitrary sleep or make
   the clock advance faster. Measure missed deadlines separately from idle time.
3. Continue eliminating CPU texture decoding/readback and GDI presentation where
   measured on the renderer worker. These remain independent costs even if the
   title thread stops consuming a core while waiting.

These are concrete, sampled targets. Eliminating spins alone is not a demonstrated
FPS fix: it must also reduce a critical-path delay or contention. A full graphics
rewrite is not justified solely by these samples. No guest rendering routine,
generated file or deployed game executable was modified by this investigation.

## Capture tools and limitations

`scripts/Capture-CpuProfile.ps1` records WPR CPU samples and scheduling for a
specified PID. It never cancels another recording or terminates the target.
WPR refused kernel profiling in this session with error `0xC5585011` (profiling
privilege could not be enabled). The attempt did not leave a WPR session running.
This is a Windows privilege limitation, not an automatic approval rejection.

`scripts/cpu_sampler.cpp` is the standalone x64 user-mode fallback, registered
with CMake under `scripts/cpu_sampler/`. Build it with a Windows C++ compiler and
link `dbghelp`. Usage: `doggame-cpu-sampler PID SECONDS NEW_OUTPUT.csv`.
Set `_NT_SYMBOL_PATH` to the directory containing the exact executable's PDB.
The output must be new. The sampler does not change guest memory, but brief
thread suspensions make it unsuitable for measuring exact frame pacing. A user
session with kernel profiling privileges is still needed for precise scheduling
and unbiased running-time attribution on the actual interactive route.
