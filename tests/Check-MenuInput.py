"""Exercise the production menu adapter and display caller with simulated guest memory."""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/menu-input-test'
out.mkdir(parents=True, exist_ok=True)
adapter = (root / "src/api/title_guest_menu.c").read_text()
adapter = adapter.replace('#include "gen/recomp_types.h"', '')
adapter = adapter.replace('#include "title_guest_menu.h"', '')
display = (root / "src/title_display.c").read_text()
start = display.index('static void remap_start_to_back_if_needed(')
caller = display[start:display.index('\n}\n', start) + 3]

prefix = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned char memory[0x4000000];
#define MEM32(address) (*(uint32_t *)(memory + (address)))
#define MEM8(address) memory[(address)]
#define MEMF(address) (*(float *)(memory + (address)))
'''
checks = r'''
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #condition); return 1; } } while (0)
int main(void)
{
    uint32_t scene = 0x20000, items = 0x30000;
    MEM32(0x1BF88C) = scene;
    MEM32(scene + 0x100) = 2;
    MEM32(scene + 0x1AC) = items;
    MEM32(items) = 0xDF911682u;
    MEM8(items + 0x14) = 1;

    MEM32(0x1B8D9C) = 2;
    CHECK(title_menu_current_mode() == TITLE_MENU_MAIN);
    CHECK(!menu_accepts_back(TITLE_MENU_MAIN));
    MEM8(0x1C2029) = 1;
    CHECK(menu_accepts_back(TITLE_MENU_MAIN));
    CHECK(menu_accepts_back(TITLE_MENU_MAP));
    for (uint32_t mode = 0; mode < 16; mode++)
    {
        if (mode != 2 && mode != 7) CHECK(!menu_accepts_back(mode));
    }

    MEM8(items + 0x14) = 0;
    CHECK(!menu_accepts_back(TITLE_MENU_MAP));
    MEM8(items + 0x14) = 2;
    CHECK(!menu_accepts_back(TITLE_MENU_MAP));
    MEM32(items + 0x100) = 0xDF911682u;
    MEM8(items + 0x100 + 0x14) = 1;
    CHECK(menu_accepts_back(TITLE_MENU_MAP));

    for (unsigned index = 0; index < 4; index++)
    {
        uint32_t controller = 0x40000 + index * 0x100;
        MEM32(0x1BEF10 + index * 4) = controller;
        MEM32(controller + 0x30) = index == 3 ? 0x40 : 0x41;
        MEM32(controller + 0x2C) = 0x12345678; /* Adjacent held/input state. */
    }
    MEM8(0x1C2029) = 0;
    remap_start_to_back_if_needed(TITLE_MENU_MAIN);
    CHECK(MEM32(0x40030) == 0x41); /* Initial Press Start is preserved. */
    remap_start_to_back_if_needed(11);
    CHECK(MEM32(0x40030) == 0x41); /* Gameplay is preserved. */
    MEM8(0x1C2029) = 1;
    remap_start_to_back_if_needed(TITLE_MENU_MAIN);
    for (unsigned index = 0; index < 4; index++)
    {
        uint32_t controller = 0x40000 + index * 0x100;
        CHECK(MEM32(controller + 0x30) == (index == 3 ? 0x40 : 0x20040));
        CHECK(MEM32(controller + 0x2C) == 0x12345678);
    }
    remap_start_to_back_if_needed(TITLE_MENU_MAIN);
    CHECK(MEM32(0x40030) == 0x20040); /* Repeated frame hooks are harmless. */
    MEM32(0x40030) = 0x41;
    remap_start_to_back_if_needed(TITLE_MENU_MAP);
    CHECK(MEM32(0x40030) == 0x20040);
    MEM8(items + 0x100 + 0x14) = 0;
    MEM32(0x40030) = 0x41;
    remap_start_to_back_if_needed(TITLE_MENU_MAP);
    CHECK(MEM32(0x40030) == 0x41);

    const uint32_t invalid_scenes[] = {0, 0xFFFF, 0x4000000 - 0x1B0, 0xFFFFFFFF};
    for (unsigned i = 0; i < 4; i++) CHECK(!title_menu_map_page_visible(invalid_scenes[i], TITLE_MENU_MAP));
    MEM32(scene + 0x100) = 4097;
    CHECK(!title_menu_map_page_visible(scene, TITLE_MENU_MAP));
    MEM32(scene + 0x100) = 1;
    const uint32_t invalid_items[] = {0, 0xFFFF, 0x4000000, 0x4000000 - 0x80, 0xFFFFFFFF};
    for (unsigned i = 0; i < 5; i++)
    {
        MEM32(scene + 0x1AC) = invalid_items[i];
        CHECK(!title_menu_map_page_visible(scene, TITLE_MENU_MAP));
    }
    const uint32_t invalid_controllers[] = {0, 0xFFFF, 0x4000000 - 0x34, 0xFFFFFFFF};
    for (unsigned i = 0; i < 4; i++) MEM32(0x1BEF10 + i * 4) = invalid_controllers[i];
    MEM32(0x4000000 - 4) = 0x41;
    remap_controller_press_edges(BUTTON_START, BUTTON_BACK);
    CHECK(MEM32(0x4000000 - 4) == 0x41);
    /* Diagnostic snapshots must copy without consuming native edges. */
    MEM32(0x1BEF10) = 0x40000;
    MEM32(0x4002C) = 0x40;
    MEM32(0x40030) = 0x40;
    MEMF(0x1A4F14) = 0.0f;
    MEMF(0x1A5220) = 0.016f;
    MEMF(0x18131C) = 0.00001f;
    TitleMenuInputSnapshot observation;
    CHECK(title_menu_input_snapshot(&observation));
    CHECK(observation.held == 0x40 && observation.pressed == 0x40);
    CHECK(observation.main_elapsed == 0.0f && observation.save_elapsed == 0.016f);
    CHECK(observation.ui_minimum_elapsed == 0.00001f && MEM32(0x40030) == 0x40);
    MEM32(0x1BEF10) = 0;
    CHECK(!title_menu_input_snapshot(&observation) && !observation.held && !observation.pressed);
    puts("Passed: menu gating, map listeners, press-edge remapping, all controller slots, held state and guest bounds.");
    return 0;
}
'''
header = '#include "' + (root / 'src/api/title_guest_menu.h').as_posix() + '"\n'
(out / "menu_input.c").write_text(prefix + header + adapter + caller + checks)
vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                        '-property', 'installationPath'], text=True).strip()
if not installation:
    raise RuntimeError('Visual Studio C++ build tools were not found.')
environment = Path(installation) / 'VC/Auxiliary/Build/vcvars64.bat'
# Invoke the compiler directly: this standalone fixture does not need MSBuild tracking.
commands = ['@echo off', f'call "{environment}" >nul', 'if errorlevel 1 exit /b 1',
            'cl /nologo /std:c11 /O2 menu_input.c /Fomenu_input.obj /Femenu_input.exe',
            'if errorlevel 1 exit /b 1']
(out / 'compile.cmd').write_text('\n'.join(commands))
subprocess.run(['cmd', '/c', 'compile.cmd'], cwd=out, check=True)
subprocess.run([str(out / "menu_input.exe")], check=True)
