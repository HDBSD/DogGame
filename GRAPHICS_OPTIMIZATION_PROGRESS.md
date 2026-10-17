# Graphics optimization progress

## Implemented changes — 17 October 2026

The first changes address clear boundaries and repeated CPU copies. The baseline and broader architectural findings remain in [the audit](GRAPHICS_PIPELINE_AUDIT.md).

### Mirror eligible clears without reading discarded pixels back

`nv_gpu_clear_attachments()` accepts a complete clear of the currently bound color/depth attachment. It validates the entire request before changing either attachment. Unsupported rectangles, layouts, mismatched surfaces and partial channel clears keep the compatibility path.

The GPU clear executes before the matching existing CPU clear. Cleared attachments then agree in both representations; unrelated dirty attachments remain GPU-owned until their next synchronization boundary. The existing CPU writes, register behavior, guest clear order and depth-shadow initialization remain in place. A color check before the following GPU batch detects CPU edits after a mirrored clear and reloads them while preserving pending depth. This is deliberately conservative rather than assuming CPU memory cannot change.

Depth-only clears no longer pass through the color-clear function's unconditional full synchronization. Complete color and depth clears are regression-tested independently and together, including internal scaling. This implements a narrow clear lifecycle; it does not introduce general guest-memory write tracking or persistent ownership for arbitrary surfaces.

### Avoid dirtying unwritable color attachments

Hardware draws with all color write channels disabled no longer mark color dirty merely because a draw was submitted. Earlier dirty data and enabled depth writes are preserved. A draw that changes neither attachment does not require a compatibility readback solely on account of that draw.

### Reuse published high-resolution CPU snapshots during repaints

The framebuffer publisher increments a sequence under its publication lock. The window thread copies a high-resolution snapshot only for a new sequence or a reallocated destination buffer. It still paints overlays and preserves the existing image/camera lock contract. Repainting the same frame no longer recopies its entire high-resolution image.

This does **not** eliminate the initial GPU-to-CPU presentation readback, the publisher's copy, GDI presentation or its lock duration. Those remain architectural work.

### Cache the performance-counter conversion

The GPU timing helper initializes its milliseconds-per-tick factor once instead of querying counter frequency for each operation. Existing accumulated timing and stall reporting remain available.

## Validation

- Full portable Clang/Ninja and MSVC Release game builds pass.
- The actual D3D11 backend regression suite passes under MSVC in all four existing configurations: normal, compatibility resolves disabled, simple pixel shaders disabled, and resident rendered textures disabled.
- The same GPU fixture compiles and passes with the portable compiler.
- New cases verify complete color/depth clears, independent attachment preservation, no extra readback at the mirrored clear, rejected partial/mismatched clears without side effects, CPU pixel edits after a mirrored clear, zero write masks, and scaled-target clear/readback pixels.
- Existing tests continue to cover ordered depth, blending, texture decoding and mutation, software/GPU handoff, masked/swizzled surfaces, target reuse, alpha discard and supersampling.
- H001 was captured with the same executable/configuration procedure as the audit, without changing user display settings or closing any unrelated game process. Only the owned test processes were stopped.

## Measured result

The final 1080p H001 report covers 852 presentation syncs, comparable to the baseline's 852. These are last emitted cumulative counters, not exact 35-second end totals or a deterministic replay.

| Counter | Baseline | After changes |
| --- | ---: | ---: |
| CPU color-clear synchronization calls | 1,657 | 828 |
| Total synchronization counter | 6,650 | 5,820 |
| Total recorded synchronization time | 3,327.8 ms | 3,007.3 ms |
| Color bytes copied at CPU color-clear boundaries | 1,941.80 MiB | 970.31 MiB |
| High-resolution presentation materializations | 852 | 852 |

That is approximately 12.5% fewer synchronization calls and 0.95 GiB less color readback at clear boundaries in this workload. The timing reduction in this sample is approximately 9.6%; it is not a claimed FPS gain or a cross-hardware result. Preparation and synchronization timers overlap and must not be added.

The initial comparison used the clear/copy changes; the final capture includes write-mask handling, cached timing conversion, and CPU-edit validation. Counters for the mirrored-clear path show depth clears in this scene. Color clear support is established by the pixel regression tests, not claimed as a measured H001 benefit.

Raw evidence is retained under `build/graphics-audit/`, including `final-scaled1080/renderer.log`, `final-native/renderer.log`, `gpu-raster-tests.log`, `portable-raster-tests.log`, `fix-build.log` and `msvc-fix-build.log`.

## Submission and display follow-up

