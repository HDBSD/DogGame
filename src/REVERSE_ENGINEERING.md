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

Synthetic tests can verify layout handling, preserved state, call order, press-edge changes, and copied geometry. Syntax checks verify that the source compiles against the available headers. Neither substitutes for a full link and a gameplay run of the supported XBE. Record those separately when performed.
