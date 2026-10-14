"""Check production runtime defaults in an isolated process environment."""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/runtime-defaults-test'
out.mkdir(parents=True, exist_ok=True)
source = (root / 'src/api/title_host_runtime.c').read_text()
configure = source[source.index('typedef struct RuntimeEnvironmentDefault'):source.index('void title_host_initialize_devices(')]
test = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
CONFIGURE
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#c); return 1; } } while (0)
int main(void)
{
    _putenv_s("RECOMP_GPU", "0");
    _putenv_s("RECOMP_USB_PORT", "custom");
    _putenv_s("RECOMP_HEAP_RECLAIM", "");
    _putenv_s("RECOMP_CONTIG_RECLAIM", "");
    _putenv_s("RECOMP_USB_NDP", "");
    title_host_configure_runtime();
    CHECK(!strcmp(getenv("RECOMP_GPU"), "0"));
    CHECK(!strcmp(getenv("RECOMP_USB_PORT"), "custom"));
    CHECK(!strcmp(getenv("RECOMP_HEAP_RECLAIM"), "1"));
    CHECK(!strcmp(getenv("RECOMP_CONTIG_RECLAIM"), "1"));
    CHECK(!strcmp(getenv("RECOMP_USB_NDP"), "4"));
    _putenv_s("RECOMP_HEAP_RECLAIM", "0");
    title_host_configure_runtime();
    CHECK(!strcmp(getenv("RECOMP_HEAP_RECLAIM"), "0"));
    puts("Passed: runtime defaults fill missing settings and preserve explicit overrides across repeated initialization.");
    return 0;
}
'''
(out / 'runtime_defaults.c').write_text(test.replace('CONFIGURE', configure))
vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                        '-property', 'installationPath'], text=True).strip()
if not installation:
    raise RuntimeError('Visual Studio C++ build tools were not found.')
environment = Path(installation) / 'VC/Auxiliary/Build/vcvars64.bat'
commands = ['@echo off', f'call "{environment}" >nul', 'if errorlevel 1 exit /b 1',
            'cl /nologo /std:c11 /O2 /D_CRT_SECURE_NO_WARNINGS runtime_defaults.c /Feruntime_defaults.exe',
            'if errorlevel 1 exit /b 1']
(out / 'compile.cmd').write_text('\n'.join(commands))
subprocess.run(['cmd', '/c', 'compile.cmd'], cwd=out, check=True)
subprocess.run([str(out / 'runtime_defaults.exe')], check=True)
