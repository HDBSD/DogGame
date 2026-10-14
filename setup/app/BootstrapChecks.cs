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
        using var window = new SetupWindow(detectRequirements: false);
        Check(window.Text == "DogGame Setup Tool", "Window title changed.");
        Check(window.Controls.OfType<CheckBox>().Count() == 2, "Both setup choices must be present.");
        Check(window.Controls.OfType<Button>().Select(button => button.Text).SequenceEqual(["Build", "Update", "Remove environment", "Cancel", "Show output"]), "Expected setup actions are missing.");
        Check(!window.Controls.OfType<Button>().Single(button => button.Text == "Cancel").Enabled, "Cancel must be disabled while idle.");
        Check(window.FormBorderStyle == FormBorderStyle.FixedDialog && !window.MaximizeBox, "Setup must remain a small fixed window.");
        string testDirectory = Path.Combine(AppContext.BaseDirectory, "bootstrap-self-test-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(testDirectory);
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
        BootstrapRunner.RemoveEnvironmentAt(environment, environment, CancellationToken.None);
        Check(!Directory.Exists(environment) && Directory.Exists(installation), "Environment cleanup removed the installation.");
        // The fixture stays beside this executable for inspection; it never uses real game files.
    }
}
