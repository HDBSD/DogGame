"""Check persisted renderer policy and confirmation without opening real dialogs."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build/renderer-settings-test"
out.mkdir(parents=True, exist_ok=True)
display = (root / "src/title_display.c").read_text()
start = display.index("static int renderer_confirmation_key(")
confirmation = display[start:display.index("\n}\n", start) + 3]
fixture = r'''
#include <windows.h>
#include <assert.h>
#include <stdio.h>
''' + f'#include "{(root / "src/api/title_renderer_settings.c").as_posix()}"\n'
fixture += "static int s_renderer_confirmation, s_renderer_pending_vulkan;\n" + confirmation + r'''
int main(void)
{
    wchar_t path[MAX_PATH];
    assert(renderer_settings_path(path));
    DeleteFileW(path); // Only this fixture's own display.ini beside its executable.
    assert(title_renderer_settings_prefer_vulkan());
    WritePrivateProfileStringW(L"Display", L"MouseInteraction", L"0", path);
    assert(title_renderer_settings_save(0));
    assert(!title_renderer_settings_prefer_vulkan());
    assert(GetPrivateProfileIntW(L"Display", L"MouseInteraction", 1, path) == 0);
    s_renderer_confirmation = 1;
    s_renderer_pending_vulkan = 1;
    assert(renderer_confirmation_key(VK_DOWN) && s_renderer_confirmation == 1);
    renderer_confirmation_key(VK_ESCAPE);
    assert(!title_renderer_settings_prefer_vulkan()); // Decline leaves saved choice intact.
    s_renderer_confirmation = 1;
    renderer_confirmation_key(VK_RETURN);
    assert(title_renderer_settings_prefer_vulkan());
    s_renderer_confirmation = 1;
    s_renderer_pending_vulkan = 0;
    renderer_confirmation_key('Z');
    assert(!title_renderer_settings_prefer_vulkan()); // A second change can undo pending selection.
    WritePrivateProfileStringW(L"Display", L"Renderer", L"invalid", path);
    assert(title_renderer_settings_prefer_vulkan());
    assert(title_renderer_settings_save(0));
    SetFileAttributesW(path, FILE_ATTRIBUTE_READONLY);
    s_renderer_confirmation = 1;
    s_renderer_pending_vulkan = 1;
    renderer_confirmation_key(VK_RETURN);
    assert(s_renderer_confirmation == 2 && !title_renderer_settings_prefer_vulkan());
    renderer_confirmation_key(VK_ESCAPE);
    assert(!s_renderer_confirmation);
    SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL);
    DeleteFileW(path);
    puts("Passed: Vulkan default, DirectX preference, confirmation/cancellation, unrelated settings and write failure.");
}
'''
(out / "fixture.c").write_text(fixture)
compiler = root / "build/portable/tools/llvm/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe"
subprocess.run([str(compiler), "-std=c11", str(out / "fixture.c"), "-luser32", "-o", str(out / "fixture.exe")], check=True)
subprocess.run([str(out / "fixture.exe")], check=True)
