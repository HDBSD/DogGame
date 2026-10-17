namespace DogGame.Setup;

internal static class BootstrapChecks
{
    public static void Run()
    {
        static void Check(bool condition, string message)
        {
            if (!condition)
            {
                throw new InvalidOperationException(message);
            }
        }
        Check(BootstrapRunner.PathsOverlap(@"C:\games", @"C:\games\DogGame"), "Nested folders must overlap.");
        Check(BootstrapRunner.PathsOverlap(@"C:\games\", @"c:\GAMES"), "Path comparison must ignore case and trailing separators.");
        Check(!BootstrapRunner.PathsOverlap(@"C:\games", @"C:\games-backup"), "Sibling prefixes must remain separate.");
        Check(BootstrapRunner.PathsOverlap(@"C:\", @"C:\games"), "Drive roots must overlap their children.");
        Check(new SetupSettings().RemoveEnvironmentAfterBuild, "Environment cleanup must default to enabled.");
        string testDirectory = Path.Combine(AppContext.BaseDirectory, "bootstrap-self-test-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(testDirectory);
        string launcherDirectory = Path.Combine(testDirectory, "Setup folder with spaces");
        Directory.CreateDirectory(launcherDirectory);
        string setupExecutable = Path.Combine(launcherDirectory, "DogGameSetup.exe");
        File.WriteAllText(setupExecutable, "fixture setup");
        var gameInstallation = new SetupInstallation(setupExecutable);
        using var window = new SetupWindow(detectRequirements: false, installation: gameInstallation);
        Check(window.Text == "DogGame Setup Tool", "Window title changed.");
        Check(window.Controls.OfType<CheckBox>().Count() == 2, "Both setup choices must be present.");
        Check(window.Controls.OfType<Button>().Select(button => button.Text).SequenceEqual(["Build", "Update", "Remove environment", "Cancel", "Show output", "Play", "Open game folder"]), "Expected setup actions are missing.");
        Check(!window.Controls.OfType<Button>().Single(button => button.Text == "Cancel").Enabled, "Cancel must be disabled while idle.");
        Check(window.FormBorderStyle == FormBorderStyle.FixedDialog && !window.MaximizeBox, "Setup must remain a small fixed window.");
        Check(gameInstallation.DirectoryPath == Path.Combine(launcherDirectory, "DogGame-Contents"), "Game contents must be beside setup.");
        Check(!gameInstallation.HasOtherEntries(), "Setup alone must not trigger a folder warning.");
        Button playButton = window.Controls.OfType<Button>().Single(button => button.Text == "Play");
        Button folderButton = window.Controls.OfType<Button>().Single(button => button.Text == "Open game folder");
        Button updateButton = window.Controls.OfType<Button>().Single(button => button.Text == "Update");
        Check(!playButton.Enabled && !folderButton.Enabled && !updateButton.Enabled, "An absent installation must not be launchable.");
        Directory.CreateDirectory(gameInstallation.DirectoryPath);
        window.RefreshInstallation();
        Check(folderButton.Enabled && !playButton.Enabled, "An empty contents folder must not count as a game.");
        Check(!gameInstallation.HasOtherEntries(), "The expected contents folder must not trigger a warning.");
        File.WriteAllText(gameInstallation.GameExecutable, "fixture game - do not execute");
        window.RefreshInstallation();
        Check(playButton.Enabled && folderButton.Enabled && updateButton.Enabled, "Installed game actions were not enabled.");
        var launch = gameInstallation.PlayRequest();
        Check(launch.FileName == gameInstallation.GameExecutable && launch.WorkingDirectory == gameInstallation.DirectoryPath &&
              launch.UseShellExecute && launch.ArgumentList.Count == 0, "Play must use the installed game and its asset working directory.");
        Check(gameInstallation.OpenFolderRequest().FileName == gameInstallation.DirectoryPath, "Open folder selected the wrong directory.");
        string unrelated = Path.Combine(launcherDirectory, "unrelated.txt");
        File.WriteAllText(unrelated, "must be untouched");
        window.RefreshInstallation();
        Check(gameInstallation.HasOtherEntries() && window.Controls.OfType<Label>().Any(label => label.Text.StartsWith("This folder contains other files")),
              "Unrelated files must produce the startup warning.");
        File.Delete(unrelated);
        Directory.CreateDirectory(Path.Combine(launcherDirectory, "unrelated-folder"));
        Check(gameInstallation.HasOtherEntries(), "Unrelated directories must also trigger a warning.");
        File.Delete(gameInstallation.GameExecutable);
        window.RefreshInstallation();
        Check(!playButton.Enabled && !updateButton.Enabled, "Removing the installed executable must disable game actions.");
        Check(SetupInstallation.Current.DirectoryPath == Path.Combine(Path.GetDirectoryName(Environment.ProcessPath!)!, "DogGame-Contents"),
              "Single-file publishing must follow the executable, not a temporary extraction folder.");
        string release = Path.Combine(testDirectory, "release");
        string installation = Path.Combine(testDirectory, "installation");
        Directory.CreateDirectory(release);
        File.WriteAllText(Path.Combine(release, "DogGame.exe"), "new build");
        BootstrapRunner.Install(release, installation, CancellationToken.None);
        File.WriteAllText(Path.Combine(installation, "display.ini"), "user settings");
        File.WriteAllText(Path.Combine(release, "DogGame.exe"), "updated build");
        BootstrapRunner.Install(release, installation, CancellationToken.None);
        Check(File.ReadAllText(Path.Combine(installation, "DogGame.exe")) == "updated build", "Update was not installed.");
        Check(File.ReadAllText(Path.Combine(installation, "display.ini")) == "user settings", "Update discarded settings.");
        using var cancelled = new CancellationTokenSource();
        cancelled.Cancel();
        try
        {
            BootstrapRunner.Install(release, installation, cancelled.Token);
            throw new InvalidOperationException("Cancelled installation unexpectedly succeeded.");
        }
        catch (OperationCanceledException)
        {
            Check(File.ReadAllText(Path.Combine(installation, "DogGame.exe")) == "updated build", "Cancellation changed the installation.");
        }
        string environment = Path.Combine(testDirectory, "environment");
        Directory.CreateDirectory(environment);
        try
        {
            BootstrapRunner.RemoveEnvironmentAt(environment, environment, CancellationToken.None);
            throw new InvalidOperationException("Unmarked environment was removed.");
        }
        catch (InvalidOperationException)
        {
            Check(Directory.Exists(environment), "Unmarked directory must remain intact.");
        }
        File.WriteAllText(Path.Combine(environment, ".doggame-setup-environment"), "DogGame Setup Tool v1\n");
        string readOnlyObject = Path.Combine(environment, "git-object");
        File.WriteAllText(readOnlyObject, "cached tool data");
        File.SetAttributes(readOnlyObject, FileAttributes.ReadOnly);
        string outside = Path.Combine(testDirectory, "outside-cleanup-root");
        Directory.CreateDirectory(outside);
        string protectedFile = Path.Combine(outside, "keep.txt");
        File.WriteAllText(protectedFile, "must survive cleanup");
        File.SetAttributes(protectedFile, FileAttributes.ReadOnly);
        // Retrying after a failed cleanup must also discard a linked checkout.
        string workspace = Path.Combine(environment, "source");
        Directory.CreateDirectory(workspace);
        File.CreateSymbolicLink(Path.Combine(workspace, "dependency-link"), protectedFile);
        BootstrapRunner.PrepareWorkspace(environment, workspace, CancellationToken.None);
        Check(!Directory.Exists(workspace), "Retry did not remove the old checkout.");
        Check(File.ReadAllText(protectedFile) == "must survive cleanup", "Retry followed a dependency link.");
        File.CreateSymbolicLink(Path.Combine(environment, "AndroidManifest.xml"), protectedFile);
        File.CreateSymbolicLink(Path.Combine(environment, "dangling-file"), Path.Combine(outside, "missing.txt"));
        Directory.CreateSymbolicLink(Path.Combine(environment, "linked-directory"), outside);
        Directory.CreateSymbolicLink(Path.Combine(environment, "cycle"), environment);
        bool cleanupCancelled = false;
        try
        {
            BootstrapRunner.RemoveEnvironmentAt(environment, environment, cancelled.Token);
        }
        catch (OperationCanceledException)
        {
            cleanupCancelled = true;
        }
        Check(cleanupCancelled && File.Exists(Path.Combine(environment, ".doggame-setup-environment")),
              "Cancelled cleanup must retain its ownership marker for retry.");
        BootstrapRunner.RemoveEnvironmentAt(environment, environment, CancellationToken.None);
        Check(!Directory.Exists(environment) && Directory.Exists(installation), "Environment cleanup removed the installation.");
        Check(File.ReadAllText(protectedFile) == "must survive cleanup" &&
              (File.GetAttributes(protectedFile) & FileAttributes.ReadOnly) != 0,
              "Cleanup changed a link target or its attributes.");
        string rootLink = Path.Combine(testDirectory, "environment-root-link");
        Directory.CreateSymbolicLink(rootLink, outside);
        File.WriteAllText(Path.Combine(outside, ".doggame-setup-environment"), "DogGame Setup Tool v1\n");
        bool rootRejected = false;
        try
        {
            BootstrapRunner.RemoveEnvironmentAt(rootLink, rootLink, CancellationToken.None);
        }
        catch (InvalidOperationException)
        {
            rootRejected = true;
        }
        Check(rootRejected && File.Exists(protectedFile), "Linked environment roots must be rejected.");
        // The fixture stays beside this executable for inspection; it never uses real game files.
    }
}
