# Why these APIs exist

The title mixes original Xbox routines with Windows services. Our changes should read in terms of what they accomplish: restore a save, warp to a scene, collect clickable menu targets, copy collision geometry, or verify preview access. The adapters in `src/api/` contain the details needed to connect those operations to the existing game.

## Recovery and implementation workflow

1. Identify the behavior or failure and reproduce it. Record the game revision and relevant configuration.
2. Use Ghidra or the available disassembly to follow the responsible reads, writes, and calls. AI assistance can help investigate and propose explanations; verify those proposals against the code and observations.
3. Record the evidence before naming the API: instruction/function addresses, actual field widths, calling convention, observed state transitions, and unresolved meanings.
4. Implement an adapter with a clear thread, ownership, failure, and side-effect contract. Application code calls the operation; it does not repeat its recovered layout.
5. Test the boundary and validate the real behavior. Keep evidence from synthetic fixtures distinct from gameplay observations.

This refactor uses the previous implementation, its comments, generated ABI declarations, and `BOOT_NOTES.md`. It does not represent a new Ghidra session or a fresh gameplay run. Address and layout meanings without recorded analysis references remain inherited knowledge.

## System boundaries and evidence

| System | API and rationale | Available evidence and limits |
| --- | --- | --- |
| Guest calls | `title_guest_calls`: save/restore registers, invoke original routines, find assets, send menu events, restore saves and run portal transitions. | Generated register/stack model and previous wrappers. Asset lookup uses `0x9CAE0`; `0x9CA10` is asset-name lookup. Menu events use `0x14220`. Calls push a synthetic return address; outer operations preserve guest state. |
| Global game state | `title_guest_state`: named reads/writes for mode, scene, player powers, health, model, frame, camera, boot state and save selection. | Addresses and widths inherited from existing hooks. This API makes those meanings explicit; it does not independently prove them. |
| Menu input | `title_guest_menu`: determine whether the active menu accepts Back and remap press edges. | Prior implementation identifies modes 2/7 and the visible map listener `0xDF911682`. Boot notes record event 66 using B (`0x20000`). Initial Start remains necessary. Original listener asset name is unrecorded. |
| Menu assets and hits | `title_guest_ui`: decode font/image arrays and links, collect copied click targets, access fields, and write native event links. | Existing code identifies font count/array at scene `+0x100/+0x1AC` with `0x100` stride; image count/array at `+0xFC/+0x1A8` with `0xF8` stride. Links have 32-byte records. Comments explain map page selection and save-row hit limits. Widths and event order are preserved. |
| Player damage | `title_guest_player`: classify monster damage separately from debug protection policy. | Previous comments identify NPC contacts at `0x2C19A/0x2C28D`, generic damage wrapper `0x28E80`, and attack initialization `0x2E8EE`. NPC type 2 and attack type `0x19` are recorded there. A damage source can be an attack record, so caller evidence and entity type are both used. Other recorded caller addresses remain in the adapter. |
| Player flight | `title_player_flight` interprets host input; `title_guest_player` applies movement while retaining native model updates. | Existing implementation calls `0x860C0` before motion and `0x861A0` afterward. It clears velocity, floor/platform associations and collision contacts. Previous comments record the reversed horizontal camera basis. The bit representation of elapsed time is passed unchanged to native updates. |
| Debug transitions | `title_guest_warp_to_scene` owns cleanup/load/setup and clock reset; debug policy decides when to call it. | Boot notes record that gameplay state 1 continues the loop and state 0 returns to the menu. Previous code follows mode change `0x5F3A0`, cleanup `0x5CAF0`, load `0x5D2A0`, setup `0x5D330`, clock `0xADFA0`, state `0x5F2F0`. Power bits are restored after the transition. |
| Geometry | `title_guest_geometry` copies a `TitleGeometrySnapshot`; `title_debug_geometry` rasterizes that snapshot. | Existing comments cite native trigger update `0x3A510` and live RenderWare sectors. Trigger count/list are scene `+8/+0x28`; environment is `+0x40`; collision world root is `+0x1C`. Leaf type is -1. Vertex/polygon counts are `+0x8A/+0x8C`. Bounds, recursion and work budgets remain in the adapter. |
| Original routines | `title_guest_routines` and `title_guest_debug` retain translated originals; the override files preserve the original `sub_` entry points and forward to named operations. | Existing labels and instruction comments are retained. Known roles come from the original comments. S005 and O008 routines with incomplete meanings retain identifying addresses in their names. These implementations have not been reconstructed into ordinary high-level game source. |
| Guest diagnostics | `title_guest_diagnostics` owns trace wrappers and state observation; `recomp_manual` owns dispatch and unresolved-call reporting. | Prior trace code supplies the record offsets. The observation worker reads volatile guest values without a coherent snapshot; it must not mutate the guest or invoke native game routines. |
| Crash symbol lookup | `title_guest_symbols` maps named routine implementations back to their original Xbox entry addresses. | The mappings come directly from the retained override entry points. This preserves address discovery when optimization places a fault in the named implementation rather than its `sub_` wrapper. |
| Runtime startup | `title_host_runtime` sets defaults and initializes device integration; `main` controls startup and crash handling. | Previous comments cite D3D fence waiter `0x1045A0`, device pointer `0x10E648`, completion fields `+0x2C/+0x30`, initial ring base `+0x24`, completion event `+0x1DBC`, and video clock `+0x1DD8`. USB defaults preserve the recorded Xbox port-index XOR 2 mapping. Explicit environment settings retain precedence. |
| Prompts | `prompt_text` converts explicit colored glyph tokens; `title_prompts` temporarily substitutes the draw string and restores continuation pointers. | Existing implementation keeps source assets unchanged and records output-to-source byte offsets. The draw entry point is `0x8CE00`; the original renderer remains available under its private name. |
| Saves | `title_saves` exposes a storage initialization API and maps the title's guest save paths to per-user files. | Existing guest title path `UDATA/5451001f` and three slot names are retained. Boot notes and code record one-time migration with a marker outside the saves folder so deletion does not import old saves again. Copy errors remain failures; missing legacy saves are allowed. |
| Arguments | `debug_options` validates the complete command line before publishing options. | Existing parser contracts cover debug-only flags, four-character scene IDs and save slots 1–3. Argument errors and missing levels are reported before entering the guest. |
| Preview identity | `preview_auth` handles browser/network flow; `preview_auth_identity` verifies signature and claims; `preview_auth_json` provides bounded parsing and Base64URL helpers. | Existing fixed-tenant RS256/PKCE flow. Key lookup rejects ambiguity, signature verification precedes identity checks, and issuer/audience/nonce/time/role claims remain required. Configuration is shared through `preview_auth_config`; token-provided key URLs are never trusted. This is host protocol code, not recovered game logic. |

