"""Check the production command-line gate and embedded Vulkan shader freshness."""
from pathlib import Path
import hashlib
import subprocess
from test_support import msbuild

root = Path(__file__).resolve().parents[1]
out = root / "tests/irq-test-build"
options = (root / "src/debug_options.c").read_text()
parse = options[options.index("int title_options_parse("):options.index("int title_options_init(")]
fixture = '#include <stdio.h>\n#include <string.h>\n#include <wchar.h>\n'
fixture += f'#include "{(root / "src/debug_options.h").as_posix()}"\nTitleOptions title_options;\n'
fixture += parse + r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    char error[512];
    const wchar_t *ordinary[] = {L"DogGame.exe"};
    const wchar_t *alone[] = {L"DogGame.exe", L"--vulkan"};
    const wchar_t *debug[] = {L"DogGame.exe", L"--debug"};
    const wchar_t *selected[] = {L"DogGame.exe", L"--debug", L"--vulkan"};
    const wchar_t *reversed[] = {L"DogGame.exe", L"--vulkan", L"--debug"};
    const wchar_t *invalid[] = {L"DogGame.exe", L"--debug", L"--vulkan", L"--unknown"};
    CHECK(title_options_parse(1, ordinary, error, sizeof error));
    CHECK(!title_options.vulkan && !title_options.debug);
    CHECK(!title_options_parse(2, alone, error, sizeof error));
    CHECK(!title_options.vulkan && !title_options.debug);
    CHECK(title_options_parse(2, debug, error, sizeof error)); CHECK(!title_options.vulkan);
    CHECK(title_options_parse(3, selected, error, sizeof error));
    CHECK(title_options.vulkan && title_options.debug);
    CHECK(title_options_parse(3, reversed, error, sizeof error)); CHECK(title_options.vulkan);
    CHECK(!title_options_parse(4, invalid, error, sizeof error));
    CHECK(title_options.vulkan && title_options.debug); // failed parse publishes no partial options
    CHECK(title_options_parse(1, ordinary, error, sizeof error)); CHECK(!title_options.vulkan);
    puts("Passed: Vulkan requires explicit --debug --vulkan, either order; default/debug-only unchanged; invalid arguments publish no partial options.");
}
'''
(out / "renderer_options.c").write_text(fixture)
project = (out / "apu_irq_harness.vcxproj").read_text()
project = project.replace("apu_irq_harness.c", "renderer_options.c").replace("obj\\apu_irq\\", "obj\\renderer_options\\")
(out / "renderer_options.vcxproj").write_text(project)
subprocess.run([msbuild, str(out / "renderer_options.vcxproj"), "/p:Configuration=Release", "/p:Platform=x64", "/verbosity:minimal"], check=True)
subprocess.run([str(out / "bin/renderer_options.exe")], check=True)
backend = root / "runtime_overrides/src/kernel"
shader = (backend / "nv2a_gpu.cpp").read_text().split('static const char shader[]=R"(', 1)[1].split(')";', 1)[0]
shader = shader.replace("(1-v.p.y*2/viewport.y)*w", "(v.p.y*2/viewport.y-1)*w")
digest = hashlib.sha256(shader.encode()).hexdigest()
assert digest in (backend / "nv2a_vulkan_shaders.h").read_text(), "Regenerate embedded Vulkan shaders"
print("Passed: embedded SPIR-V source digest matches the current shared shader.")
