using System.Diagnostics;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;

namespace DogGame.Setup;

internal static class PortableToolchain
{
    internal static bool IsBundled => Assembly.GetExecutingAssembly().GetManifestResourceNames().Contains("DogGame.Setup.Tools.zip");

    internal static async Task ProvisionAsync(string tools, Action<string> output, CancellationToken token)
    {
        Directory.CreateDirectory(tools);
        string archive = Path.Combine(tools, "tools.zip");
        using Stream payload = Assembly.GetExecutingAssembly().GetManifestResourceStream("DogGame.Setup.Tools.zip")
            ?? throw new InvalidOperationException("Publish setup with its portable toolchain before building.");
        using Stream hashResource = Assembly.GetExecutingAssembly().GetManifestResourceStream("DogGame.Setup.Tools.sha256")
            ?? throw new InvalidOperationException("The toolchain checksum is missing.");
        using var reader = new StreamReader(hashResource);
        string expected = (await reader.ReadToEndAsync(token)).Trim();
        await using (var destination = File.Create(archive))
        {
            await payload.CopyToAsync(destination, token).ConfigureAwait(false);
        }
        await using (var file = File.OpenRead(archive))
        {
            string actual = Convert.ToHexString(await SHA256.HashDataAsync(file, token).ConfigureAwait(false));
            if (!actual.Equals(expected, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException("The bundled toolchain checksum does not match.");
            }
        }
        output("Extracting verified portable Python, Git, CMake, Ninja and LLVM-MinGW tools.");
        await Task.Run(() =>
        {
            using var zip = ZipFile.OpenRead(archive);
            string root = Path.GetFullPath(tools) + Path.DirectorySeparatorChar;
            foreach (ZipArchiveEntry entry in zip.Entries)
            {
                token.ThrowIfCancellationRequested();
                string destination = Path.GetFullPath(Path.Combine(tools, entry.FullName));
                if (!destination.StartsWith(root, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException("Toolchain archive contains an invalid path.");
                }
                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                entry.ExtractToFile(destination, overwrite: true);
            }
            File.Copy(Path.Combine(tools, "python", "portable._pth.template"), Path.Combine(tools, "python", "python313._pth"), overwrite: true);
        }, token).ConfigureAwait(false);
        File.Delete(archive);
    }

    internal static void Configure(ProcessStartInfo start, string tools)
    {
        start.Environment["PATH"] = string.Join(Path.PathSeparator, new[]
        {
            Path.Combine(tools, "python"), Path.Combine(tools, "git", "cmd"),
            Path.Combine(tools, "llvm", "bin"), Path.Combine(tools, "cmake", "bin"),
            Path.Combine(tools, "ninja"), Environment.GetFolderPath(Environment.SpecialFolder.System),
            Environment.GetFolderPath(Environment.SpecialFolder.Windows)
        });
    }
}