## Working at the instruction boundary

Do not remove `loc_` labels, emulate a guest return with a host return alone, infer an original function's parameters from a C signature, or reorder flag/x87/SSE assignments merely to simplify the source. The original game ABI is part of behavior.

New policy belongs in an intent-based helper. A small guest entry point routes to the relevant operation. Original translations remain in their adapters until evidence supports a safe high-level reconstruction. The generated-source exclusion scan still sees every public `sub_` wrapper in the maintained sources outside `src/recomp/`.

## Validation limits

The [first graphics optimization pass](../GRAPHICS_OPTIMIZATION_PROGRESS.md)
documents mirrored GPU/CPU clears, preservation of unrelated dirty attachments,
post-clear CPU-edit validation, repaint snapshot reuse and the before/after
H001 measurements. The clear path is optional and falls back without side
effects when the complete attachment overwrite cannot be established.

The [graphics pipeline audit](../GRAPHICS_PIPELINE_AUDIT.md) traces the active
NV2A/D3D11/framebuffer route, documents source-established repeated work and
serial native/scaled H001 profiles, and lists the ownership and correctness
tests required before removing compatibility transfers.

On 15 October 2026, the portable Clang build failed during device initialization.
Both supplied `DogGame-crash-20261015-0130*.dmp` files stopped in host code for
`sub_0017ACD8`, at instruction bytes `23 14 08` (`AND edx, [rax+rcx]`). The native
fault address was `FED10034`; subtracting the recorded guest mapping offset
`10000` identifies OHCI0 register `FED00034`. The shared MMIO decoder understood
memory-destination AND but not this register-destination read.

