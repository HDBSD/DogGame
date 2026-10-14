namespace DogGame.Setup;

internal sealed class SetupProgress : Control
{
    private readonly System.Windows.Forms.Timer animation = new() { Interval = 40 };
    private int value;
    private int position;
    private ProgressBarStyle style;

    [System.ComponentModel.DesignerSerializationVisibility(System.ComponentModel.DesignerSerializationVisibility.Hidden)]
    public int Value
    {
        get => value;
        set
        {
            this.value = Math.Clamp(value, 0, 100);
            Invalidate();
        }
    }

    [System.ComponentModel.DesignerSerializationVisibility(System.ComponentModel.DesignerSerializationVisibility.Hidden)]
    public ProgressBarStyle Style
    {
        get => style;
        set
        {
            style = value;
            animation.Enabled = style == ProgressBarStyle.Marquee;
            Invalidate();
        }
    }

    public SetupProgress()
    {
        DoubleBuffered = true;
        AccessibleRole = AccessibleRole.ProgressBar;
        AccessibleName = "Build progress";
        animation.Tick += (_, _) =>
        {
            position = (position + 5) % Math.Max(1, Width);
            Invalidate();
        };
    }

    protected override void OnPaint(PaintEventArgs eventArgs)
    {
        var bounds = new Rectangle(0, 0, Width - 1, Height - 1);
        eventArgs.Graphics.FillRectangle(SystemBrushes.ControlLight, bounds);
        using var fill = new SolidBrush(Color.FromArgb(66, 156, 83));
        if (style == ProgressBarStyle.Marquee)
        {
            eventArgs.Graphics.FillRectangle(fill, new Rectangle(position, 1, Math.Min(Width / 4, Width - position - 1), Height - 2));
        }
        else
        {
            eventArgs.Graphics.FillRectangle(fill, new Rectangle(1, 1, (Width - 2) * value / 100, Height - 2));
        }
        eventArgs.Graphics.DrawRectangle(SystemPens.ControlDark, bounds);
        TextRenderer.DrawText(eventArgs.Graphics, style == ProgressBarStyle.Marquee ? "Working…" : $"{value}%",
                              Font, bounds, Color.Black, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            animation.Dispose();
        }
        base.Dispose(disposing);
    }
}
