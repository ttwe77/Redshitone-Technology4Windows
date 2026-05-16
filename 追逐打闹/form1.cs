using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;

public class EscapeWindow : Form
{
    private Label lblInfo;
    private Button btnEnter;

    private double posX = 100.0;
    private double posY = 100.0;
    private bool isReturning = false;
    private double targetX;
    private double targetY;

    private int currentDpi = 96;
    private const int StandardWidth = 420;
    private const int StandardHeight = 220;

    [StructLayout(LayoutKind.Sequential)]
    private struct RECT
    {
        public int Left, Top, Right, Bottom;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct POINT
    {
        public int X, Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct MONITORINFO
    {
        public int cbSize;
        public RECT rcMonitor;
        public RECT rcWork;
        public uint dwFlags;
    }

    [DllImport("user32.dll")]
    private static extern IntPtr MonitorFromWindow(IntPtr hwnd, uint dwFlags);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern bool GetMonitorInfoW(IntPtr hMonitor, ref MONITORINFO lpmi);

    [DllImport("user32.dll")]
    private static extern bool GetCursorPos(out POINT lpPoint);

    private const int MONITOR_DEFAULTTONEAREST = 2;

    public EscapeWindow()
    {
        Text = "为什么会有这个窗口？";
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = true;
        ClientSize = new Size(StandardWidth, StandardHeight);
        StartPosition = FormStartPosition.Manual;
        currentDpi = DeviceDpi;

        CreateControls();
        LayoutControls();
        CenterWindow();
        UpdateTargetCenter();
        System.Windows.Forms.Timer timer;
        timer = new System.Windows.Forms.Timer { Interval = 16 };
        timer.Tick += Timer_Tick;
        timer.Start();

        Disposed += (s, e) => timer.Dispose();
    }

    private void CreateControls()
    {
        // 标签：自动大小、完全显示文字
        lblInfo = new Label
        {
            Text = "快来抓我！",
            TextAlign = ContentAlignment.MiddleCenter,
            Font = GetScaledFont(12),
            AutoSize = false,
            Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right
        };
        Controls.Add(lblInfo);

        // 按钮
        btnEnter = new Button
        {
            Text = "进入软件",
            Font = GetScaledFont(11),
            UseVisualStyleBackColor = true
        };
        btnEnter.Click += (s, e) =>
        {
            btnEnter.Text = "已进入软件";
            MessageBox.Show(this, "已经进入软件。", "提示", MessageBoxButtons.OK, MessageBoxIcon.Information);
        };
        Controls.Add(btnEnter);
    }

    // 缩放字体（修复显示不全核心）
    private Font GetScaledFont(float baseSize)
    {
        float dpiScale = currentDpi / 96f;
        return new Font("Microsoft YaHei UI", baseSize * dpiScale);
    }

    private int Scale(int v) => (int)Math.Round(v * currentDpi / 96.0);

    private void LayoutControls()
    {
        int w = ClientSize.Width;
        int h = ClientSize.Height;

        // 标签布局
        int labelPad = Scale(20);
        

        // 按钮布局（确保完全显示、不消失）
        int btnW = Scale(140);
        int btnH = Scale(40);
        int btnX = (w - btnW) / 2;
        int btnY = h - Scale(70);
    }

    private Rectangle GetWorkArea()
    {
        IntPtr hMon = MonitorFromWindow(Handle, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = new MONITORINFO();
        mi.cbSize = Marshal.SizeOf(mi);
        GetMonitorInfoW(hMon, ref mi);
        return Rectangle.FromLTRB(mi.rcWork.Left, mi.rcWork.Top, mi.rcWork.Right, mi.rcWork.Bottom);
    }

    private void CenterWindow()
    {
        var work = GetWorkArea();
        posX = work.Left + (work.Width - StandardWidth) / 2.0;
        posY = work.Top + (work.Height - StandardHeight) / 2.0;
        Location = new Point((int)Math.Round(posX), (int)Math.Round(posY));
    }

    private void UpdateTargetCenter()
    {
        var work = GetWorkArea();
        targetX = work.Left + (work.Width - StandardWidth) / 2.0;
        targetY = work.Top + (work.Height - StandardHeight) / 2.0;
    }

    private void TeleportToOpposite(int dirX, int dirY)
    {
        var work = GetWorkArea();
        double margin = Scale(16);

        if (dirX > 0) posX = work.Left - StandardWidth + margin;
        else if (dirX < 0) posX = work.Right - margin;

        if (dirY > 0) posY = work.Top - StandardHeight + margin;
        else if (dirY < 0) posY = work.Bottom - margin;

        isReturning = true;
        UpdateTargetCenter();
        Location = new Point((int)Math.Round(posX), (int)Math.Round(posY));
    }

    private void MoveAwayFromCursor()
    {
        GetCursorPos(out POINT pt);
        var win = Bounds;
        double cx = (win.Left + win.Right) / 2.0;
        double cy = (win.Top + win.Bottom) / 2.0;

        double dx = cx - pt.X;
        double dy = cy - pt.Y;
        double dist2 = dx * dx + dy * dy;
        double danger = Scale(220);

        if (dist2 > danger * danger) return;

        double dist = Math.Sqrt(dist2);
        if (dist < 1) dist = 1;

        double step = Scale(22);
        posX += dx / dist * step;
        posY += dy / dist * step;
        Location = new Point((int)Math.Round(posX), (int)Math.Round(posY));

        var work = GetWorkArea();
        int dxDir = 0, dyDir = 0;

        if (win.Right < work.Left - Scale(8)) dxDir = 1;
        else if (win.Left > work.Right + Scale(8)) dxDir = -1;
        if (win.Bottom < work.Top - Scale(8)) dyDir = 1;
        else if (win.Top > work.Bottom + Scale(8)) dyDir = -1;

        if (dxDir != 0 || dyDir != 0) TeleportToOpposite(dxDir, dyDir);
    }

    private void SmoothReturn()
    {
        if (!isReturning) return;
        const double ease = 0.3;
        posX += (targetX - posX) * ease;
        posY += (targetY - posY) * ease;

        if (Math.Abs(targetX - posX) < 1 && Math.Abs(targetY - posY) < 1)
        {
            posX = targetX;
            posY = targetY;
            isReturning = false;
        }
        Location = new Point((int)Math.Round(posX), (int)Math.Round(posY));
    }

    private void CheckReposition()
    {
        var win = Bounds;
        var work = GetWorkArea();
        int dx = 0, dy = 0;

        if (win.Right < work.Left - Scale(8)) dx = 1;
        else if (win.Left > work.Right + Scale(8)) dx = -1;
        if (win.Bottom < work.Top - Scale(8)) dy = 1;
        else if (win.Top > work.Bottom + Scale(8)) dy = -1;

        if (dx != 0 || dy != 0) TeleportToOpposite(dx, dy);
    }

    private void Timer_Tick(object sender, EventArgs e)
    {
        if (isReturning) SmoothReturn();
        else MoveAwayFromCursor();
        CheckReposition();
    }

    protected override void OnDpiChanged(DpiChangedEventArgs e)
    {
        base.OnDpiChanged(e);
        currentDpi = e.DeviceDpiNew;
        lblInfo.Font = GetScaledFont(12);
        btnEnter.Font = GetScaledFont(11);
        LayoutControls();
    }

    protected override void OnKeyDown(KeyEventArgs e)
    {
        base.OnKeyDown(e);
        if (e.KeyCode == Keys.Escape) Close();
    }

    protected override void OnResize(EventArgs e)
    {
        base.OnResize(e);
        LayoutControls();
    }
}

static class Program
{
    [STAThread]
    static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new EscapeWindow());
    }
}