"""Create the shared native-test fixture locally; no binaries are distributed."""
import os
from pathlib import Path
import subprocess

out = Path(__file__).resolve().parent / "irq-test-build"
out.mkdir(exist_ok=True)
vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
installation = subprocess.check_output([
    str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"
], text=True).strip()
msbuild = str(Path(installation) / "MSBuild/Current/Bin/MSBuild.exe")
version = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationVersion"], text=True).strip()
toolset = "v145" if int(version.split(".")[0]) >= 18 else "v143"
(out / "apu_irq_harness.vcxproj").write_text('''<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
<ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Release|x64"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration></ItemGroup>
<PropertyGroup Label="Globals"><ProjectGuid>{13D09601-9B48-4E15-9000-110011001100}</ProjectGuid></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props"/>
<PropertyGroup Label="Configuration"><ConfigurationType>Application</ConfigurationType><PlatformToolset>TOOLSET</PlatformToolset></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props"/>
<PropertyGroup><OutDir>$(ProjectDir)bin\\</OutDir><IntDir>$(ProjectDir)obj\\apu_irq\\</IntDir></PropertyGroup>
<ItemDefinitionGroup><ClCompile><RuntimeLibrary>MultiThreaded</RuntimeLibrary></ClCompile></ItemDefinitionGroup>
<ItemGroup><ClCompile Include="apu_irq_harness.c"/></ItemGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets"/>
</Project>'''.replace("TOOLSET", toolset))
