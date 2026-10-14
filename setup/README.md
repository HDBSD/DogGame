# DogGame Setup Tool

The bootstrap has project information, the two setup choices, a progress bar, and Build / Update / Remove environment / Cancel buttons. Build output expands to the right when a build starts and can be shown or hidden independently.

- **Build** selects the user's extracted game folder and a dedicated installation folder, verifies the XBE, and builds on the user's machine.
- **Update** downloads the latest selected branch using the saved game and installation folders, then rebuilds locally.
- **Enable beta build** selects the `beta` branch; normal builds use `master`. Beta selection does not enable preview authentication locking. A missing beta branch produces an error; setup does not fall back to master.
- **Remove environment** deletes only the marked build environment. It retains the installed game, source game, settings and last build log.
- **Remove build environment after build** performs that cleanup after a successful installation.
- Cleanup is enabled by default. Older saved defaults are migrated once; subsequent explicit checkbox choices are retained.

Setup checks requirements when the window opens and again before building. It verifies x64 Windows, Python 3.10+ with virtual-environment/pip support, Git, the Visual Studio C++ toolset, and CMake 3.20+ (including Visual Studio's bundled CMake). A small C/C++ executable and resource are compiled and linked to check the actual compiler, Windows SDK and required import libraries. Missing requirements appear in the output panel and disable Build/Update until setup is restarted after installing them. Dependency downloads still require an Internet connection; this check does not download packages.

Work runs under `%LOCALAPPDATA%\DogGameSetup\environment`. Installation is staged beside the destination; the old installation is restored if publishing the new build fails. Existing `.ini` and `.json` settings are preserved. The game folder and build environment cannot overlap the installation. Setup does not launch or terminate a running game.

The progress bar shows completed preparation/installation stages and animates while the current build pipeline runs; it does not invent a compilation percentage. Cancel stops the active build and its child processes. Closing the window requests the same cancellation before exiting. Publishing a completed installation is a brief, non-cancellable transaction so an existing installation can be restored on failure.

Stdout and stderr are drained concurrently on background workers, written to disk, and queued for the live view. The UI appends bounded batches on a timer rather than updating a control for every build line. The live view limits its history and reports omitted lines; the full log remains at `%LOCALAPPDATA%\DogGameSetup\last-build.log`.

The window explains the project's work-in-progress status, xboxrecomp dependency, declared AI use, 1.0.0 goal, and approach to original glitches and unintended behavior.

## Current backend

This is the initial bootstrap, not the finished prerequisite-free installer. The setup UI can be published with its own .NET runtime, but game compilation currently uses the existing Python, Git, CMake and Visual Studio C++ environment through `Setup.ps1`. CMake may come from Visual Studio as before. Missing prerequisites cause a build error rather than pretending portable tools were provisioned.

`toolchain.lock.json` records this development backend. The next step is to implement verified portable downloads after selecting and validating a compatible toolchain. No unverified download URLs or hashes are used. MSVC remains the game-build backend until the planned MinGW experiment is performed.

## Development and packaging

Run `dotnet run --project setup/app/DogGame.Setup.csproj` from this checkout. Development builds locate the repository beside the executable and copy maintained sources into their private workspace; they do not regenerate this checkout's `src/recomp/`.

Run `setup/Publish.ps1` to create `build/setup/publish/DogGameSetup.exe`. The executable does not bundle game source. Build and Update shallow-clone `https://github.com/HDBSD/DogGame.git` into the owned environment, build the selected branch, install its output, and remove the checkout and build environment after success by default. Unchecking cleanup retains the environment; the next build replaces its source checkout. Failed or cancelled builds retain files for diagnosis until Remove environment is used. The game executable is generated and compiled locally from the user's supplied XBE.

While the repository is private, Git for Windows must already have credentials with access to it. Setup uses configured Git credentials and disables interactive credential prompts so the hidden process cannot hang waiting for input. Authentication errors appear in the output panel. Once the repository is public, ordinary clones require no sign-in.

Run the built `DogGameSetup.exe --self-test` to check path separation and the window controls without starting a game build. These tests do not verify a clean-machine toolchain or gameplay.

The self-test also exercises staged installs, settings preservation, cancellation, and removal of a marked environment containing read-only cache files. Fixtures and results are written beside the test executable, without accessing real game files.

The process test starts a harmless parent/child fixture, verifies live delivery of both stdout and stderr before exit, cancels it, and checks that neither process remains running and both streams were retained in the log.

The self-test also creates a local Git repository to verify master/beta selection and missing-branch errors, without accessing GitHub or real game files.
