# Build DogGame locally

Use the portable tools extracted by DogGameSetup.exe, or publish setup with the .NET 10 SDK and run its `--toolchain-test` to extract them. Supply your own extracted Xbox game folder containing `default.xbe`. The current configuration accepts the two XBE hashes checked by Setup.ps1; other game revisions require porting and validation.

From PowerShell in SD100F:

DogGame is the product and executable name; the existing checkout directory can remain named `SD100F`. `Resources/DogGame.ico` is embedded in the executable and used by the game window. Existing saves continue to use the established `sd100f` storage directories.

```powershell
.\Setup.ps1 -GameDirectory 'D:\Games\Scooby' -ToolchainDirectory 'build\setup\portable-publish\portable-self-test\tools'
.\Run-Menu.ps1
```

Setup downloads the pinned upstream revision, uses bundled Python/Capstone, copies your game into `build/Release/game`, generates C locally, and builds `build/Release/DogGame.exe`. It never launches or terminates the game. Close the game before rebuilding. `-PreviewBuildLocking` builds the authorized preview gate; ordinary setup builds an unrestricted release. Maintainers can use `-XboxRecompDirectory ..\xboxrecomp` and `-SkipGeneration` for an existing generated tree.

## Runtime layout

- `config/xboxrecomp.lock.json`: upstream revision and normalized source hashes.
- `runtime_overrides/`: DogGame's customized runtime, tools, support headers and tests, retaining the upstream directory layout and license notices.
- `scripts/prepare_runtime.py`: verifies upstream and assembles it with overrides into `build/runtime/xboxrecomp`. No files in the upstream checkout are changed.
- `config/seed_functions.json`: recovered function entry points required for regeneration.
- `scripts/generate_game.py`: regenerates from the XBE, scanning all persistent title sources for overrides.

CMake prepares the runtime automatically. Edit customized files in `runtime_overrides`, not the generated build copy. Changing an override triggers CMake to refresh the private copy. Upstream updates require reviewing the overrides and updating the lock; mismatched upstream sources are rejected rather than silently combined.

Keep game assets, generated C, build output, dependency checkouts, saves and diagnostics out of distribution. The existing historical BOOT_NOTES commands refer to the former layout; use Setup.ps1 for the current pipeline.

## Restructure verification

On 14 October 2026, Setup.ps1 completed the full analysis, identification, ABI analysis, generation and Release build against a pristine pinned upstream checkout. All 441 upstream files matched their pinned hashes afterward. Hardware rendering checks (including fallback variants), native display/menu checks and 10,000 vertex-input comparisons passed from `tests/`. These checks do not replace a gameplay smoke test of the freshly generated executable.

## Portable setup builds

The published setup embeds LLVM-MinGW, Ninja, CMake, Git and Python/Capstone; users do not install development tools. See [setup/README.md](setup/README.md). For a local portable build, pass `-ToolchainDirectory` to Setup.ps1 and use a fresh `-BuildDirectory` when switching from MSVC. The portable compiler links its C++ runtime statically. Full gameplay validation of the compiler change remains required.

On 14 October 2026, the embedded tools passed their compile/link/resource checks with installed development tools excluded from PATH. A separate source checkout then downloaded pinned xboxrecomp, generated the title from the supported XBE, and completed a Release build with embedded Python/Capstone, LLVM-MinGW and Ninja. The built executable imports Windows components and no LLVM C++ runtime DLL. Setup's branch-selection, cancellation, staged-installation and cleanup self-tests also passed. This is build validation; the new executable has not been gameplay-tested.
