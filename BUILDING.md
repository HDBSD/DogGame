# Build DogGame locally

Install Git, Python 3.10+, CMake 3.20+ and Visual Studio's **Desktop development with C++** workload. Supply your own extracted Xbox game folder containing `default.xbe`. The current configuration accepts the two XBE hashes checked by Setup.ps1; other game revisions require porting and validation.

From PowerShell in SD100F:

DogGame is the product and executable name; the existing checkout directory can remain named `SD100F`. `Resources/DogGame.ico` is embedded in the executable and used by the game window. Existing saves continue to use the established `sd100f` storage directories.

```powershell
.\Setup.ps1 -GameDirectory 'D:\Games\Scooby'
.\Run-Menu.ps1
```

Setup downloads the pinned upstream revision, creates a project-local Python environment, copies your game into `build/Release/game`, generates C locally, and builds `build/Release/DogGame.exe`. It never launches or terminates the game. Close the game before rebuilding. `-PreviewBuildLocking` builds the authorized preview gate; ordinary setup builds an unrestricted release. Maintainers can use `-XboxRecompDirectory ..\xboxrecomp` and `-SkipGeneration` for an existing generated tree.

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
