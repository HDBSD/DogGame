using System.Text.Json;

namespace DogGame.Setup;

internal sealed record SetupSettings
{
    public string? GameDirectory { get; init; }
    public string? InstallDirectory { get; init; }
    public bool RemoveEnvironmentAfterBuild { get; init; } = true;
    public int CleanupDefaultVersion { get; init; } = 1;
    public bool EnableBetaBuild { get; init; }

    public static string Home => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "DogGameSetup");
    public static string EnvironmentDirectory => Path.Combine(Home, "environment");

    public static SetupSettings Load()
    {
        string path = Path.Combine(Home, "settings.json");
        if (!File.Exists(path))
        {
            return new SetupSettings();
        }
        string json = File.ReadAllText(path);
        SetupSettings settings = JsonSerializer.Deserialize<SetupSettings>(json) ?? new SetupSettings();
        using var document = JsonDocument.Parse(json);
        // Earlier versions saved an unchecked default. Adopt the new default once;
        // subsequent explicit choices remain preserved.
        return document.RootElement.TryGetProperty(nameof(CleanupDefaultVersion), out _)
            ? settings : settings with { RemoveEnvironmentAfterBuild = true };
    }

    public void Save()
    {
        Directory.CreateDirectory(Home);
        string path = Path.Combine(Home, "settings.json");
        string temporary = path + ".tmp";
        File.WriteAllText(temporary, JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true }));
        File.Move(temporary, path, overwrite: true);
    }
}
