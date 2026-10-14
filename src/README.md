# DogGame source

`api/` contains the shared guest and host adapters and helper implementations. Their public headers are included as `api/<name>.h` by application code.

These files contain the Windows host and the title-specific changes that survive regeneration. `recomp/` contains generated translations and is maintained by the generation pipeline.

The project-wide standard lives in [AGENTS.md](../AGENTS.md). [REVERSE_ENGINEERING.md](REVERSE_ENGINEERING.md) explains the recovery workflow, system APIs, evidence, and remaining uncertainty for all maintained work.

| File | Responsibility |
| --- | --- |
| `main.c` | Load the XBE, initialize the runtime, enter the game, and report crashes. |
| `recomp_manual.c` | Select overrides by Xbox address and log unresolved indirect calls. |
| `debug_options.c` / `.h` | Validate command-line options and initialize diagnostics. |
| `title_debug.c` | Queue debug actions and execute them on the title thread. |
| `title_debug_geometry.c` / `.h` | Render copied collision and trigger snapshots in the debug overlay. |
| `title_debug_overrides.c` | Route original guest-address entry points to debug-aware operations. |
| `title_display.c` / `.h` | Handle display settings, menu events, and mouse targets. |
| `api/title_guest_menu.c` / `.h` | Expose recovered menu and controller behavior through a small title-thread API. |
| `api/title_guest_ui.c` / `.h` | Decode menu fields, links and clickable targets. |
| `api/title_guest_text.c` / `.h` | Own text-box layout, guest buffers and the original renderer. |
| `api/title_guest_calls.c` / `.h` | Preserve guest registers and expose native calls and transitions. |
| `api/title_guest_state.c` / `.h` | Name global game-state reads and writes. |
| `api/title_guest_player.c` / `.h`, `title_player_flight.c` | Separate player layout and native updates from host flight input. |
| `api/title_guest_geometry.c` / `.h`, `api/title_geometry_types.h` | Capture owned geometry snapshots and provide shared vector types. |
| `api/title_guest_routines.c` / `.h`, `api/title_guest_debug.c` | Retain original guest instruction semantics behind named operations. |
| `api/title_guest_symbols.c` | Map named implementations back to Xbox addresses in crash diagnostics. |
| `api/title_guest_diagnostics.c` / `.h` | Trace guest records and observe state for debugging. |
| `api/title_host_runtime.c` / `.h` | Configure runtime defaults and initialize recovered device integration. |
| `title_overrides.c` | Route original guest-address entry points to menu, timing, and gameplay operations. |
| `title_prompts.c` / `api/prompt_text.h` | Substitute keyboard labels during text drawing and preserve dialogue continuation offsets. |
| `api/title_saves.c` / `.h` | Map guest save paths to per-user storage and import legacy saves. |
| `api/save_dialogue.h` | Bound the elapsed time used by save-screen cooldowns. |
| `preview_auth.c` / `.h` | Run browser login and validate preview access when build locking is enabled. |
| `api/preview_auth_identity.c` / `.h`, `api/preview_auth_config.h` | Verify identity independently of browser/network orchestration. |
| `api/preview_auth_json.h` | Parse authentication JSON and encode/decode Base64URL values. |

## Following a guest call

`sub_000740D0` names the function at Xbox address `0x000740D0`. A `loc_` label identifies an instruction address inside that function. Keep these names so crash logs, disassembly, generated calls, and manual dispatch agree.

Guest routines share emulated registers. Arguments are read from the guest stack through `esp`; integer return values are normally placed in `eax`. `PUSH32` changes that stack, and `RECOMP_ABI_CALL` applies the runtime's guest calling convention. Replacing one of these calls with an ordinary C call can change register or stack behavior.

`flag_result` and `flag_operand` retain values used by translated integer branches. Their signed counterparts support signed comparisons; `float_compare_left` and `float_compare_right` support floating-point branches. These temporaries can outlive the instruction that produced them. Preserve their assignment order when editing a hook.

Start with the host helper called by an override to understand the custom behavior, then follow the surrounding guest labels if the failure involves original game logic. `recomp_manual.c` also contains wrappers used only for indirect calls; direct calls can reach the persistent `sub_` implementation through generated dispatch.

## Threads and diagnostics

Guest reads and calls belong on the title thread. The window thread queues actions and draws copied state. The geometry overlay owns its snapshots, and menu hit targets are protected by their existing locks. Keep that boundary when adding debug controls.

`--debug` enables the title's debug controls and writes diagnostics to `debug-render.log`. Startup faults are handled in `main.c`; unresolved guest call targets are logged in `recomp_manual.c`. Preview builds run authentication before entering the game.

## Style

Write gameplay and menu changes against helpers that express intent. Put recovered addresses, structure offsets, bit masks, and their bounds checks in an adapter for the relevant game system. A caller should say `menu_accepts_back(menu_mode)` and `remap_controller_press_edges(BUTTON_START, BUTTON_BACK)` without needing to know the memory layout.

Use Ghidra to trace reads, writes, and call sites, then debugging to test the proposed meaning during actual transitions. AI assistance can help follow the disassembly, suggest names, and build the adapter and regression checks. Record the evidence that supports the result: the XBE revision, function or instruction addresses, observed behavior, and any uncertainty. AI suggestions are hypotheses until checked. Only claim a Ghidra or debugging result when that work has actually been recorded.

### Menu input adapter: what we know

The first adapter extracts the behavior already present in `title_display.c`. Its purpose is to translate the title's Start press into its Back action where the native menu exposes Back. Start must still work on the initial title screen; ordinary pause and options modes keep their existing behavior. Only the current controller press edges change.

The existing implementation identifies mode `2` as the main menu and mode `7` as the map. It reads the accepted-Start flag at `0x1C2029`, the current scene at `0x1BF88C`, and four controller pointers at `0x1BEF10`. Controller press edges are at offset `0x30`. The map's font items use scene offsets `0x100` (count) and `0x1AC` (array), with a `0x100` stride. Item `0xDF911682` acts as the Back listener, and bit 0 at item offset `0x14` indicates visibility.

These layout meanings come from the previous implementation and its comments. This extraction does not establish their original Ghidra analysis or independent runtime verification; no corresponding analysis export or instruction references have been recorded here. The listener's original asset name remains unknown. Preserve that distinction when extending the adapter.

`BOOT_NOTES.md` records a Back-label follow-up: native menu event 66 uses B (`0x20000`), and the user confirmed the keyboard Back label should be L. Consequently `BUTTON_BACK` names the menu action implemented by the title's B bit; it is unrelated to the physical Xbox Back button or an XInput constant.

`tests/Check-MenuInput.py` checks the adapter against simulated guest memory: initial Start, main-menu Back, visible and hidden map listeners, other modes, controller slots, unrelated press bits, and invalid memory ranges. These checks establish the adapter's intended behavior; gameplay validation is separate.

Use four spaces, braces on separate lines, descriptive helper names, and comments that explain constraints or intent. `src/.clang-format` records this style. Format only the maintained C and header files; leave `recomp/` to the generator.
