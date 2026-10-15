using System.Diagnostics;

namespace DogGame.Setup;

internal static class SourceCheckoutChecks
{
    internal static async Task RunAsync()
    {
        string root = Path.Combine(AppContext.BaseDirectory, "checkout-self-test-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        File.WriteAllText(Path.Combine(root, ".doggame-setup-environment"), "DogGame Setup Tool v1\n");
        string repository = Path.Combine(root, "repository");
        Directory.CreateDirectory(repository);
        string log = Path.Combine(root, "git.log");
        string tools = Path.Combine(root, "tools");
        async Task Git(params string[] arguments)
        {
            var start = new ProcessStartInfo
            {
                FileName = Path.Combine(tools, "git", "cmd", "git.exe"), WorkingDirectory = repository,
                UseShellExecute = false, CreateNoWindow = true,
                RedirectStandardOutput = true, RedirectStandardError = true
            };
            PortableToolchain.Configure(start, tools);
            foreach (string argument in arguments) start.ArgumentList.Add(argument);
            await BuildProcess.RunAsync(start, log, _ => { }, CancellationToken.None).ConfigureAwait(false);
        }
        try
        {
            await PortableToolchain.ProvisionAsync(tools, _ => { }, CancellationToken.None).ConfigureAwait(false);
            await Git("init", "--initial-branch=master").ConfigureAwait(false);
            Directory.CreateDirectory(Path.Combine(repository, "config"));
            foreach (string file in new[] { "Setup.ps1", "CMakeLists.txt", "config/xboxrecomp.lock.json" })
            {
                File.WriteAllText(Path.Combine(repository, file), "fixture");
            }
            File.WriteAllText(Path.Combine(repository, "channel.txt"), "master");
            await Git("add", ".");
            await Git("-c", "user.name=Setup Test", "-c", "user.email=setup-test@example.invalid",
                      "-c", "commit.gpgsign=false", "commit", "-m", "master fixture");
            await Git("checkout", "-b", "beta");
            File.WriteAllText(Path.Combine(repository, "channel.txt"), "beta");
            await Git("add", ".");
            await Git("-c", "user.name=Setup Test", "-c", "user.email=setup-test@example.invalid",
                      "-c", "commit.gpgsign=false", "commit", "-m", "beta fixture");
            foreach (bool beta in new[] { false, true })
            {
                string branch = BootstrapRunner.SourceBranch(beta);
                string checkout = Path.Combine(root, "checkout-" + branch);
                await BootstrapRunner.CloneSourceAsync(repository, branch, checkout, log, _ => { }, CancellationToken.None, tools)
                    .ConfigureAwait(false);
                if (File.ReadAllText(Path.Combine(checkout, "channel.txt")) != branch)
                {
                    throw new InvalidOperationException("Setup cloned the wrong source branch.");
                }
            }
            try
            {
                await BootstrapRunner.CloneSourceAsync(repository, "missing-beta", Path.Combine(root, "missing"),
                    log, _ => { }, CancellationToken.None, tools).ConfigureAwait(false);
            }
            catch (InvalidOperationException error) when (error.InnerException != null)
            {
                return;
            }
            throw new InvalidOperationException("A missing source branch must fail without fallback.");
        }
        finally
        {
            BootstrapRunner.RemoveEnvironmentAt(root, root, CancellationToken.None);
        }
    }
}
