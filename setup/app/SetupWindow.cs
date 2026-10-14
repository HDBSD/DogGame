namespace DogGame.Setup;

internal sealed class SetupWindow : Form
{
    private readonly CheckBox removeAfterBuild = new() { Text = "Remove build environment after build", AutoSize = true };
    private readonly CheckBox betaBuild = new() { Text = "Enable beta build", AutoSize = true };
    private readonly SetupProgress progress = new();
    private readonly Label status = new() { Text = "Ready.", AutoEllipsis = true };
    private readonly Button build = new() { Text = "Build" };
    private readonly Button update = new() { Text = "Update" };
    private readonly Button remove = new() { Text = "Remove environment" };
    private readonly Button cancel = new() { Text = "Cancel", Enabled = false };
    private readonly Button toggleOutput = new() { Text = "Show output" };
    private readonly BuildOutputPanel output = new() { Visible = false };
    private readonly BootstrapRunner runner = new();
    private SetupSettings settings = new();
    private CancellationTokenSource? operation;
    private bool closeWhenFinished;
    private bool requirementsMet;

    public SetupWindow(bool detectRequirements = true)
    {
        Text = "DogGame Setup Tool";
        ClientSize = new Size(650, 660);
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        Font = new Font("Segoe UI", 9);
        Icon = Icon.ExtractAssociatedIcon(Environment.ProcessPath!);

        var heading = new Label
        {
            Text = "DogGame PC port — work in progress", Font = new Font("Segoe UI", 11, FontStyle.Bold)
        };
        heading.SetBounds(18, 18, 614, 26);
        var information = new Label
        {
            BackColor = SystemColors.Control,
            Text = "This is an ongoing project. The recompilation may be playable, but it is not perfect.\r\n\r\n" +
                   "DogGame uses xboxrecomp. Both projects have used AI for log review, documentation, code review, code documentation, refactoring and code generation.\r\n\r\n" +
                   "DogGame has attempted to keep code generation to a minimum. xboxrecomp is an independent project with its own approach to AI use.\r\n\r\n" +
                   "The goal is a fully playable PC port of the dog game. Version 1.0.0 targets a fully playable experience with minor enhancements.\r\n\r\n" +
                   "For 1.0, reproducing original glitches or unintended behavior is not a target. If they are replicated, we do not plan to patch them."
        };
        information.SetBounds(18, 51, 614, 420);
        removeAfterBuild.SetBounds(18, 487, 600, 24);
        betaBuild.SetBounds(18, 516, 600, 24);
        progress.SetBounds(18, 553, 614, 22);
        status.SetBounds(18, 580, 614, 20);
        build.SetBounds(18, 615, 90, 28);
        update.SetBounds(116, 615, 90, 28);
        remove.SetBounds(214, 615, 190, 28);
        cancel.SetBounds(412, 615, 90, 28);
        toggleOutput.SetBounds(510, 615, 122, 28);
        output.SetBounds(650, 0, 500, 660);
        Controls.AddRange([heading, information, removeAfterBuild, betaBuild, progress, status,
                          build, update, remove, cancel, toggleOutput, output]);
        AcceptButton = build;

        try
        {
            settings = SetupSettings.Load();
        }
        catch (Exception error) when (error is IOException or System.Text.Json.JsonException or UnauthorizedAccessException)
        {
            status.Text = "Saved setup choices could not be loaded.";
        }
        removeAfterBuild.Checked = settings.RemoveEnvironmentAfterBuild;
        betaBuild.Checked = settings.EnableBetaBuild;
        requirementsMet = !detectRequirements;
        build.Enabled = update.Enabled = requirementsMet;
        if (detectRequirements)
        {
            status.Text = "Checking build requirements…";
            Shown += async (_, _) => await RunAsync(async token =>
            {
                string scratch = Path.Combine(Path.GetTempPath(), "DogGameSetup-requirements-" + Guid.NewGuid().ToString("N"));
                try
                {
                    RequirementResult[] results = await RequirementChecker.CheckAsync(scratch, output.Enqueue, token);
                    requirementsMet = results.Length > 0 && results.All(result => result.Passed);
                    status.Text = requirementsMet ? "Requirements met. Ready to build." : "Missing build requirements. See output.";
                    if (!requirementsMet) ShowOutput(true);
                }
                finally
                {
                    if (Directory.Exists(scratch))
                    {
                        await Task.Run(() => Directory.Delete(scratch, recursive: true));
                    }
                }
            });
        }
        build.Click += async (_, _) => await BuildAsync(reuseChoices: false);
        update.Click += async (_, _) => await BuildAsync(reuseChoices: true);
        cancel.Click += (_, _) => CancelOperation();
        toggleOutput.Click += (_, _) => ShowOutput(!output.Visible);
        remove.Click += async (_, _) => await RunAsync(async token =>
        {
            status.Text = "Removing build environment…";
            await Task.Run(() => runner.RemoveEnvironment(token), token);
            status.Text = "Build environment removed. Your installed game is unchanged.";
        });
        FormClosing += (_, eventArgs) =>
        {
            if (operation != null)
            {
                eventArgs.Cancel = true;
                closeWhenFinished = true;
                CancelOperation();
            }
        };
    }

