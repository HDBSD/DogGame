# Testing shortcuts

Preview-only access control is documented in [PREVIEW_BUILD_LOCKING.md](PREVIEW_BUILD_LOCKING.md). Set the CMake option `PREVIEW_BUILD_LOCKING=ON` to require the Discord tester role; use `OFF` for an unrestricted public build.

## Display resolution

Open **Options** from the main menu and select **Resolution**, the fourth native row below Sound Effects Volume, with the game's normal navigation and A button (Space on keyboard). The row uses the original menu font and blinking selection cursor. R/controller Y remain shortcuts to the selector. Inside the selector, use arrows/D-pad to choose, Enter/A to apply, and Esc/B to return; clicking a resolution applies it directly. Paused Options still provides the bottom-right Resolution entry. This setting does not require `--debug`.

Available window client sizes: 640×480, 800×600, 1024×768, 1280×720, 1280×960, 1600×900, 1920×1080, and 2560×1440. The selector also includes an **Aspect ratio: 4:3 / 16:9** row; press A/Enter to toggle. 16:9 widens the main world camera horizontally while retaining its vertical field of view. Presentation fits the selected aspect ratio with black bars where necessary; use a 16:9 window preset such as 1280?720 to fill the window. HUD and menu layout in widescreen still need visual verification. Manual window resizing and maximizing also scale the picture. Large presets may exceed smaller desktops.

The selected preset and aspect ratio are saved in `display.ini` beside `DogGame.exe` and restored on launch. Delete that file to reset to 640×480. This currently changes display/window size; internal rendering remains at the game's original resolution.

## Launch arguments

Run `DogGame.exe` from `SD100F/build/Release`, where its `game` and `saves` folders live. Use the same executable for ordinary play and debugging.

```powershell
.\DogGame.exe --nofmv
.\DogGame.exe --debug --level H001
.\DogGame.exe --debug --loadsaveslot 1
.\DogGame.exe --debug --loadsaveslot 1 --level W020 --noaudio
.\DogGame.exe --debug --nofmv --fpsgraph
```

| Argument | Behavior |
| --- | --- |
| `--nofmv` | Skip startup/logo movies and the Start prompt, opening the main-menu choices directly. Gameplay cutscenes remain enabled. |
| `--noaudio` | Start with silent output. Audio processing and completion timing continue so gameplay events can finish. |
| `--debug` | Add `[DEBUG]` to the window title, enable the pause debug page, and write renderer diagnostics to `debug-render.log`. |
| `--level <ID>` | Start directly in a level using its four-character scene ID, such as `H001` or `W020`. Requires `--debug`. IDs are case insensitive and must have installed HIP data. Direct H001 startup also skips its opening `cin1` movie. |
| `--loadsaveslot <1-3>` | Restore a save and enter its saved level. Requires `--debug`. Combined with `--level`, restore saved progress first, then enter the requested level. An unavailable save falls back to the menu and records an error in the debug log. |
| `--trace` | Request one frame trace at startup. Requires `--debug`. |
| `--vulkan` | Select the experimental Vulkan renderer. Requires `--debug`; ordinary launches retain D3D11. See [VULKAN_RENDERER.md](VULKAN_RENDERER.md) for limits. |
| `--profiler` | Start a ten-second renderer timing capture. Requires `--debug`. |
| `--fpsgraph` | Start with the FPS/frame-time graph visible. Requires `--debug`. |
| `--help` | Show argument help. |

Argument order does not matter. Level, save, and diagnostic arguments without `--debug` produce an argument error before starting the game. `--nofmv` and `--noaudio` work independently.

## Pause debug page

Pause normally, then press **F1**, press **controller Back**, or click the **Debug** entry at the bottom left. Use arrows/D-pad to select a row, left/right to change a level or power-up, Enter/A to apply, and Esc/B to return to the native pause menu. Guest controls are captured while this page is open.

- **Warp to level:** lists scene IDs discovered from installed HIP files (87 in the current installation). Warping resumes gameplay and preserves the current power-up bits.
- **Power-up:** toggle bubble gum, soap bubbles, sticky boots, plungers, slippers, lampshade, Black Knight, spring/double jump, lightning bolt, football helmet, umbrella, shovel, or lightning stun. These change the title's ability flags, not story progression or collected inventory.
- **Trace next frame:** record one complete renderer frame in `debug-render.log`; repeat when needed. Look for `[FTRACE]` begin/end markers. Detailed tracing briefly slows that frame.
- **Capture 10 second renderer profile:** arms a timing capture and returns to the native pause menu. Resume normally to start recording gameplay. The interval's flip count and renderer, vertex, bridge, and geometry times appear under `[DEBUG-PROFILE]` in `debug-render.log`.
- **Capture 10 second render trace:** arms detailed draw tracing and returns to Pause. Resume normally to start a ten-second, complete-frame capture under `[FTRACE]`. This intentionally slows rendering and can produce a large log. Both capture actions show an armed/recording status on screen.
- **FPS graph / level / coordinates:** display recent presented-frame timing, the current four-character level ID, and Scooby's world X/Y/Z position in one compact panel with a 40% transparent black backing. Also enabled by `--fpsgraph`; gameplay pacing is unchanged.
- **Ignore warp gate requirements:** allow unactivated destinations in the native warp menu without changing saved activation flags.
- **Undamageable by monsters:** block damage from native monster entities, including bosses. Floor hazards and falls remain active.
- **Scooby noclip:** enable collision-free flight without gravity. WASD/left stick moves horizontally relative to the camera; **Q/E** moves down/up, or **LT/RT** on a controller. Ordinary attacks and interactions are suspended during flight. Disable noclip while clear of walls and above a floor to return to ordinary gameplay. Motion stops when focus is lost or an overlay captures input.
- **Ignore snack gate requirements:** open snack gates without sufficient snacks and waive the snack cost. The normal gate animation and unlock events still run.
- **Add 100 snacks:** award 100 using the native snack addition function. These snacks, and gates opened while testing, can be retained if you save afterward.