`nv2a_gpu.cpp` now tracks the graphics bindings owned by its title-thread immediate context. Consecutive batches issue only changed vertex-buffer, shader, texture, sampler, attachment, blend/depth, viewport and scissor bindings. Fixed input layout, topology, vertex shader, constant buffers and rasterizer bindings are established after invalidation. Readback invalidates the snapshot before unbinding attachments; shutdown also invalidates it. Compute reduction changes CS bindings only. Any future graphics-context mutation outside the binding helper must participate in invalidation. These are borrowed identities kept alive by context references, not a new resource owner.

The 496-byte, fully initialized shader constants are compared byte-for-byte against the last uploaded buffer. Identical constants skip `UpdateSubresource`; changes retain the original upload before the draw. Neither optimization changes draw ordering, geometry, guest memory visibility or readback boundaries. `RECOMP_GPU_STATE_CACHE=0` disables both optimizations for comparison. The new `[GPU-SUBMISSION]` counters count grouped binding calls in the helper, not every D3D11 API operation. The former separate primary/extra texture and sampler bindings are now grouped into four-slot calls, independently of caching.

The window now borrows the published high-resolution snapshot while holding the existing shared publication lock through GDI drawing and world-overlay composition. It drops that pointer before unlocking or processing another window message. `WM_PRINTCLIENT` follows the same lock/lifetime contract. The publisher retains ownership and cannot overwrite or resize either snapshot during a borrow. This removes the separate high-resolution display allocation and a full-image copy for each newly displayed frame (7.91 MiB at 1920x1080), replacing the earlier repaint-only copy avoidance. Native overlay/screenshot storage remains separate. GPU readback, publisher copying and GDI drawing still exist.

### Follow-up evidence

Both compiler game builds pass. The actual backend pixel suite passes all five configurations, including state caching disabled; the portable compiler also passes the same fixture. New checks cover repeated batches with changed vertices but unchanged state/constants, rebinding after synchronization, and scissor changes. Existing blending, texture mutation, attachment switching, depth, alpha and scaled-image checks continue to pass. Display tests pass, with added pointer-lifetime/publication selection checks and existing high-resolution GDI/movie and capture-ownership coverage.

Serial H001 captures use the same 35-second procedure at 1920x1080. The disabled capture predates the display-copy removal; the enabled capture includes both changes. Thus the timing comparison is for this follow-up as a whole, not an isolated state-cache benchmark. Samples are not deterministic replays, and last periodic reports cover slightly different counts:

| Counter | Cache disabled, old display copy | Cache enabled, borrowed display |
| --- | ---: | ---: |
| Presentation syncs | 847 | 852 |
| Draw batches | 215,153 | 216,526 |
| Tracked pipeline calls issued | 3,227,295 | 364,344 |
| Tracked pipeline calls skipped | 0 | 2,883,546 |
| Constant uploads issued | 215,153 | 60,195 |
| Constant uploads skipped | 0 | 156,331 |
| Submission CPU timer | 458.1 ms | 408.9 ms |
| Synchronization CPU timer | 3,087.2 ms | 3,075.1 ms |

The enabled run skips 88.8% of tracked binding calls and 72.2% of constant uploads. Submission time per draw is approximately 11.3% lower in this sample. This is neither a measured total-frame speedup nor a claim that overall performance is fixed. High-resolution materialization remains 6,739.45 MiB over 852 presentation syncs. Raw logs are `build/graphics-audit/submission-{disabled1080,enabled1080,enabled-native}/renderer.log`; comparison counters are in `submission-comparison.json`. Build and regression logs use the `submission-` prefix in the same directory.

## CPU-to-GPU geometry transfer

The bridge now preserves batch-local transformed vertex identities instead of packing and uploading a fresh 116-byte vertex for every triangle corner. `transform_batch_vertices()` supplies canonical identities only where its existing cache has already established repeated-index reuse. Programs that can write constants retain distinct transformed outputs. Even without transform reuse, strips/fans can reuse the same transformed output within a batch. `gpu_append_transformed_vertex()` packs each identified output once, including the existing texture-coordinate scaling and fog calculation; clipped stack temporaries are packed independently and never cached by their recycled addresses.

The backend copies appended vertices, collects triangle indices in original order, and uses a separate dynamic index-buffer ring with DISCARD/NO_OVERWRITE. The vertex-buffer ring retains its existing behavior. The host shader does not consume a vertex-ID semantic. This change preserves the existing guest transforms, rounding, clipping, culling, attributes, provoking-vertex order and blend/depth submission order; it does not translate guest vertex programs to HLSL. Index-buffer creation or mapping failure expands the submitted index stream and uses the direct draw path. Batch reset clears all identities/indices; shutdown releases the index buffer. The API in `nv2a_gpu.h` is title-thread-only and copies input values.

