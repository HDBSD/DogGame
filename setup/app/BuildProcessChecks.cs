using System.Collections.Concurrent;
using System.Diagnostics;
using System.Text;

namespace DogGame.Setup;

internal static class BuildProcessChecks
{
    internal static async Task RunAsync()
    {
        string executable = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
                                         "WindowsPowerShell", "v1.0", "powershell.exe");
        string childScript = Convert.ToBase64String(Encoding.Unicode.GetBytes("Start-Sleep -Seconds 20"));
        string script = $"$child = Start-Process -FilePath '{executable.Replace("'", "''")}' " +
                        $"-ArgumentList '-NoProfile','-EncodedCommand','{childScript}' -WindowStyle Hidden -PassThru; " +
                        "Write-Output ('child:' + $child.Id); Write-Output ('parent:' + $PID); " +
                        "[Console]::Error.WriteLine('stderr fixture'); Write-Output 'stdout fixture'; Start-Sleep -Seconds 20";
        var start = new ProcessStartInfo
        {
            FileName = executable, UseShellExecute = false, CreateNoWindow = true,
            RedirectStandardOutput = true, RedirectStandardError = true
        };
        start.ArgumentList.Add("-NoProfile");
        start.ArgumentList.Add("-EncodedCommand");
        start.ArgumentList.Add(Convert.ToBase64String(Encoding.Unicode.GetBytes(script)));
        string log = Path.Combine(AppContext.BaseDirectory, "process-self-test.log");
        var lines = new ConcurrentQueue<string>();
        var ready = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        using var cancellation = new CancellationTokenSource();
        Task running = BuildProcess.RunAsync(start, log, line =>
        {
            lines.Enqueue(line);
            if (lines.Any(value => value == "stdout fixture") && lines.Any(value => value == "stderr fixture"))
            {
                ready.TrySetResult();
            }
        }, cancellation.Token);
        try
        {
            await ready.Task.WaitAsync(TimeSpan.FromSeconds(10)).ConfigureAwait(false);
            if (running.IsCompleted)
            {
                throw new InvalidOperationException("Build output was not delivered before process exit.");
            }
        }
        finally
        {
            cancellation.Cancel();
            try
            {
                await running.WaitAsync(TimeSpan.FromSeconds(10)).ConfigureAwait(false);
                throw new InvalidOperationException("The process runner did not report cancellation.");
            }
            catch (OperationCanceledException)
            {
                // Expected: cancellation must complete before checking child-process lifetimes.
            }
        }
        foreach (string line in lines.Where(line => line.StartsWith("child:") || line.StartsWith("parent:")))
        {
            int id = int.Parse(line[(line.IndexOf(':') + 1)..]);
            try
            {
                using var process = Process.GetProcessById(id);
                if (!process.HasExited)
                {
                    throw new InvalidOperationException("Cancellation left a build process running.");
                }
            }
            catch (ArgumentException)
            {
                // The OS has already removed the exited process.
            }
        }
        string recorded = await File.ReadAllTextAsync(log).ConfigureAwait(false);
        if (!recorded.Contains("stdout fixture") || !recorded.Contains("stderr fixture"))
        {
            throw new InvalidOperationException("The on-disk log lost stdout or stderr.");
        }
    }
}
