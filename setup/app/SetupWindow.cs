using System.Diagnostics;

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
    private readonly Button play = new() { Text = "Play", Enabled = false };
    private readonly Button openFolder = new() { Text = "Open game folder", Enabled = false };
    private readonly Label installLocation = new() { AutoEllipsis = true };
    private readonly Label locationWarning = new()
    {
        BackColor = Color.LightGoldenrodYellow,
        ForeColor = Color.FromArgb(92, 61, 0),
        Padding = new Padding(8, 4, 8, 4),
        Visible = false
    };
    private readonly SetupInstallation installation;
    private readonly Button toggleOutput = new() { Text = "Show output" };
    private readonly BuildOutputPanel output = new() { Visible = false };
    private readonly BootstrapRunner runner = new();
    private SetupSettings settings = new();
    private CancellationTokenSource? operation;
    private bool closeWhenFinished;
    private bool requirementsMet;

    public SetupWindow(bool detectRequirements = true, SetupInstallation? installation = null)
    {
        this.installation = installation ?? SetupInstallation.Current;
        Text = "DogGame Setup Tool";
        ClientSize = new Size(650, 712);
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
        var information = new TextBox
        {
            ReadOnly = true,
            Multiline = true,
            ScrollBars = ScrollBars.Vertical,
            BorderStyle = BorderStyle.None,
            TabStop = false,
            BackColor = SystemColors.Control,
            Text = "This is an ongoing project. The recompilation may be playable, but it is not perfect.\r\n\r\n" +
                   "DogGame uses xboxrecomp. Both projects have used AI for log review, documentation, code review, code documentation, refactoring and code generation.\r\n\r\n" +
                   "DogGame has attempted to keep code generation to a minimum. xboxrecomp is an independent project with its own approach to AI use.\r\n\r\n" +
                   "The goal is a fully playable PC port of the dog game. Version 1.0.0 targets a fully playable experience with minor enhancements.\r\n\r\n" +
                   "For 1.0, reproducing original glitches or unintended behavior is not a target. If they are replicated, we do not plan to patch them."
        };
        information.SetBounds(18, 51, 614, 330);
        installLocation.Text = "Game folder: DogGame-Contents (beside this setup executable)";
        installLocation.AccessibleDescription = this.installation.DirectoryPath;
        installLocation.SetBounds(18, 397, 614, 34);
        locationWarning.SetBounds(18, 438, 614, 48);
        removeAfterBuild.SetBounds(18, 500, 600, 24);
        betaBuild.SetBounds(18, 529, 600, 24);
        progress.SetBounds(18, 566, 614, 22);
        status.SetBounds(18, 593, 614, 20);
        build.SetBounds(18, 628, 90, 28);
        update.SetBounds(116, 628, 90, 28);
        remove.SetBounds(214, 628, 190, 28);
        cancel.SetBounds(412, 628, 90, 28);
        toggleOutput.SetBounds(510, 628, 122, 28);
        play.SetBounds(18, 666, 90, 28);
        openFolder.SetBounds(116, 666, 190, 28);
        output.SetBounds(650, 0, 500, 712);
        Controls.AddRange([heading, information, removeAfterBuild, betaBuild, progress, status,
                          build, update, remove, cancel, toggleOutput, play, openFolder, installLocation, locationWarning, output]);
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
            // Portable tools are checked after extraction when a build starts.
            // Opening the UI must not unpack hundreds of megabytes unnecessarily.
            requirementsMet = PortableToolchain.IsBundled && Environment.Is64BitOperatingSystem && Environment.Is64BitProcess;
            build.Enabled = update.Enabled = requirementsMet;
            status.Text = requirementsMet ? "Portable tools included. Ready to build." : "Publish setup with the portable x64 toolchain first.";
            if (!requirementsMet)
            {
                output.Enqueue(status.Text);
                ShowOutput(true);
            }
        }
        RefreshInstallation();
        Activated += (_, _) => RefreshInstallation();
        play.Click += (_, _) => Launch(playGame: true);
        openFolder.Click += (_, _) => Launch(playGame: false);
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
        if (game == null || !Directory.Exists(game))
        {
            game = SelectFolder("Select the extracted Xbox game folder containing default.xbe", settings.GameDirectory);
            if (game == null)
            {
                return;
            }
        }
        settings = new SetupSettings
        {
            GameDirectory = game,
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
        foreach (Control control in new Control[] { build, update, remove, play, openFolder, removeAfterBuild, betaBuild })
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
            foreach (Control control in new Control[] { build, update, remove, play, openFolder, removeAfterBuild, betaBuild })
            {
                control.Enabled = true;
            }
            RefreshInstallation();
            if (closeWhenFinished)
            {
                Close();
            }
        }
    }

    internal void RefreshInstallation()
    {
        bool idle = operation == null;
        build.Enabled = idle && requirementsMet;
        update.Enabled = idle && requirementsMet && installation.IsInstalled;
        play.Enabled = idle && installation.IsInstalled;
        openFolder.Enabled = idle && installation.FolderExists;
        try
        {
            locationWarning.Visible = installation.HasOtherEntries();
            locationWarning.Text = "This folder contains other files. Please use a dedicated setup folder.";
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            locationWarning.Visible = true;
            locationWarning.Text = "Setup could not check this folder. We recommend keeping setup in its own folder.";
        }
    }

    private void Launch(bool playGame)
    {
        if (operation != null)
        {
            return;
        }
        try
        {
            ProcessStartInfo request = playGame ? installation.PlayRequest() : installation.OpenFolderRequest();
            using Process? process = Process.Start(request);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or
                                      System.ComponentModel.Win32Exception or InvalidOperationException)
        {
            MessageBox.Show(this, error.Message, "DogGame Setup Tool", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        RefreshInstallation();
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
        ClientSize = new Size(visible ? 1150 : 650, 712);
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