Bypass, monster protection, and noclip toggles start OFF each launch and require `--debug`; they are not saved as settings. Flower pot and diving bell/helmet have been removed from the power-up selector without changing the remaining ability-bit mappings.

Keyboard input is enabled by default; `RECOMP_KEYBOARD=0` disables it. Input only applies while the game window has focus and can be used alongside a controller. Enter remains a Start alias, Z an A alias, and X a B alias.

`debug-render.log` is replaced on each debug launch; copy it before restarting when retaining a capture. The existing `xbox_kernel.log` remains available. Warps use native scene cleanup/loading, but the entire installed level catalog has not been play-tested; some scenes require story state supplied by a suitable save.

## Keyboard controls

During active gameplay, the mouse cursor is hidden and confined to the game window's client area. It is visible and free in the main menu, pause menu, and map/warp-gate menus. Alt-Tab, minimizing, moving or resizing the window releases it; returning to active gameplay captures it again. This also works without `--debug`.

Move the mouse over native menu choices to highlight them; left-click to activate them, including main-menu submenus, pause choices, and map/warp-gate icons on the current page. Nested warp pages use their own clickable icons, including navigation-only icons. The native Back label is clickable where present; Escape/L remains available for returning. Mouse menu input does not require `--debug`.

| Key | Controller input |
| --- | --- |
| W / A / S / D | Left stick movement |
| Left Shift | RT |
| Left Ctrl | RB (Xbox black button) |
| Escape | Start / pause |
| Space | A |
| J | X |
| K | Y |
| L | B |
| U | LT |
| I | LB (Xbox white button) |
| Arrow keys | D-pad / menu navigation |
| Backspace | Back |

In-game text prompts follow the last used input: keyboard input shows bracketed key labels (for example, `[L]` for B and `[SPACE]` for A), and a controller button press or intentional stick movement restores controller icons. Idle controller drift and the automatic startup Start press do not switch the labels. This is enabled without `--debug`.

Resolution and debug selectors also accept WASD, Space to choose, and L or Escape to return. Left and right modifiers are distinguished; the old Shift/Ctrl thumb-click bindings are removed.
# Trigger and collision views

Pause → Debug now includes **Show triggers** and **World collision view**.
Scroll down to reach these rows. Both are session-only toggles requiring
`--debug`, and both are hidden while the native pause/map/main menus are open.

Trigger boxes have translucent faces and coloured outlines: red for player
death/fall-to-death links, blue for level portals, yellow for other triggers,
and grey for disabled triggers. Labels include the asset ID. Classification
follows up to three levels of linked events; ambiguous triggers retain the
generic label. Sphere/cylinder triggers show enclosing boxes marked “bounds”.

World collision view overlays the actual static collision world at 50% opacity,
using shaded faces and triangle edges. The game remains visible underneath.
Dynamic object collision is not included. It can be combined with
Show triggers. Gameplay and collision behaviour are unchanged.

Collision geometry is copied once per level/toggle; camera and triggers update
each frame. This diagnostic rendering can reduce FPS while enabled. If a level
has no readable collision mesh, the normal picture remains with a status
message rather than displaying its visual mesh as if it were collision data.

# Audio corruption capture

Main menu **Recomp Settings** replaces Promo Materials and uses the green
computer-screen presentation. Its **Resolution** submenu contains display
sizes and the 4:3/16:9 aspect toggle. **Mouse interaction** enables or disables
menu hover and clicks, including the clickable debug shortcut. The preference
is saved in `display.ini`; keyboard and controller navigation remain available
when mouse interaction is OFF. Back returns from Resolution to Recomp Settings,
then to the main menu. These settings do not require `--debug`.
**Dynamic button prompts** defaults to ON. Turning it OFF keeps the original
Xbox button icons when using a keyboard; controls are unchanged. This preference
is also saved in `display.ini`.

