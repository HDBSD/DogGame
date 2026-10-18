# Controllers on Windows

DogGame accepts Xbox controllers, DualShock 4 (PS4) and DualSense (PS5) through a statically linked SDL 2.32.10 controller backend. SDL's Sony HID drivers cover USB and Bluetooth. Connect the controller in Windows before or while playing; new devices are assigned to the first free controller port within the adapter's one-second rescan interval. Attached devices keep their ports when another pad disconnects. Up to four controllers are exposed.

The game continues to receive its existing Xbox controller format and displays Xbox prompts. No DS4Windows, Steam Input or SDL DLL installation is required.

| PlayStation control | Xbox control |
| --- | --- |
| Cross | A |
| Circle | B |
| Square | X |
| Triangle | Y |
| Options | Start |
| Share / Create | Back |
| L1 / R1 | LB / RB (existing original-Xbox Black / White mapping) |
| L2 / R2 | Analog LT / RT |
| L3 / R3 | Left / right stick click |
| D-pad and sticks | Corresponding Xbox controls |

The PS button, touchpad gestures, gyro, adaptive triggers and controller speaker are not game controls. Standard low/high-frequency rumble is forwarded through the existing host vibration API. Actual in-game vibration also depends on the guest USB/output path invoking that API; the new adapter does not add guest USB output handling.

Keyboard merging, overlay capture, synthetic Start and last-input prompt switching use the same existing conversion as Xbox pads. Windows controller input remains active independently of keyboard focus, matching the previous XInput behavior.

If a separate remapper exposes both a physical controller and a virtual Xbox controller, they may occupy two ports. Use the direct connection or configure the remapper to expose only one device. SDL's Bluetooth rumble mode enables enhanced Sony reports; applications using legacy DirectInput may require the controller to be reconnected afterward. The standard SDL `SDL_JOYSTICK_HIDAPI_PS4_RUMBLE=0` / `SDL_JOYSTICK_HIDAPI_PS5_RUMBLE=0` environment overrides can disable that mode.

## Implementation and validation

`runtime_overrides/src/input/host_gamepad.c` owns SDL controller handles, polling, stable ports, packet changes, hot-plugging and rumble. Calls serialize internally and never access guest memory. `xinput_device.c` retains Xbox conversion and keyboard/overlay policy. The public host interface is `runtime_overrides/include/xbox/host_gamepad.h`; debug menus, recomp settings and debug flight use this interface rather than bypassing it with native XInput. SDL controller initialization failure logs the error and falls back to native Windows XInput. Routine disconnect returns a neutral state and a disconnected result. Joystick detection uses SDL's Windows joystick thread, independent of the game's Win32 message loop; unused SDL joystick/controller events are disabled to avoid an unconsumed event queue.

The host adapter requests a sample every 4 ms on its own thread, independently of guest updates and menu drawing. Each menu owns a separate ordered press-queue cursor, so a button pressed and released between rendered frames remains available to both menus. Reading current gameplay state never consumes menu presses, and menu history never turns a released button into a held guest button. Holding a button produces one edge; multiple taps retain their order and are drained during the next menu update. Each port retains 128 press events; a stalled reader that falls further behind loses the oldest events and reports an overflow. Disconnect/reconnect discards the previous device's history. Shutdown signals and joins the sampler before releasing SDL handles. If thread creation fails, polling remains available and a diagnostic explains that short taps may be missed. Sampling is subject to Windows scheduling; this is not a guarantee of capturing arbitrarily short pulses.

This fixes two code-established faults: `title_debug.c:debug_draw` and `title_display.c:display_draw` previously called native `XInputGetState`, excluding Sony pads handled by SDL, and calculated edges only at draw time. Debug flight also bypassed SDL. This is a host input correction, with no recovered guest layout or revision-specific behavior changed.

CMake fetches the official release archive using a pinned SHA-256 and builds SDL statically. Setup's normal game build therefore downloads source automatically; no system SDL installation or additional toolchain is selected. `SDL-LICENSE.txt` is copied alongside `DogGame.exe` and retained when setup copies the output.

