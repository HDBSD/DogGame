using System.Diagnostics;
using System.Security.Cryptography;

namespace DogGame.Setup;

internal sealed record BuildStatus(int Percent, string Message, bool Indeterminate = false);

internal sealed class BootstrapRunner
{
    private const string EnvironmentMarker = ".doggame-setup-environment";
    private const string InstallationMarker = ".doggame-setup-installation";
    private const string MarkerContents = "DogGame Setup Tool v1\n";
    internal const string SourceRepository = "https://github.com/HDBSD/DogGame.git";

    internal static string SourceBranch(bool beta) => beta ? "beta" : "master";
    private static readonly HashSet<string> SupportedHashes = new(StringComparer.OrdinalIgnoreCase)
    {
        "E593C49BB9B403ACC7CC4416B83D6130322DF1B70B89E17038FD437044F37459",
        "E6947834C4F9AE096B720D8B123EB8F27E69077E356EDEF327DCA30D43674814"
    };

    public async Task BuildAsync(SetupSettings settings, IProgress<BuildStatus> progress,
                                 Action<string> output, CancellationToken token)
    {
        string game = Path.GetFullPath(settings.GameDirectory ?? throw new InvalidOperationException("Select a game folder."));
        string installation = Path.GetFullPath(settings.InstallDirectory ?? throw new InvalidOperationException("Select an installation folder."));
        string environment = Path.GetFullPath(SetupSettings.EnvironmentDirectory);
        ValidateInstallation(game, installation, environment);
        progress.Report(new(0, "Checking game files…"));
        await using (var xbe = File.OpenRead(Path.Combine(game, "default.xbe")))
        {
            string hash = Convert.ToHexString(await SHA256.HashDataAsync(xbe, token));
            if (!SupportedHashes.Contains(hash))
            {
                throw new InvalidOperationException("This version of default.xbe is not supported.");
            }
        }

        progress.Report(new(5, "Preparing build environment…"));
        string workspace = Path.Combine(environment, "source");
        await Task.Run(() => PrepareWorkspace(environment, workspace, token), token);
        string tools = Path.Combine(environment, "tools");
        progress.Report(new(5, "Extracting portable build tools…", Indeterminate: true));
        await PortableToolchain.ProvisionAsync(tools, output, token);
        progress.Report(new(6, "Checking build requirements…", Indeterminate: true));
        RequirementResult[] requirements = await RequirementChecker.CheckAsync(Path.Combine(environment, "requirements"), output, token, tools);
        RequirementResult[] missing = requirements.Where(requirement => !requirement.Passed).ToArray();
        if (requirements.Length == 0 || missing.Length > 0)
        {
            throw new InvalidOperationException("Build requirements are not met:\n" +
                string.Join("\n", missing.Select(requirement => $"{requirement.Name}: {requirement.Detail}")));
        }
        string logPath = Path.Combine(SetupSettings.Home, "last-build.log");
        string branch = SourceBranch(settings.EnableBetaBuild);
        progress.Report(new(8, $"Downloading DogGame source ({branch})…", Indeterminate: true));
        await CloneSourceAsync(SourceRepository, branch, workspace, logPath, output, token, tools);
        progress.Report(new(10, "Building DogGame…", Indeterminate: true));
        await RunBuildAsync(workspace, game, logPath, output, token, tools);

        string release = Path.Combine(workspace, "build", "Release");
        if (!File.Exists(Path.Combine(release, "DogGame.exe")))
        {
            throw new InvalidOperationException($"The build finished without DogGame.exe. See {logPath}");
        }
        token.ThrowIfCancellationRequested();
        progress.Report(new(90, "Installing completed build…"));
        await Task.Run(() => Install(release, installation, token), token);
        if (settings.RemoveEnvironmentAfterBuild)
        {
            progress.Report(new(95, "Removing build environment…"));
            // Installation has committed. Finish requested cleanup even if the window closes now.
            await Task.Run(() => RemoveEnvironment(CancellationToken.None));
        }
        progress.Report(new(100, "Build complete. DogGame.exe is ready."));
    }

