using System.Diagnostics;

namespace DogGame.Setup;

internal static class BuildProcess
{
    internal static async Task RunAsync(ProcessStartInfo start, string logPath,
                                       Action<string> output, CancellationToken token, bool appendLog = false)
    {
        token.ThrowIfCancellationRequested();
        using var log = new StreamWriter(logPath, append: appendLog) { AutoFlush = true };
        using var process = new Process { StartInfo = start };
        process.Start();

        async Task DrainAsync(StreamReader reader)
        {
            while (await reader.ReadLineAsync().ConfigureAwait(false) is string line)
            {
                lock (log)
                {
                    log.WriteLine(line);
                }
                output(line);
            }
        }

        // Neither stream is drained on the UI thread. In particular, an immediately
        // available burst must not synchronously monopolize the window's message loop.
        Task stdout = Task.Run(() => DrainAsync(process.StandardOutput));
        Task stderr = Task.Run(() => DrainAsync(process.StandardError));
        try
        {
            await process.WaitForExitAsync(token).ConfigureAwait(false);
        }
        catch (OperationCanceledException)
        {
            try
            {
                if (!process.HasExited)
                {
                    process.Kill(entireProcessTree: true);
                }
            }
            catch (InvalidOperationException) when (process.HasExited)
            {
                // The child exited between the cancellation check and Kill.
            }
            await process.WaitForExitAsync().ConfigureAwait(false);
            await Task.WhenAll(stdout, stderr).ConfigureAwait(false);
            output("Build cancelled.");
            throw;
        }
        await Task.WhenAll(stdout, stderr).ConfigureAwait(false);
        token.ThrowIfCancellationRequested();
        if (process.ExitCode != 0)
        {
            throw new InvalidOperationException($"The build failed (exit {process.ExitCode}). Full details: {logPath}");
        }
    }
}
