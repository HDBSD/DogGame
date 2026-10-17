using System.Diagnostics;

namespace DogGame.Setup;

// Installation follows the actual executable, including single-file publishing.
// This class describes launch requests; only explicit UI actions start processes.
internal sealed class SetupInstallation
{
    private readonly string setupExecutable;
    internal string SetupDirectory { get; }
    internal string DirectoryPath => Path.Combine(SetupDirectory, "DogGame-Contents");
    internal string GameExecutable => Path.Combine(DirectoryPath, "DogGame.exe");
    internal bool IsInstalled => File.Exists(GameExecutable);
    internal bool FolderExists => Directory.Exists(DirectoryPath);

    internal static SetupInstallation Current => new(Environment.ProcessPath
        ?? throw new InvalidOperationException("The setup executable location could not be determined."));

    internal SetupInstallation(string executable)
    {
        setupExecutable = Path.GetFullPath(executable);
        SetupDirectory = Path.GetDirectoryName(setupExecutable)
            ?? throw new InvalidOperationException("The setup executable needs a parent folder.");
    }

    internal bool HasOtherEntries()
    {
        return Directory.EnumerateFileSystemEntries(SetupDirectory).Any(entry =>
            !Path.GetFullPath(entry).Equals(setupExecutable, StringComparison.OrdinalIgnoreCase) &&
            !Path.GetFullPath(entry).Equals(DirectoryPath, StringComparison.OrdinalIgnoreCase));
    }

    internal ProcessStartInfo PlayRequest()
    {
        if (!IsInstalled)
        {
            throw new InvalidOperationException("Build DogGame before playing.");
        }
        return new ProcessStartInfo
        {
            FileName = GameExecutable,
            WorkingDirectory = DirectoryPath,
            UseShellExecute = true
        };
    }

    internal ProcessStartInfo OpenFolderRequest()
    {
        if (!FolderExists)
        {
            throw new InvalidOperationException("The game folder has not been created yet.");
        }
        return new ProcessStartInfo { FileName = DirectoryPath, UseShellExecute = true };
    }
}