    private async Task BuildAsync(bool reuseChoices)
    {
        string? game = reuseChoices ? settings.GameDirectory : null;
        string? destination = settings.InstallDirectory;
        if (game == null || !Directory.Exists(game))
        {
            game = SelectFolder("Select the extracted Xbox game folder containing default.xbe", settings.GameDirectory);
            if (game == null)
            {
                return;
            }
        }
        if (!reuseChoices || destination == null)
        {
            destination = SelectFolder("Select a dedicated folder for DogGame", destination);
            if (destination == null)
            {
                return;
            }
        }
        settings = new SetupSettings
        {
            GameDirectory = game,
            InstallDirectory = destination,
            RemoveEnvironmentAfterBuild = removeAfterBuild.Checked,
            EnableBetaBuild = betaBuild.Checked
        };
        await RunAsync(async token =>
        {
            output.ClearOutput();
            ShowOutput(true);
            settings.Save();
            var reporting = new Progress<BuildStatus>(message =>
            {
                progress.Style = message.Indeterminate ? ProgressBarStyle.Marquee : ProgressBarStyle.Continuous;
                progress.Value = Math.Clamp(message.Percent, 0, 100);
                status.Text = message.Message;
            });
            await runner.BuildAsync(settings, reporting, output.Enqueue, token);
            progress.Style = ProgressBarStyle.Continuous;
            progress.Value = 100;
            status.Text = "Build complete. DogGame.exe is ready.";
        });
    }

    private async Task RunAsync(Func<CancellationToken, Task> action)
    {
        if (operation != null)
        {
            return;
        }
        operation = new CancellationTokenSource();
        cancel.Enabled = true;
        foreach (Control control in new Control[] { build, update, remove, removeAfterBuild, betaBuild })
        {
            control.Enabled = false;
        }
        progress.Value = 0;
        progress.Style = ProgressBarStyle.Marquee;
        try
        {
            await action(operation.Token);
        }
        catch (OperationCanceledException)
        {
            status.Text = "Setup stopped. The previous installation is unchanged.";
        }
        catch (Exception error)
        {
            output.Enqueue(error.Message);
            ShowOutput(true);
            status.Text = "Setup failed. See the error details.";
            MessageBox.Show(this, error.Message, "DogGame Setup Tool", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            operation.Dispose();
            operation = null;
            cancel.Enabled = false;
            output.Flush();
            progress.Style = ProgressBarStyle.Continuous;
            foreach (Control control in new Control[] { build, update, remove, removeAfterBuild, betaBuild })
            {
                control.Enabled = true;
            }
            build.Enabled = update.Enabled = requirementsMet;
            if (closeWhenFinished)
            {
                Close();
            }
        }
    }

    private void CancelOperation()
    {
        if (operation == null || operation.IsCancellationRequested)
        {
            return;
        }
        cancel.Enabled = false;
        status.Text = "Cancelling — waiting for the build to stop…";
        operation.Cancel();
    }

    internal void ShowOutput(bool visible)
    {
        ClientSize = new Size(visible ? 1150 : 650, 660);
        output.Visible = visible;
        toggleOutput.Text = visible ? "Hide output" : "Show output";
        if (Right > Screen.FromControl(this).WorkingArea.Right)
        {
            Left = Math.Max(Screen.FromControl(this).WorkingArea.Left,
                            Screen.FromControl(this).WorkingArea.Right - Width);
        }
    }

    private string? SelectFolder(string description, string? previous)
    {
        using var dialog = new FolderBrowserDialog
        {
            Description = description,
            UseDescriptionForTitle = true,
            InitialDirectory = previous ?? Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)
        };
        return dialog.ShowDialog(this) == DialogResult.OK ? dialog.SelectedPath : null;
    }
}
