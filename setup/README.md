# DogGame Setup Tool

DogGameSetup.exe is a self-contained Windows x64 application. It embeds Python with Capstone, portable Git, CMake, Ninja and LLVM-MinGW. Users do not install Python, Visual Studio, MSBuild, CMake, Git or .NET. Internet access is still required to download the game source and pinned xboxrecomp source; private GitHub repositories require credentials with access.

## Build and update

Clean game generation must retain the original routines called by maintained
FMV observers. They use the recompiler's `extern sub_ADDRESS_gen(void)` wrapper
convention; arbitrary delegate names make the manual scanner omit their bodies.
`tests/Check-GeneratedDelegates.py` verifies this against the pinned scanner.
CMake also supports older generated trees by locating the original definitions
by address instead of assuming fixed chunk filenames.

If an editor or terminal holds `DogGame-Contents` open against directory
renames, installation keeps that directory and replaces staged files with
per-file backups. A locked destination file aborts this fallback and restores
earlier replacements. Existing settings and unrelated files remain in place.
The self-test reproduces both the directory lock and file-lock rollback.
Running DogGame variants are checked again immediately before installation.

- **Build** selects the extracted Xbox game folder containing the supported `default.xbe`. The built game is installed into `DogGame-Contents` beside `DogGameSetup.exe`. There is no installation-folder picker.
- **Update** downloads fresh source using the saved Xbox game folder and updates the adjacent `DogGame-Contents` installation.
- **Play** launches `DogGame-Contents/DogGame.exe` with that folder as its working directory, so game assets resolve correctly. It is enabled when the executable exists, independently of build-tool availability.
- **Open game folder** opens the contents folder in Windows Explorer when that folder exists.
- **Enable beta build** selects `beta`; otherwise setup clones `master` from `https://github.com/HDBSD/DogGame.git`. Missing branches fail without fallback. This choice does not enable preview authentication locking.
- **Remove build environment after build** is enabled by default. Following successful installation, setup removes its source checkout, tools, generated code and build files. Explicitly disabling this retains the environment; the next build replaces the source checkout.
- **Remove environment** deletes only the marked environment, preserving the user's game, installation, settings and last build log. Failed/cancelled builds retain files for diagnosis.

Work runs under `%LOCALAPPDATA%\DogGameSetup\environment`. Setup verifies the embedded tool archive's SHA-256, extracts it, checks Python/Capstone and Git, and compiles/links a C/C++/resource fixture using its own CMake, Ninja and compiler. It then generates and builds the game on the user's device. Embedded Python uses bundled packages rather than pip or a virtual environment. Tool locations are explicitly passed to Setup.ps1; system-wide PATH is not modified.

The installation is staged beside the destination and the prior installation is restored if publishing fails. Existing `.ini`/`.json` settings are retained. Source game, installation and environment cannot overlap. Setup starts the game only when Play is clicked and never stops a running game. The resulting executable links the compiler's C++ runtime statically so removing the tools does not remove runtime DLLs it needs. Windows graphics, audio and Media Foundation components remain OS dependencies.

## Output and cancellation

Live stdout/stderr appears in the panel to the right, which can be shown/hidden. Streams drain concurrently on background workers; bounded queues and UI batches keep the window responsive. The complete clone/build log remains at `%LOCALAPPDATA%\DogGameSetup\last-build.log`. Cancel or closing the window stops active child processes. Installation is a short non-cancellable transaction; requested cleanup completes after commit.

Portable Git uses configured credentials, disables hidden interactive prompts, and reports authentication errors in the output panel. While the source is private, configure a Git credential helper or existing credentials before building. Setup does not collect or store GitHub tokens.

## Publishing

The [setup release workflow](../.github/workflows/setup-release.yml) publishes a
GitHub release whenever `setup/`, the setup icon, or the workflow changes on
`master`. It also supports a manual run from the Actions page on `master`.
The Windows runner builds the self-contained setup, verifies its embedded
toolchain and runs the setup self-tests before publishing
`DogGameSetup-win-x64.zip` and its SHA-256 checksum. Only the setup executable
is packaged; game source, game executables and assets are excluded. Each run
uses a unique `setup-<run number>-<attempt>` tag targeting the tested commit.
Tool downloads are cached, and their pinned checksums are still verified on
every build. Publishing uses the repository's Actions token with release write
permission; no personal access token is needed.

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

The game build also downloads a hash-pinned SDL controller source release and links it statically for Xbox, PS4 and PS5 input. Users do not install SDL or a controller remapper. The output includes `SDL-LICENSE.txt`; see [controller support](../CONTROLLERS.md) for mappings and validation limits.

Dependency archives can contain source symlinks for other platforms (including SDL's Android manifests). Environment cleanup removes link entries without following their targets. The marked environment root and checkout root must be real directories; copying links into the installed game remains rejected. Cleanup retains its ownership marker until the final removal so interruption can be retried. The self-test includes file/directory links, dangling links, a cycle and unchanged read-only targets; creating these test links requires Windows Developer Mode or symbolic-link privileges.

Keep `DogGameSetup.exe` in its own folder. Setup displays a non-blocking warning when its directory contains anything other than that executable and `DogGame-Contents`; unrelated files are not modified. The installation location is shown in the window. Moving setup together with its contents folder preserves the launcher layout. Old saved custom installation paths are ignored; existing installations in other locations are not moved automatically. Building/updating replaces only the adjacent installation under the existing managed-folder checks.