Resolution settings use rows for Resolution, Aspect ratio, Scaling mode and
Display mode. Up/Down selects a row; Left/Right changes its value. The mouse
can click the arrows beside a value. Aspect ratio selects 4:3 or true 16:9.
Window scaling changes the window size while keeping native render resolution.
Internal scaling changes GPU render resolution independently of window size,
presenting the full-resolution GPU image directly. A native-resolution copy is
retained for game compatibility; software/FMV frames fall back to that copy.
Higher internal sizes may reduce FPS; this does not change game timing.
Window and internal resolution preferences are stored separately. Borderless
fills the current monitor; Windowed restores the bordered window. Settings
are saved in `display.ini`.
The main menu includes **Quit Game** below Credits. Select it with the keyboard,
controller or mouse to close the game using the same shutdown path as closing
the game window.

Save slots are ordinary files in `%APPDATA%\sd100f\saves`: `save1.sav`,
`save2.sav`, and `save3.sav`. The first launch copies existing saves from
`game\UDATA\5451001f` without replacing files already in the new location or
deleting the originals. A hidden `.metadata` folder retains the small metadata
needed by the native Load/Save menus. The migration record lives separately in
`%LOCALAPPDATA%\sd100f\storage\save-migration-v1.done`, so deleting the entire
Roaming `sd100f` folder does not automatically restore legacy saves.
Emulated partition/cache backing files live separately under
`%LOCALAPPDATA%\sd100f\storage`; they are not part of the three save files.

Launch with `--debug`. If audio becomes corrupted, press **F8** while the game
window has focus, preferably before pausing. Keep it running for another ten
seconds. This saves `audio-capture-<timestamp>-<PID>.wav` and a matching `.txt`
beside the executable (the launch working directory). The WAV contains up to
ten seconds before the keypress and ten seconds afterward, at 48 kHz stereo.
Repeated requests during a capture are ignored; another capture can be made
after completion. Normal non-debug launches do not retain this audio history.

You can also select **Capture 10 second audio trace (after unpausing)** in the
Debug menu. It arms the capture while paused and starts the following ten
seconds when gameplay resumes. The existing pre-trigger history is retained.
Both this action and F8 show an on-screen recording countdown, followed by a
five-second saved/failed notice. The countdown follows processed audio frames;
if audio processing stalls, it does not falsely report that recording finished.

Send both capture files and `debug-render.log` before the next launch overwrites
the log. Include the level, action immediately before the problem, and whether
music, speech, effects or all audio were affected. Debug mode also records
XAudio2 queue/underrun statistics every five seconds. The text capture records
active hardware/software voice state at the trigger and during the following
ten seconds. The companion report also includes XAudio2 queue depth, samples
played, underruns, full-queue drops, submission/start failures and the last
recorded error. Each snapshot includes the mixed frame's peak and count of
samples at the 16-bit limits. Saturation alone does not identify its source. The WAV is the mixed signal before Windows playback: if it sounds
normal while the live output was corrupted, investigate the output backend or
device; if it contains the corruption, investigate voice data, decoding or
mixing. Clipping alone does not prove memory corruption.


Recomp Settings includes **30 FPS cap** (OFF by default). ON limits guest frame presentation to 30 FPS without changing the refresh clock or simulation timestep. It cannot raise low FPS. The setting persists as `FpsCap30` in `display.ini`.

**Pause on focus loss** defaults to ON and is saved as `PauseOnFocusLoss` in
the `[Display]` section of `display.ini`. Switching to another window requests
the game's normal pause action during gameplay. Existing menus and cutscenes
are left alone; returning focus does not automatically resume. Turn the option
OFF to keep gameplay running when the window loses focus.

### Renderer comparison switches

Set `RECOMP_GPU_INDEXED_UPLOAD=0` before launching to compare the direct triangle upload path against the default batch-local indexed path. `[GPU-GEOMETRY-TRANSFER]` reports the expanded-equivalent vertex payload, actual vertex/index upload bytes, indexed draw count and reused vertex references. These are cumulative counters; uploads and timing alone do not establish an FPS improvement.

`RECOMP_GPU_STATE_CACHE=0` disables redundant binding/constant-upload avoidance. `[GPU-SUBMISSION]` reports the corresponding issued/skipped operations. These environment switches are renderer diagnostics, independent of the user-facing graphics settings. They are read once per process.

`RECOMP_GPU_RESIDENT_SURFACES=0` disables the retained surface-content path for comparison, using dimension-based allocation reuse and the legacy rendered-texture snapshot cache. The default retains GPU attachment contents, validates CPU edits before skipping uploads, binds compatible inactive targets directly as textures, and reproduces uniform contents with attachment clears. `[GPU-RESIDENT-SURFACES]` reports per-attachment reuse hits, direct texture bindings, upload replacements using clears, actual upload payload and inactive pair count. Existing readback boundaries remain enabled in both modes. Inactive resource payload is bounded to eight pairs and 128 MiB, excluding the active pair and CPU validation storage.