Configure with `-DDOGGAME_BUILD_CONTROLLER_TESTS=ON`, build `doggame_controller_test`, and run it from the runtime input build directory. The test disables physical device backends and attaches virtual PS4/PS5-style SDL gamepads. It checks button press/release mapping, unchanged/changed packets, neutral and extreme axes, independent triggers, rumble callbacks, guest conversion, overlay suppression, disconnect/reconnect and stable ports. It also simulates a 120 ms interval without menu/guest polling around a 40 ms tap: both independent menu readers receive the released tap once, while gameplay sees neutral state. Held-button deduplication and history reset after reconnect are checked. `tests/Check-InputPrompts.py` checks existing keyboard/prompt/Start behavior.

Portable Clang and MSVC game builds and virtual-controller tests pass. Existing prompt/keyboard tests pass. PE import inspection confirms there is no SDL DLL dependency, and the copied license matches the source notice. A 35-second H001 game smoke test succeeded and detected a physical `DualSense Wireless Controller` (`054C:0CE6`) on player port 1. Logs are retained under `build/graphics-audit/controller-*`.

Virtual devices validate normalized SDL state and the host-to-guest boundary; the smoke test additionally establishes physical DualSense enumeration. Neither establishes actual button/axis behavior over both USB and Bluetooth, radio behavior, real motors or a physical DualShock 4. Those hardware checks remain outstanding.

