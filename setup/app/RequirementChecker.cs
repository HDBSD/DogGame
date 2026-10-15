using System.Diagnostics;
using System.Reflection;
using System.Text.Json;

namespace DogGame.Setup;

internal sealed record RequirementResult(string Name, bool Passed, string Detail);

internal static class RequirementChecker
{
    internal static async Task<RequirementResult[]> CheckAsync(string scratch, Action<string> output, CancellationToken token, string tools)
    {
        Directory.CreateDirectory(scratch);
        string script = Path.Combine(scratch, "Check-Requirements.ps1");
        using (Stream payload = Assembly.GetExecutingAssembly().GetManifestResourceStream("DogGame.Setup.Requirements.ps1")
                                ?? throw new InvalidOperationException("The requirements checker is missing."))
        {
            await using var destination = File.Create(script);
            await payload.CopyToAsync(destination, token);
        }
        var start = new ProcessStartInfo
        {
            FileName = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell", "v1.0", "powershell.exe"),
            UseShellExecute = false, CreateNoWindow = true,
            RedirectStandardOutput = true, RedirectStandardError = true
        };
        foreach (string argument in new[] { "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", script,
                                           "-ScratchDirectory", Path.Combine(scratch, "compiler-test-" + Guid.NewGuid().ToString("N")) })
        {
            start.ArgumentList.Add(argument);
        }
        using var process = new Process { StartInfo = start };
        PortableToolchain.Configure(start, tools);
        start.ArgumentList.Add("-ToolchainDirectory");
        start.ArgumentList.Add(tools);
        process.Start();
        Task<string> json = process.StandardOutput.ReadToEndAsync();
        Task errors = Task.Run(async () =>
        {
            while (await process.StandardError.ReadLineAsync().ConfigureAwait(false) is string line)
            {
                output(line);
            }
        });
        try
        {
            await process.WaitForExitAsync(token).ConfigureAwait(false);
        }
        catch (OperationCanceledException)
        {
            try
            {
                if (!process.HasExited) process.Kill(entireProcessTree: true);
            }
            catch (InvalidOperationException) when (process.HasExited) { }
            await process.WaitForExitAsync().ConfigureAwait(false);
            await Task.WhenAll(json, errors).ConfigureAwait(false);
            throw;
        }
        await errors.ConfigureAwait(false);
        string result = await json.ConfigureAwait(false);
        if (process.ExitCode != 0)
        {
            throw new InvalidOperationException("The requirements check could not finish. See the output panel.");
        }
        return JsonSerializer.Deserialize<RequirementResult[]>(result) ?? [];
    }
}