An initial experiment deduplicated fully expanded vertices by hash and exact comparison. It cut transfer bytes but increased submission CPU time to 1,104.6 ms. That approach was removed. Preserving known identities at the bridge avoids paying for a second search over already-expanded geometry.

### Controlled comparison

Serial H001 captures at 1920x1080 use the same executable, with only `RECOMP_GPU_INDEXED_UPLOAD=0` changed for the direct-path capture. Both last periodic reports cover 853 presentation syncs. They remain nondeterministic runtime samples, not a replay or an FPS benchmark.

| Counter | Direct upload | Identity-based upload |
| --- | ---: | ---: |
| Draw batches | 216,516 | 216,416 |
| Vertex upload | 9,057.64 MiB | 3,886.60 MiB |
| Index upload | 0 MiB | 312.32 MiB |
| Reused vertex references | 0 | 46,740,402 |
| Submission CPU time | 410.0 ms | 250.1 ms |
| Synchronization CPU time | 3,186.8 ms | 2,960.9 ms |

Combined geometry payload is approximately **53.6% lower**. Submission time per draw is approximately **39% lower** in this sample. The synchronization difference is not attributable to the indexing change: synchronization boundaries and ownership are unchanged. These numbers measure submitted payload and CPU spans, not PCIe utilization or end-to-end frame speedup. The separate 10-second profiler windows still report 289 flips in each capture; they do not demonstrate higher FPS.

Evidence: `build/graphics-audit/identity-{disabled1080,final1080}/renderer.log`, `identity-comparison.json`. The rejected hash experiment is retained only as a log in `indexed-enabled1080`, not as maintained code.

### Boundary validation

Both final game builds pass (MSVC and portable Clang). The actual D3D11 pixel fixture passes all six MSVC configurations and the portable-compiler run. It now covers indexed draws, direct-path comparison, vertex/index ring wraparound, attribute-sensitive existing tests, and deliberate index-buffer upload failure followed by a successful recovery. `Check-GpuVertexIdentity.py` exercises the production identity and packing functions with pure versus side-effecting transform decisions, clipped temporary address reuse, coordinate/fog packing and new batches at reused addresses. It uses a controlled transform stub to isolate the new identity boundary; it does not independently establish the interpreter's cacheability classifier. The existing `Check-VshCacheability.py` could not run because `tests/vsh_operand_reference.inc` is absent in this checkout. This is an unresolved pre-existing test dependency, not a passing interpreter comparison. `Check-VertexInputs.py` continues to pass its 10,000 input comparisons.

## Retained GPU surface contents

The runtime now retains complete color/depth resource pairs by guest color address and software depth-shadow identity, native dimensions, pitch/depth stride, depth scale, surface format/layout callback and physical dimensions. Allocation reuse by dimensions alone remains available with `RECOMP_GPU_RESIDENT_SURFACES=0` for comparison. The active pair has a separate owner from inactive pairs; a matching rebind moves the actual textures, views and staging resources back into the active slot. No GPU copy is needed to restore them.

Each attachment has an independent snapshot of the guest color pixels or software depth-shadow values it represents. Draw writes and mirrored clears invalidate that attachment's snapshot. Successful upload/readback establishes a fresh snapshot; failed readback does not certify residency. Before a rebind skips an upload, all used guest bytes must still match. CPU edits reload the changed attachment independently. Swizzled layouts cache their pixel offsets once per resource identity rather than re-running the layout decoder on every validation. Padding is not treated as a pixel. Snapshot comparison is title-thread work and certifies byte equality at that boundary, not tracked write events, allocation lifetime notifications or safety against concurrent external memory writers.

Inactive, native-sized compatible color attachments can be bound directly as shader resources after the destination switch. This replaces the separate rendered-texture `CopyResource` snapshots. The active output is never returned as a sampling view; incompatible/scaled layouts and CPU edits retain the decode/upload fallback. The older rendered-texture snapshot cache remains in the disabled-residency comparison path. Cache eviction releases owned references, while context/draw references keep already submitted resources alive.

Uniform CPU contents are reproduced with a GPU attachment clear instead of a full re-upload when a correctly sized resource pair already exists. This checks every used pixel/float word, including swizzled pixels; it does not trust a previous clear command or discard intervening CPU edits. Color uses the exact packed channel values. Finite depth values use the original normalize/clamp calculation; unsupported/nonfinite cases retain upload. This avoids full-resolution CPU expansion for cleared scaled surfaces. Guest CPU buffers still retain their existing clear/shadow representation.

Inactive residency is bounded to eight pairs and 128 MiB of attachment/staging payload, excluding the current pair, CPU snapshots/layout tables and temporary context references. Oversized pairs are released instead of cached. Scale/layout changes select another identity; overlapping aliases observe the existing synchronization before guest-byte validation. Identical-byte writes or reused allocations without a byte change cannot be distinguished by this scheme. The scaled-image enhancement retains its original weaker correspondence with guest-native pixels.

