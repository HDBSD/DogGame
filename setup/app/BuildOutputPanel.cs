using System.Collections.Concurrent;
using System.Text;

namespace DogGame.Setup;

internal sealed class BuildOutputPanel : Panel
{
    private const int MaximumQueuedLines = 5000;
    private const int MaximumVisibleCharacters = 200000;
    private readonly ConcurrentQueue<string> pending = new();
    private readonly TextBox text = new()
    {
        Multiline = true, ReadOnly = true, WordWrap = false,
        ScrollBars = ScrollBars.Both, BackColor = Color.White,
        Font = new Font("Consolas", 9), Dock = DockStyle.Fill
    };
    private readonly System.Windows.Forms.Timer refresh = new() { Interval = 100 };
    private int queued;
    private int omitted;

    public BuildOutputPanel()
    {
        var heading = new Label { Text = "Build output", Dock = DockStyle.Top, Height = 28 };
        Padding = new Padding(14, 18, 18, 18);
        Controls.Add(text);
        Controls.Add(heading);
        refresh.Tick += (_, _) => Flush();
        refresh.Start();
    }

    public void Enqueue(string line)
    {
        if (Interlocked.Increment(ref queued) > MaximumQueuedLines)
        {
            Interlocked.Decrement(ref queued);
            Interlocked.Increment(ref omitted);
            return;
        }
        pending.Enqueue(line.Length > 4000 ? line[..4000] + " [line truncated in live view]" : line);
    }

    public void ClearOutput()
    {
        while (pending.TryDequeue(out _))
        {
            Interlocked.Decrement(ref queued);
        }
        Interlocked.Exchange(ref omitted, 0);
        text.Clear();
    }

    internal void Flush()
    {
        var batch = new StringBuilder();
        int dropped = Interlocked.Exchange(ref omitted, 0);
        if (dropped > 0)
        {
            batch.AppendLine($"[{dropped} live-view lines omitted. The full build log retains them.]");
        }
        for (int count = 0; count < 200 && batch.Length < 32000 && pending.TryDequeue(out string? line); count++)
        {
            Interlocked.Decrement(ref queued);
            batch.AppendLine(line);
        }
        if (batch.Length == 0)
        {
            return;
        }
        if (text.TextLength + batch.Length > MaximumVisibleCharacters)
        {
            string existing = text.Text;
            int keep = Math.Max(0, MaximumVisibleCharacters - batch.Length);
            text.Text = existing.Length > keep ? existing[^keep..] : existing;
        }
        text.AppendText(batch.ToString());
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            refresh.Dispose();
        }
        base.Dispose(disposing);
    }
}
