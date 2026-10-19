# Recompiled execution: feasibility and current evidence

## Assessment

The current instruction-preserving translation has identifiable overhead that
idiomatic native game code would not need. That warrants measurement and
selective replacement. It does not establish that static recompilation is
impractical, or that translated game logic is the cause of the remaining FPS
drops. The renderer and runtime are distinct from the generated title code;
both can consume CPU time on the same title thread.

This review reads the generated source without editing it. It also reads the
maintained runtime template, generated dispatch table, build configuration and
latest renderer log. It is not a new Ghidra recovery or a full CPU profile.

## Concrete implementation costs

| Mechanism | Local evidence | Consequence to investigate |
| --- | --- | --- |
| Thread-local guest register model | `runtime_overrides/templates/runtime/recomp_types.h`: `g_eax`, `g_ecx`, `g_edx`, `g_esp` and other guest state; register aliases | Host code must maintain guest-visible state at call boundaries. It cannot freely treat these values as ordinary C function arguments and locals. Actual load/store frequency requires optimized assembly or sampling. |
| Volatile guest memory | Same template: `MEM32` dereferences a `volatile uint32_t *` | The compiler must preserve observable volatile accesses. It has less scope to eliminate repeated guest loads/stores or optimize memory loops than with ordinary RAM accesses. This is not a reason to remove volatile globally: device accesses, concurrency and ordering contracts require an audit. |
| Explicit guest stack | `PUSH32`, `POP32` and emitted call/return sequences in `src/recomp/gen/recomp_0014.c` | Guest stack operations remain in addition to host call machinery. Some values and bookkeeping can be optimized, but accesses that implement guest-visible semantics cannot simply disappear. |
| Indirect call adaptation | `RECOMP_ICALL_SAFE_AT_CC` in the runtime template | Trace/count stores, validation, manual/generated/kernel lookup and an indirect native call can occur where original code had an indirect guest call. The template performs this bookkeeping even without opt-in direct-call profiling. Frequency and total cost have not been measured in this session. |
| Dispatch lookup | Generated `src/recomp/gen/recomp_dispatch.c` | Generated lookup already has a flat-table fast path, with binary search as fallback. Manual/generated/kernel resolution still adds work on indirect paths. Direct `RECOMP_ABI_CALL` expands to the native call when optional checks/profiling are disabled; not every call takes dispatch lookup. |
| Floating-point state adaptation | x87 stack/control/status helpers in the runtime template | Maintaining guest floating-point semantics can inhibit ordinary host optimization. Whether these helpers are hot in DogGame remains unmeasured. Fast-math is not a safe general replacement. |
| Compilation boundaries | Generated functions split across C translation units; final portable link has no LTO flag | Cross-file optimization is limited in this configuration. LTO/PGO is a testable build experiment, not a demonstrated fix for volatile memory or resource synchronization. |

The portable Release cache selects `-O3 -DNDEBUG`, and its generated object
rules use the Release optimization flags. This is not an unoptimized Debug
build. Large emitted C files and repeated flag temporaries are not, by
themselves, proof of equally large machine-code overhead: dead temporaries and
redundant arithmetic can be removed. The guest-state and volatile-memory
contracts are the more relevant constraints.

## What the latest log does and does not establish

The latest installed `debug-render.log` has a final renderer reporting window
with 6,840.9 ms raster-path elapsed time, including 869.1 ms vertex transforms
and 5,483.0 ms bridge time, over about ten seconds. Its associated pacing
window contains 298 publications averaging 33.530 ms, with p99 41.704 ms and
maximum 55.160 ms. This is approximately 23 ms of rendering-path elapsed time
per publication in that window. Bridge, vertex and backend counters are nested;
they must not be added to raster time.

This elapsed time includes maintained host rendering code and CPU/GPU waits.
It is not all generated title execution. The remaining wall time can contain
translated game logic, audio/runtime work, scheduling and intentional pacing.
The log cannot divide those costs or attribute an individual slow frame to a
generated function. Average 30 FPS captions do not establish consistent frame
deadlines, and renderer publication timestamps are not scanout timestamps.

## Practical direction

Keep the generated title translation as the compatibility implementation while
measuring its actual hot paths. This allows graphics or other proven hot
subsystems to be replaced independently, without requiring a complete game
rewrite. Title-specific replacements belong in `src/overrides/`, with recovered
interfaces, provenance and guest ABI contracts documented beside their adapters.
Rendering replacements remain Vulkan-only under the existing project policy.

The next useful measurement is a whole-frame sampled CPU profile with stacks
and thread scheduling, correlated with GPU timestamps and the slow frame IDs.
It must distinguish generated title code, dispatch/ABI helpers, vertex shader
interpretation, texture validation/decoding, Vulkan recording/readback, audio,
OS waits and intentional frame pacing. Call counts alone cannot establish time
spent; adding a timer to every guest call can perturb the workload severely.

Use the same interactive route and resolution to compare baseline with each
experiment. Test native and 4K rendering to help expose resolution-dependent
work, while retaining whole-frame attribution. Then:

1. If generated geometry, skinning or scene traversal dominates, recover those
   specific routines and implement native equivalents behind their guest entry
   points in `src/overrides/`. Preserve data layout and call order at the boundary.
2. If runtime translation dominates, keep work in the runtime: GPU vertex
   processing, resident textures/surfaces and GPU presentation. Rewriting guest
   game logic would not remove those adapter costs.
3. If dispatch or register-state adaptation dominates, investigate cached call
   resolution across manual/generated/kernel paths and compiler/PGO experiments
   using the existing ABI; account for the existing flat-table fast path. Validate
   stack cleanup, registers and indirect targets; do not regenerate or hand-edit
   generated files without the user's explicit instruction.
4. If slow frames are scheduling or pacing events, correct those dependencies
   rather than using average render time as evidence for a game-code rewrite.

This audit establishes plausible translation costs and a concrete decision
process. It does not establish a single generated-code bottleneck or justify
abandoning the approach. No executable or generated source was changed by it.