This is **conservative GPU residency**, not deferred guest-memory coherence: the old readback boundaries remain. Guest code can still access RAM directly without notifying the renderer, so no stale guest buffer is introduced to remove a synchronization point. All D3D11 context, resource ownership and guest mutation work remains on the title thread; the window continues to consume its published CPU snapshot.

### Residency verification

The pixel fixture now checks same-sized independent surface identities, preserved depth ordering after alternating targets, independent CPU color/depth mutations, layout changes at the same address, bounded eviction/reload, uniform clear values and upload avoidance, native texture sampling, CPU texture edits, scaling, existing masked/swizzled surfaces and the disabled-residency path. These are host backend observations and tests, not fresh guest disassembly recovery. Full game builds and synthetic tests remain narrower than gameplay across all levels, video, save/load and GPU families.

### Residency measurements

Final serial H001 captures use the same portable executable, 35 seconds, 1920x1080, with `RECOMP_GPU_RESIDENT_SURFACES=0` for the disabled run. Last periodic reports cover slightly different frame counts; this is not a deterministic replay.

| Counter | Residency disabled | Residency enabled |
| --- | ---: | ---: |
| Presentation syncs | 851 | 856 |
| Draw batches | 215,999 | 217,296 |
| Color attachment upload | 7,362.29 MiB | 16.58 MiB |
| Depth attachment upload | 7,354.38 MiB | 260.04 MiB |
| Retained color/depth reuse | 0 / 0 | 4,157 / 3,350 |
| Direct resident texture bindings | 0 | 7,488 |
| Separate rendered-texture GPU snapshots | 3,308 | 0 |
| Upload replacements using color/depth clears | 0 / 0 | 1,685 / 1,661 |
| Preparation CPU timer | 3,891.7 ms | 2,517.3 ms |
| Synchronization CPU timer | 2,968.4 ms | 2,939.6 ms |

Combined attachment upload payload per presentation is approximately **98.1% lower**, and preparation CPU time per draw approximately **35.7% lower**. These counters exclude geometry and ordinary decoded texture uploads. Preparation and synchronization timers overlap and cannot be added. GPU clear operations retain command/VRAM costs; avoided upload bytes do not measure total GPU traffic.

High-resolution CPU presentation materialization remains 6,771.09 MiB over the enabled sample. Readback count/bytes remain comparable after accounting for frame counts. The separate 10-second profiler windows record 289 and 290 flips respectively; they do not demonstrate an uncapped FPS gain. The early cache implementation without uniform upload replacement was slower; its log remains as `resident-enabled1080`, and it is not the final result.

Evidence: `build/graphics-audit/resident-final{,-disabled}1080/renderer.log`, `resident-comparison.json`. Final GPU pixel coverage passes seven configurations under MSVC (including residency disabled), with an additional portable compiler run. Game builds pass under both compilers. Native-resolution runtime capture is recorded in `resident-final-native/renderer.log`. This remains synthetic coverage plus H001, not full-game or cross-GPU validation.

## CPU/GPU ownership work that remains

The indexed bridge reduces CPU-to-GPU payload; it does not fix GPU-to-CPU ownership. The next architectural boundary must distinguish three consumers: another GPU draw sampling a surface, a guest CPU/software read requiring bytes, and window presentation requiring a completed image. Only the second inherently requires a guest-memory readback. A render-target switch or GPU sample should retain or copy a GPU resource; the window should consume a GPU snapshot through DXGI. Today those consumers still converge on synchronization and CPU/GDI images.

The conservative cache now retains per-surface identities and validates independent attachments. Deferring CPU readback additionally requires explicit CPU-read/write/alias invalidation and reference-counted GPU presentation frames. Guest reads/writes can currently bypass this backend, so silently leaving guest RAM stale would break the compatibility contract. The title thread must remain the immediate-context owner; a UI callback cannot safely start using that context. Representative video, overlays, screenshots, CPU fallback, allocation reuse and scene transitions must be validated before replacing that contract.

## Still outstanding

The renderer still materializes the high-resolution image for CPU/GDI display and still synchronizes on many target/sample boundaries. Deferred per-surface readback, GPU-native presentation, lazy general depth shadows, write-aware texture validation, further geometry and submission preparation reduction remain separate work packages. Removing these transfers requires addressing their consumers rather than simply deleting synchronization calls.

The changes have synthetic GPU pixel coverage and an H001 runtime sample, not a full playthrough. Further scene, cutscene/video, resolution, overlay, save/load, older GPU and integrated GPU validation remains necessary.
