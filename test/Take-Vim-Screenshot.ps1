Param(
  [Parameter(Mandatory=$true)]
  [string]$OutFile
)

$code = @"
using System;
using System.Linq;
using System.Drawing;
using System.Drawing.Imaging;
using System.Diagnostics;
using System.Runtime.InteropServices;

public class Screenshot
{
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool GetWindowRect(IntPtr hwnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, int nFlags);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    private static Bitmap PrintWindow(IntPtr hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, out rc);

        var result = new Bitmap(rc.Right - rc.Left, rc.Bottom - rc.Top, PixelFormat.Format32bppArgb);
        var g = Graphics.FromImage(result);
        var hdc = g.GetHdc();

        PrintWindow(hwnd, hdc, 0);

        g.ReleaseHdc(hdc);
        g.Dispose();

        return result;
    }

   public static void Save(string name, string path)
   {
       var process = Process.GetProcessesByName(name).OrderByDescending(p => p.StartTime).First();
       var rect = new RECT();
       GetWindowRect(process.MainWindowHandle, out rect);
       using (var bitmap = PrintWindow(process.MainWindowHandle))
       {
           bitmap.Save(path, ImageFormat.Png);
       }
   }
}
"@

Add-Type -TypeDefinition $code -ReferencedAssemblies 'System.Drawing', 'System.Windows.Forms'
[Screenshot]::Save("GVIM", $OutFile);