Sources: [pinned SDL release](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10), [Windows joystick-thread contract](https://wiki.libsdl.org/SDL2/SDL_HINT_JOYSTICK_THREAD), and the release's `src/joystick/hidapi/SDL_hidapi_ps4.c`, `SDL_hidapi_ps5.c` and `include/SDL_hints.h`. This uses SDL's maintained transport/mapping implementation; no fresh recovery of Sony reports or guest input routines is claimed.

### Menu update cadence

Menu input policy now runs once per window-loop iteration (normally a 16 ms sleep plus work), before frame acquisition, rather than in the debug/settings draw callbacks. Window painting and `WM_PRINTCLIENT` do not consume controller edges. Frame publication can hold its lock across snapshot conversion and world-diagnostic capture; inspection found that the window previously waited on this lock before processing either menu. The window now tries the lock without waiting and retains a UI-owned bitmap of the last complete background, including its matching world diagnostics. Menus redraw over a fresh copy of that bitmap while publication is busy; borrowed image/camera pointers are never reused after unlocking. Two window-sized GDI canvases are released on resize/shutdown.

`tests/Check-DisplayResolution.py` deliberately holds the publication lock for one second and verifies at least 20 input updates and menu redraws during that interval, unchanged background pixels, no new world-diagnostic reads while locked out, and normal refresh after release. This establishes the contention failure path and fix; it does not measure the cause of a particular physical controller session. Keyboard/controller responsiveness in the actual game still needs user validation.

### Rapid taps and input tracing

The previous per-button counters collapsed repeated taps between UI updates and combined different buttons into a mask, losing their sequence. The adapter now publishes an ordered 128-entry ring per port with independent reader cursors. Menu updates drain that queue; they acquire only a short snapshot-copy lock, not the lock held during SDL updates, device discovery or rumble. If the sampler cannot start, synchronous polling remains the logged fallback. This still samples current physical state: a press and release entirely between device samples can be missed, especially during an OS scheduling stall. An initial read/reconnection starts from current held state rather than replaying an old device's history.

The virtual-controller test sends Down, Down, Confirm, Up without menu reads and verifies the exact sequence for two independent consumers, with neutral held gameplay state afterward. The display test feeds Down, Down, Confirm into the actual settings input callback and verifies the correct row is activated.

For an unresolved in-game miss, set `$env:DOGGAME_INPUT_TRACE='1'` before launching the game and capture stderr (for example `./DogGame.exe 2> input-trace.log`). The trace records normalized controller button transitions, edges processed by each menu with its availability/open state, keyboard messages and UI update gaps over 100 ms. Tracing is off by default and can affect timing; these logs establish which stages observed an input, not physical USB/Bluetooth delivery.

### Native main/load/save menus

The added-menu queue does not feed the game's native menus. Those obtain live state via `xbox_InputGetState` -> USB reports -> guest XAPI -> `sub_00090F50`. The translated sequence at `loc_00091192` computes pressed as new-held AND NOT old-held, stores it at controller+0x30, and stores new-held at +0x2C. This is evidence from the existing translation; it is not a fresh disassembly recovery.

With `DOGGAME_INPUT_TRACE` enabled, `USB-source` records state transitions handed to the USB layer and long gaps between reads. `native` records player one's held/pressed words and mode at existing title-thread frame/input hooks, before the custom Start remap. These are observations at their named boundaries, not atomic snapshots of asynchronous USB/XAPI state. Diagnostic previous-state tracking is thread-local; logging never consumes or latches a guest press.

A hidden `--nofmv --noaudio` main-menu probe posted six 80 ms keyboard taps (two Enter, then Down/Up/Down/Up). All six appeared in USB-source state and then in native press edges; capture remained off. Logs: `build/native-menu-probe/input-trace-nofmv.log`. This did not reproduce the reported intermittent miss, validate visible highlight changes, or exercise native Load/Save. No native input timing fix has been established by this probe.

To capture an actual failure from PowerShell in the installed game folder:

```powershell
$env:DOGGAME_INPUT_TRACE = '1'
.\DogGame.exe 2> native-input-trace.log
Remove-Item Env:DOGGAME_INPUT_TRACE
```

Reproduce the miss and exit, retaining `native-input-trace.log`. Include which key/button was ignored and whether navigation, confirmation or Back failed. Trace logging can affect timing.

User-provided native-menu trace analysis (2026-10-18): after the first mode-2 observation at host tick 232275015, all 93 traced USB-source button rises (30 Down, 35 Up, 9 A, 18 B, 1 Start) have corresponding native pressed bits within the next 160 ms observation window (allowing one 16 ms trace-clock tick for concurrent logging order). Overlay capture is zero throughout those observations. The capture contains controller events and no keyboard-down trace records. Earlier A/Start inputs occur before the main-menu observations begin and cannot be classified as lost menu inputs by this log. A native press is not evidence that its UI listener handled it.

The timing diagnostic records IEEE float bits as `main_bits`, `save_bits` and `minimum_bits` on `native-edge-timing-v2` lines. It logs these only on presses, avoiding per-frame floating-point formatting that changes the observed loop rate. Main/save loops poll input before calling `sub_000791B0`; that routine returns immediately at `loc_000794EB` when elapsed is at or below the float at 0x0018131C. Zero-time edge loss is a candidate, not established by the supplied trace, which lacks those delta fields. A 24-pulse scripted main-menu probe observed positive deltas for its recorded edges and did not reproduce that candidate. No clock or native input timing workaround was applied without confirmation.

### Native menu clock correction

A subsequent navigation-only probe reproduced a native edge at host tick 233230171 with `main_bits=00000000` (zero elapsed) while the UI minimum was `3727C5AC` (about 0.00001 s). At that boundary, the controller update had consumed the edge and the following UI call took its elapsed-time early return. This establishes a concrete loss path shared by the main and nested Load/Save loops. The user's three marked dropped presses were also visible as native Down edges, but their older-format log does not establish their elapsed values individually.

`runtime_overrides/src/kernel/guest_clock.c` now supplies a QPC-backed millisecond clock for Windows, anchored once to host uptime. It supplies the guest KeTickCount data publication/initialization and KeQueryInterruptTime using a common epoch/rate, with explicit 32-bit truncation for KeTickCount. Frequency/epoch initialization is synchronized. Unsupported QPC initialization falls back to the existing host uptime clock; non-Windows keeps its existing compatibility clock. No title stack/register sequence, USB report or controller hold duration changes. A 32-bit guest tick can still wrap, and truly sub-millisecond loop iterations can still have zero elapsed; this corrects the larger host tick quantization rather than inventing elapsed time.

The same 36-scheduled-pulse probe ran for 14.5 seconds before and after: among the observed edges, before recorded 1 edge on a skipped UI tick (27 edges total), after recorded 0 (28 total). The finite runs do not claim every scheduled pulse was reached. Logs are `build/native-menu-probe/clock-edge-before.log` and `clock-edge-after.log`. `tests/Check-GuestClock.py` verifies the actual clock implementation with deterministic coarse/fine clocks, concurrent initialization, millisecond advances, 32-bit wrap, long-run conversion and unavailable-QPC fallback. Portable and MSVC game builds and menu/input regressions pass. Physical controller operation and native Load/Save behavior after the correction still need user validation.