After supporting that instruction, a local startup run reached the APU and
faulted in `sub_0013E507` on `F7 84 06 F0 1F FE FF 18 00 00 00`
(`TEST dword [rsi+rax-1E010], 18`) at guest register `FE802000`. The APU's legacy
decoder now delegates unsupported forms to the shared decoder, which handles
this immediate TEST. Device callbacks remain responsible for access semantics;
the decoder updates host registers/flags and advances the native instruction.

`tests/mmio_and_test.c` covers both instruction byte sequences, one read/no
writes, register widths, logical flags, extended registers, SIB displacement
lengths and rejection of unsupported register-only forms. A rebuilt portable
executable ran for 20 seconds into logo video playback and D3D11 rendering
without either MMIO fault, then the owned test process was closed. This is a
startup smoke test, not validation of a complete playthrough.

The same fixes were validated with MSVC 19.51.36257 on 15 October 2026:
the full x64 Release executable linked, the MMIO regression executable passed,
and a 20-second startup run reached D3D11 rendering and logo video playback.
The Microsoft SDK uses `xinput.lib` for `XINPUT1_4.dll`, whereas MinGW needs
`xinput1_4`; the build selects that name per toolchain. Both compiler builds
were relinked after this adjustment. Generated-code shift warnings remain;
startup success does not establish correctness of every generated routine.

Synthetic tests can verify layout handling, preserved state, call order, press-edge changes, and copied geometry. Syntax checks verify that the source compiles against the available headers. Neither substitutes for a full link and a gameplay run of the supported XBE. Record those separately when performed.

The graphics submission follow-up tracks title-thread D3D11 bindings and exact constant-buffer contents; synchronization invalidates binding assumptions. Display composition borrows immutable published high-resolution pixels only while holding the publication lock. These are host-side optimizations established from the maintained backend and pixel/window fixtures, not newly recovered guest behavior. See `GRAPHICS_OPTIMIZATION_PROGRESS.md` for contracts, counter definitions and H001 measurements; GPU readback/GDI presentation remains in use.

The indexed host geometry bridge reuses the batch identities already established by `transform_batch_vertices()` in `runtime_overrides/src/kernel/nv2a_pb_exec.c`. It retains distinct outputs for constant-writing programs and does not reuse clipped stack temporaries. `nv2a_gpu.h` exposes copied vertices and batch-local triangle indices; the D3D11 backend preserves triangle order and falls back to direct upload on index-buffer failure. This is an optimization of the maintained host bridge, not fresh recovery of a guest algorithm. Transfer measurements, tests and the missing interpreter-reference fixture are recorded in `GRAPHICS_OPTIMIZATION_PROGRESS.md`.

The host backend now retains actual GPU surface pairs rather than only reusing allocations by dimensions. `select_surface_resources()`, `same_surface_layout()` and the per-attachment snapshot helpers in `runtime_overrides/src/kernel/nv2a_gpu.cpp` define title-thread ownership and rebind validation. Inactive compatible native targets can be sampled directly; uniform CPU contents can be reproduced through exact attachment clears. The evidence is maintained backend inspection, offscreen GPU pixel tests and H001 comparison captures. No new Ghidra recovery or guest access tracking is claimed. Existing CPU visibility boundaries remain in force; snapshots cannot identify identical-byte writes or allocation lifetime changes. See `GRAPHICS_OPTIMIZATION_PROGRESS.md` for limits and measurements.

Windows controller transport now lives in `runtime_overrides/src/input/host_gamepad.c`, using pinned SDL 2.32.10 Sony HID/game-controller drivers to produce the existing XInput-shaped host state. `xinput_device.c` still owns conversion to the original Xbox analog-button structure and keyboard/overlay policy. This is a host backend extension, not recovery of guest input routines or Sony HID packet layouts. `CONTROLLERS.md` records mapping, ownership, source provenance, virtual-device tests and outstanding physical USB/Bluetooth validation.

### Host menu controller input