    internal static bool PathsOverlap(string first, string second)
    {
        string left = Path.TrimEndingDirectorySeparator(Path.GetFullPath(first));
        string right = Path.TrimEndingDirectorySeparator(Path.GetFullPath(second));
        return left.Equals(right, StringComparison.OrdinalIgnoreCase) ||
               left.StartsWith(Path.EndsInDirectorySeparator(right) ? right : right + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase) ||
               right.StartsWith(Path.EndsInDirectorySeparator(left) ? left : left + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
    }

    private static void ValidateInstallation(string game, string installation, string environment)
    {
        if (!File.Exists(Path.Combine(game, "default.xbe")))
        {
            throw new InvalidOperationException("The selected game folder does not contain default.xbe.");
        }
        if (PathsOverlap(game, installation) || PathsOverlap(game, environment) || PathsOverlap(installation, environment))
        {
            throw new InvalidOperationException("The source game, installation and build environment must be separate folders.");
        }
        if (Directory.Exists(installation))
        {
            RejectLinks(installation);
            if (Directory.EnumerateFileSystemEntries(installation).Any())
            {
                RequireMarker(installation, InstallationMarker);
            }
        }
        foreach (var process in Process.GetProcessesByName("DogGame"))
        {
            using (process)
            {
                if (process.MainModule?.FileName is string executable && PathsOverlap(executable, installation))
                {
                    throw new InvalidOperationException("Close DogGame before updating its installation.");
                }
            }
        }
    }

    private static void PrepareWorkspace(string environment, string workspace, CancellationToken token)
    {
        if (Directory.Exists(environment))
        {
            RequireMarker(environment, EnvironmentMarker);
            RejectLinks(environment);
        }
        else
        {
            Directory.CreateDirectory(environment);
            File.WriteAllText(Path.Combine(environment, EnvironmentMarker), MarkerContents);
        }
        token.ThrowIfCancellationRequested();
        // Always start from a fresh checkout, including when switching branches.
        // The marker and link checks above establish ownership of this tree.
        string expected = Path.GetFullPath(Path.Combine(environment, "source"));
        if (!Path.GetFullPath(workspace).Equals(expected, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException("The source checkout path is invalid.");
        }
        if (Directory.Exists(workspace))
        {
            ClearReadOnlyFiles(workspace);
            Directory.Delete(workspace, recursive: true);
        }
    }

    internal static async Task CloneSourceAsync(string repository, string branch, string workspace,
                                                string logPath, Action<string> output, CancellationToken token, string tools)
    {
        var start = new ProcessStartInfo
        {
            FileName = "git", WorkingDirectory = Path.GetDirectoryName(workspace)!,
            UseShellExecute = false, CreateNoWindow = true,
            RedirectStandardOutput = true, RedirectStandardError = true
        };
        // Use credentials already configured for Git. Hidden console prompts would
        // otherwise leave a private-repository download waiting indefinitely.
        start.Environment["GIT_TERMINAL_PROMPT"] = "0";
        start.Environment["GCM_INTERACTIVE"] = "never";
        start.FileName = Path.Combine(tools, "git", "cmd", "git.exe");
        PortableToolchain.Configure(start, tools);
        foreach (string argument in new[] { "clone", "--depth", "1", "--single-branch", "--branch", branch,
                                           "--progress", "--", repository, workspace })
        {
            start.ArgumentList.Add(argument);
        }
        output($"Cloning {repository}, branch {branch}.");
        try
        {
            await BuildProcess.RunAsync(start, logPath, output, token);
        }
        catch (InvalidOperationException error)
        {
            throw new InvalidOperationException($"Could not download the '{branch}' branch. Check that it exists and that Git has access to the repository. " +
                "For a private repository, sign in with Git for Windows before running setup. See the output for Git's error.", error);
        }
        foreach (string required in new[] { "Setup.ps1", "CMakeLists.txt", "config/xboxrecomp.lock.json" })
        {
            if (!File.Exists(Path.Combine(workspace, required)))
            {
                throw new InvalidOperationException($"The selected branch is missing {required}.");
            }
        }
    }

    private static async Task RunBuildAsync(string workspace, string game, string logPath,
                                            Action<string> output, CancellationToken token, string tools)
    {
        var start = new ProcessStartInfo
        {
            FileName = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell", "v1.0", "powershell.exe"),
            WorkingDirectory = workspace,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        start.Environment["PIP_CACHE_DIR"] = Path.Combine(workspace, "build", "pip-cache");
        PortableToolchain.Configure(start, tools);
        foreach (string argument in new[] { "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File",
                                           Path.Combine(workspace, "Setup.ps1"), "-GameDirectory", game })
        {
            start.ArgumentList.Add(argument);
        }
        start.ArgumentList.Add("-ToolchainDirectory");
        start.ArgumentList.Add(tools);
        await BuildProcess.RunAsync(start, logPath, output, token, appendLog: true);
    }

    internal static void Install(string release, string installation, CancellationToken token)
    {
        string parent = Path.GetDirectoryName(installation) ?? throw new InvalidOperationException("Choose an installation subfolder.");
        Directory.CreateDirectory(parent);
        string staging = Path.Combine(parent, ".doggame-install-" + Guid.NewGuid().ToString("N"));
        string backup = Path.Combine(parent, ".doggame-backup-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(staging);
        File.WriteAllText(Path.Combine(staging, InstallationMarker), MarkerContents);
        bool committed = false;
        try
        {
            CopyTree(release, staging, token);
            if (Directory.Exists(installation))
            {
                foreach (string configuration in Directory.EnumerateFiles(installation))
                {
                    if (Path.GetExtension(configuration) is ".ini" or ".json")
                    {
                        File.Copy(configuration, Path.Combine(staging, Path.GetFileName(configuration)), overwrite: true);
                    }
                }
            }
            token.ThrowIfCancellationRequested();
            // Commit has no cancellation points: either publish the new build or restore the old directory.
            if (Directory.Exists(installation))
            {
                Directory.Move(installation, backup);
            }
            try
            {
                Directory.Move(staging, installation);
                committed = true;
            }
            catch
            {
                if (Directory.Exists(backup))
                {
                    Directory.Move(backup, installation);
                }
                throw;
            }
        }
        finally
        {
            if (Directory.Exists(staging))
            {
                RejectLinks(staging);
                ClearReadOnlyFiles(staging);
                Directory.Delete(staging, recursive: true);
            }
            if (committed && Directory.Exists(backup))
            {
                RejectLinks(backup);
                ClearReadOnlyFiles(backup);
                Directory.Delete(backup, recursive: true);
            }
        }
    }

    private static void CopyTree(string source, string destination, CancellationToken token, bool skipGenerated = false)
    {
        RejectLink(source);
        Directory.CreateDirectory(destination);
        foreach (string file in Directory.EnumerateFiles(source))
        {
            token.ThrowIfCancellationRequested();
            RejectLink(file);
            if (Path.GetExtension(file) != ".pyc")
            {
                File.Copy(file, Path.Combine(destination, Path.GetFileName(file)), overwrite: true);
            }
        }
        foreach (string folder in Directory.EnumerateDirectories(source))
        {
            token.ThrowIfCancellationRequested();
            string name = Path.GetFileName(folder);
            if (name == "__pycache__" || (skipGenerated && name == "recomp"))
            {
                continue;
            }
            CopyTree(folder, Path.Combine(destination, name), token, skipGenerated);
        }
    }

    public void RemoveEnvironment(CancellationToken token)
    {
        string environment = Path.GetFullPath(SetupSettings.EnvironmentDirectory);
        string expected = Path.GetFullPath(Path.Combine(SetupSettings.Home, "environment"));
        RemoveEnvironmentAt(environment, expected, token);
    }

    internal static void RemoveEnvironmentAt(string environment, string expected, CancellationToken token)
    {
        environment = Path.GetFullPath(environment);
        expected = Path.GetFullPath(expected);
        if (!environment.Equals(expected, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException("The build environment path is invalid.");
        }
        if (!Directory.Exists(environment))
        {
            return;
        }
        RequireMarker(environment, EnvironmentMarker);
        RejectLinks(environment);
        token.ThrowIfCancellationRequested();
        ClearReadOnlyFiles(environment);
        Directory.Delete(environment, recursive: true);
    }

    private static void ClearReadOnlyFiles(string directory)
    {
        // Git object files can be read-only. Clear that flag only in the verified owned tree.
        foreach (string file in Directory.EnumerateFiles(directory, "*", SearchOption.AllDirectories))
        {
            FileAttributes attributes = File.GetAttributes(file);
            if ((attributes & FileAttributes.ReadOnly) != 0)
            {
                File.SetAttributes(file, attributes & ~FileAttributes.ReadOnly);
            }
        }
    }

    private static void RequireMarker(string directory, string marker)
    {
        string path = Path.Combine(directory, marker);
        if (!File.Exists(path) || File.ReadAllText(path) != MarkerContents)
        {
            throw new InvalidOperationException("Choose an empty installation folder, or a folder already managed by DogGame Setup.");
        }
    }

    private static void RejectLink(string path)
    {
        if ((File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0)
        {
            throw new InvalidOperationException("Setup cannot copy or remove symbolic links or junctions: " + path);
        }
    }

    private static void RejectLinks(string directory)
    {
        RejectLink(directory);
        foreach (string entry in Directory.EnumerateFileSystemEntries(directory))
        {
            RejectLink(entry);
            if (Directory.Exists(entry))
            {
                RejectLinks(entry);
            }
        }
    }
}
