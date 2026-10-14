"""Compile the production icon resource and verify Windows can load it."""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/application-identity-test'
out.mkdir(parents=True, exist_ok=True)
(out / 'application_identity.c').write_text(r'''
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <xbox/application_identity.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#c); return 1; } } while (0)
int main(void)
{
    HINSTANCE application = GetModuleHandleA(NULL);
    CHECK(!strcmp(DOGGAME_APP_NAME, "DogGame"));
    CHECK(FindResourceA(application, MAKEINTRESOURCEA(DOGGAME_ICON_ID), RT_GROUP_ICON));
    HICON large_icon = LoadIconA(application, MAKEINTRESOURCEA(DOGGAME_ICON_ID));
    HICON small_icon = (HICON)LoadImageA(application, MAKEINTRESOURCEA(DOGGAME_ICON_ID), IMAGE_ICON,
                                   GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    CHECK(large_icon && small_icon);
    ICONINFO details;
    CHECK(GetIconInfo(small_icon, &details));
    CHECK(details.fIcon);
    if (details.hbmColor) DeleteObject(details.hbmColor);
    if (details.hbmMask) DeleteObject(details.hbmMask);
    puts("Passed: DogGame branding and production icon resource load at application and title-bar sizes.");
    return 0;
}
''')
vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                        '-property', 'installationPath'], text=True).strip()
if not installation:
    raise RuntimeError('Visual Studio C++ build tools were not found.')
environment = Path(installation) / 'VC/Auxiliary/Build/vcvars64.bat'
commands = ['@echo off', f'call "{environment}" >nul', 'if errorlevel 1 exit /b 1',
            f'rc /nologo /I"{root / "Resources"}" /fo application.res "{root / "Resources/DogGame.rc"}"',
            'if errorlevel 1 exit /b 1',
            f'cl /nologo /std:c11 /O2 /I"{root / "runtime_overrides/include"}" application_identity.c application.res /Feapplication_identity.exe /link user32.lib gdi32.lib',
            'if errorlevel 1 exit /b 1']
(out / 'compile.cmd').write_text('\n'.join(commands))
subprocess.run(['cmd', '/c', 'compile.cmd'], cwd=out, check=True)
subprocess.run([str(out / 'application_identity.exe')], check=True)
