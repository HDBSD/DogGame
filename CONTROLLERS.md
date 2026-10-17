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

`runtime_overrides/src/input/host_gamepad.c` owns SDL controller handles, polling, stable ports, packet changes, hot-plugging and rumble. Calls serialize internally and never access guest memory. `xinput_device.c` retains Xbox conversion and keyboard/overlay policy. SDL controller initialization failure logs the error and falls back to native Windows XInput. Routine disconnect returns a neutral state and a disconnected result. Joystick detection uses SDL's Windows joystick thread, independent of the game's Win32 message loop; unused SDL joystick/controller events are disabled to avoid an unconsumed event queue.

CMake fetches the official release archive using a pinned SHA-256 and builds SDL statically. Setup's normal game build therefore downloads source automatically; no system SDL installation or additional toolchain is selected. `SDL-LICENSE.txt` is copied alongside `DogGame.exe` and retained when setup copies the output.

Configure with `-DDOGGAME_BUILD_CONTROLLER_TESTS=ON`, build `doggame_controller_test`, and run it from the runtime input build directory. The test disables physical device backends and attaches virtual PS4/PS5-style SDL gamepads. It checks button press/release mapping, unchanged/changed packets, neutral and extreme axes, independent triggers, rumble callbacks, guest conversion, overlay suppression, disconnect/reconnect and stable ports. `tests/Check-InputPrompts.py` checks existing keyboard/prompt/Start behavior.

Portable Clang and MSVC game builds and virtual-controller tests pass. Existing prompt/keyboard tests pass. PE import inspection confirms there is no SDL DLL dependency, and the copied license matches the source notice. A 35-second H001 game smoke test succeeded and detected a physical `DualSense Wireless Controller` (`054C:0CE6`) on player port 1. Logs are retained under `build/graphics-audit/controller-*`.

Virtual devices validate normalized SDL state and the host-to-guest boundary; the smoke test additionally establishes physical DualSense enumeration. Neither establishes actual button/axis behavior over both USB and Bluetooth, radio behavior, real motors or a physical DualShock 4. Those hardware checks remain outstanding.

Sources: [pinned SDL release](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10), [Windows joystick-thread contract](https://wiki.libsdl.org/SDL2/SDL_HINT_JOYSTICK_THREAD), and the release's `src/joystick/hidapi/SDL_hidapi_ps4.c`, `SDL_hidapi_ps5.c` and `include/SDL_hints.h`. This uses SDL's maintained transport/mapping implementation; no fresh recovery of Sony reports or guest input routines is claimed.