The added debug and recomp-settings menus previously read native `XInputGetState` in their draw callbacks (`title_debug.c:debug_draw`, `title_display.c:display_draw`). Inspection established that this bypassed the SDL backend used by Sony controllers and limited edge detection to rendered frames. Both now read `host_gamepad_read_menu` through the runtime's public host adapter, while debug flight uses its current-state API. The adapter samples host state on an independent thread and retains ordered press events with per-consumer cursors; no guest addresses, calling conventions or original input routines were reconstructed or changed. The virtual-controller test reproduces a released tap between menu reads, independent consumer ownership, held-button deduplication and reconnect history reset. Physical PS4/PS5 menu operation over USB/Bluetooth remains unverified. See `CONTROLLERS.md` for sampling/coalescing limits and ownership.

The menu draw callbacks have since been split into window-thread input updates and drawing. `fb_present.c:display_present` uses nonblocking frame acquisition and UI-owned background/menu canvases, so a slow publication cannot stop message dispatch and menu updates. World diagnostics are cached with their original frame; they are not read again against a borrowed/stale camera when acquisition fails. A hidden-window regression test holds publication for one second and checks continued updates/redraws and background ownership. This is host scheduling/composition work, not a change to recovered guest menu timing.

Rapid-input validation exposed a separate host-policy loss: comparing per-button counters as booleans merged repeated taps and discarded ordering between menu reads. The adapter now publishes a bounded ordered ring, and each menu drains its own cursor before drawing. SDL operations and device scanning hold a separate lock from UI snapshot reads. Virtual-device tests verify the Down/Down/Confirm/Up sequence for independent readers; a production settings-handler test verifies Down/Down/Confirm activates the intended row. Optional `DOGGAME_INPUT_TRACE` records sampled/handled input and UI gaps for remaining physical-session investigation. Neither these tests nor the prior lock-contention fixture establish that every reported in-game miss has the same cause.

Native-menu clarification: the reported problem covers the game's main/load/save screens on both keyboard and controller, not only the added overlays. `title_menu_input_snapshot` observes player one's held/pressed state (+0x2C/+0x30, from translated `sub_00090F50:loc_00091192`) at existing title-thread hooks. Optional trace output correlates this with state handed to USB and keyboard messages; it does not mutate input or claim atomicity with USB/XAPI. A six-tap, 80 ms keyboard main-menu probe observed all six guest edges, so the user's intermittent loss remains unreproduced. Native Load/Save and physical-controller failures remain unvalidated; the host-menu queue fixes do not establish a fix for them.

The 2026-10-18 user trace correlates all 93 USB-source rises after the first observed native main-menu frame with native press edges; capture remains off. That moves investigation past host sampling/USB for those events, without proving listener delivery. Snapshot diagnostics now include the deltas at 0x001A4F14 (main) / 0x001A5220 (save) and UI threshold at 0x0018131C. Existing translated `sub_000791B0:loc_000791B0` skips its update at `loc_000794EB` for elapsed <= threshold; the native input polls precede that call in both maintained loop overrides. The supplied trace lacks deltas, so clock-gate involvement remains a hypothesis. A separate hidden scripted probe did not observe zero deltas at its captured edges. No guest timing behavior has been changed on that basis.

Native clock loss reproduced: the edge-only timing probe at host tick 233230171 recorded pressed Up (`0x10`), main elapsed bits `0x00000000`, and UI minimum bits `0x3727C5AC`. In the maintained main loop, input is polled at 0x0006D1C8 and observed at 0x0006D1CD, before the UI call at 0x0006D210. Translated `sub_000791B0` returns at 0x000794EB when elapsed <= the minimum at 0x0018131C. The Save/Load loop has the same poll-before-UI ordering (0x00074176 / 0x00074193). These references come from the existing instruction translation; no fresh Ghidra recovery is claimed.

The host KeTickCount publication used GetTickCount while its existing bridge contract declares a 1 ms clock. `guest_clock.c` now derives elapsed milliseconds from QPC while retaining an uptime epoch, shared with KeQueryInterruptTime. Clock initialization and publication remain host-owned; no guest ABI/instruction ordering changes. A repeated navigation probe reduced observed zero-delta press edges from 1/27 to 0/28. This corrects a reproduced failure path, not proof that every physical miss or Load/Save case is resolved. The user's marked three Down presses lack timing fields, so their individual delta values remain unknown. See CONTROLLERS.md for reproducible tests and limits.
