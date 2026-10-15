# DogGame Setup Tool

DogGameSetup.exe is a self-contained Windows x64 application. It embeds Python with Capstone, portable Git, CMake, Ninja and LLVM-MinGW. Users do not install Python, Visual Studio, MSBuild, CMake, Git or .NET. Internet access is still required to download the game source and pinned xboxrecomp source; private GitHub repositories require credentials with access.

## Build and update

- **Build** selects the extracted Xbox game folder containing the supported `default.xbe`, and a dedicated installation directory.
- **Update** downloads fresh source using saved game and installation folders.
- **Enable beta build** selects `beta`; otherwise setup clones `master` from `https://github.com/HDBSD/DogGame.git`. Missing branches fail without fallback. This choice does not enable preview authentication locking.
- **Remove build environment after build** is enabled by default. Following successful installation, setup removes its source checkout, tools, generated code and build files. Explicitly disabling this retains the environment; the next build replaces the source checkout.
- **Remove environment** deletes only the marked environment, preserving the user's game, installation, settings and last build log. Failed/cancelled builds retain files for diagnosis.

Work runs under `%LOCALAPPDATA%\DogGameSetup\environment`. Setup verifies the embedded tool archive's SHA-256, extracts it, checks Python/Capstone and Git, and compiles/links a C/C++/resource fixture using its own CMake, Ninja and compiler. It then generates and builds the game on the user's device. Embedded Python uses bundled packages rather than pip or a virtual environment. Tool locations are explicitly passed to Setup.ps1; system-wide PATH is not modified.

The installation is staged beside the destination and the prior installation is restored if publishing fails. Existing `.ini`/`.json` settings are retained. Source game, installation and environment cannot overlap. Setup never starts or stops a game. The resulting executable links the compiler's C++ runtime statically so removing the tools does not remove runtime DLLs it needs. Windows graphics, audio and Media Foundation components remain OS dependencies.

## Output and cancellation

Live stdout/stderr appears in the panel to the right, which can be shown/hidden. Streams drain concurrently on background workers; bounded queues and UI batches keep the window responsive. The complete clone/build log remains at `%LOCALAPPDATA%\DogGameSetup\last-build.log`. Cancel or closing the window stops active child processes. Installation is a short non-cancellable transaction; requested cleanup completes after commit.

Portable Git uses configured credentials, disables hidden interactive prompts, and reports authentication errors in the output panel. While the source is private, configure a Git credential helper or existing credentials before building. Setup does not collect or store GitHub tokens.

## Publishing

Run `setup/Publish.ps1` with the .NET 10 SDK on the publishing machine. Prepare-Toolchain.ps1 downloads exact packages from `toolchain.lock.json`, rejects checksum mismatches, retains their packaged license files and adds Ninja's license, and creates a compressed tools archive. The archive and checksum are embedded in `build/setup/publish/DogGameSetup.exe`; game source/assets/generated code are excluded. Downloads are cached under `build/portable/downloads`.

Python's archive hash is pinned from the official HTTPS download; GitHub release asset hashes and the Capstone PyPI digest were also checked when selecting the packages. Update versions/hashes deliberately and repeat build validation. Full distributions are bundled initially, so setup is considerably larger than its UI alone.

For a developer build with the unpacked toolchain:

```powershell
.\Setup.ps1 -ToolchainDirectory 'path\to\tools' -GameDirectory 'path\to\game'
```

Use a fresh build directory when changing compilers (`-BuildDirectory 'path\to\build'`). ToolchainDirectory is required. Missing bundled tools fail the build; setup never searches for installed replacements. Child-process PATH contains only the portable tools and Windows OS directories.

## Verification

`DogGameSetup.exe --self-test` checks the window, local master/beta checkout and missing-branch failures, live output, process-tree cancellation, staged installation, preserved settings and environment cleanup. It does not access GitHub or real game files.

`DogGameSetup.exe --toolchain-test` extracts the actual embedded tools beside the test executable and checks them with installed development tools excluded from PATH. It retains the extracted test tools for a full development build. Results are written beside the executable.

Compiler success does not establish gameplay correctness. The LLVM-MinGW executable still needs gameplay testing, including rendering, audio, input, saves and guest thread/register behavior.
