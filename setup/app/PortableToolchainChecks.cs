namespace DogGame.Setup;

internal static class PortableToolchainChecks
{
    internal static async Task RunAsync()
    {
        string root = Path.Combine(AppContext.BaseDirectory, "portable-self-test");
        Directory.CreateDirectory(root);
        string marker = Path.Combine(root, ".doggame-setup-environment");
        if (Directory.EnumerateFileSystemEntries(root).Any() && !File.Exists(marker))
        {
            throw new InvalidOperationException("The portable test directory is not owned by setup.");
        }
        File.WriteAllText(marker, "DogGame Setup Tool v1\n");
        string tools = Path.Combine(root, "tools");
        string log = Path.Combine(root, "requirements.log");
        var lines = new System.Collections.Concurrent.ConcurrentQueue<string>();
        await PortableToolchain.ProvisionAsync(tools, lines.Enqueue, CancellationToken.None).ConfigureAwait(false);
        string? previousPath = Environment.GetEnvironmentVariable("PATH");
        try
        {
            // Exclude installed Python, Git, Visual Studio and CMake for this test.
            Environment.SetEnvironmentVariable("PATH", Environment.GetFolderPath(Environment.SpecialFolder.System));
            RequirementResult[] checks = await RequirementChecker.CheckAsync(Path.Combine(root, "requirements"),
                lines.Enqueue, CancellationToken.None, tools).ConfigureAwait(false);
            File.WriteAllLines(log, lines);
            if (checks.Length == 0 || checks.Any(check => !check.Passed))
            {
                throw new InvalidOperationException("Portable prerequisites failed. See " + log);
            }
            string missingTools = Path.Combine(root, "missing-tools");
            Directory.CreateDirectory(missingTools);
            RequirementResult[] missing = await RequirementChecker.CheckAsync(Path.Combine(root, "missing-requirements"),
                _ => { }, CancellationToken.None, missingTools).ConfigureAwait(false);
            if (!missing.Any(check => !check.Passed) || missing.Any(check => check.Passed && check.Name != "Windows x64"))
            {
                throw new InvalidOperationException("Missing portable tools must not fall back to installed tools.");
            }
            string repository = Path.Combine(root, "git-fixture");
            Directory.CreateDirectory(repository);
            var start = new System.Diagnostics.ProcessStartInfo
            {
                FileName = Path.Combine(tools, "git", "cmd", "git.exe"), WorkingDirectory = repository,
                UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true
            };
            PortableToolchain.Configure(start, tools);
            start.ArgumentList.Add("init");
            await BuildProcess.RunAsync(start, Path.Combine(root, "git.log"), lines.Enqueue, CancellationToken.None).ConfigureAwait(false);
        }
        finally
        {
            Environment.SetEnvironmentVariable("PATH", previousPath);
        }
        // Leave the verified tools here so development can test a complete game build.
    }
}
