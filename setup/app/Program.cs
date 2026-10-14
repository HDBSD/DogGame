namespace DogGame.Setup;

internal static class Program
{
    [STAThread]
    private static void Main(string[] arguments)
    {
        ApplicationConfiguration.Initialize();
        if (arguments is ["--self-test"])
        {
            string result = Path.Combine(AppContext.BaseDirectory, "self-test-result.txt");
            try
            {
                BootstrapChecks.Run();
                BuildProcessChecks.RunAsync().GetAwaiter().GetResult();
                SourceCheckoutChecks.RunAsync().GetAwaiter().GetResult();
                File.WriteAllText(result, "Passed: master/beta source checkout and missing-branch rejection, window controls, live stdout/stderr, process-tree cancellation, staged installation, preserved settings and environment cleanup.");
            }
            catch (Exception error)
            {
                File.WriteAllText(result, error.ToString());
                Environment.ExitCode = 1;
            }
            return;
        }
        if (arguments is ["--preview", string output])
        {
            using var window = new SetupWindow(detectRequirements: false);
            window.ShowOutput(true);
            window.ShowInTaskbar = false;
            window.Opacity = 0;
            window.Show();
            Application.DoEvents();
            using var bitmap = new Bitmap(window.Width, window.Height);
            window.DrawToBitmap(bitmap, new Rectangle(Point.Empty, bitmap.Size));
            bitmap.Save(output, System.Drawing.Imaging.ImageFormat.Png);
            return;
        }
        using var ownership = new Mutex(initiallyOwned: true, "Local\\DogGameSetup.Bootstrap", out bool firstInstance);
        if (!firstInstance)
        {
            MessageBox.Show("DogGame Setup Tool is already running.", "DogGame Setup Tool");
            return;
        }
        Application.Run(new SetupWindow());
    }
}
