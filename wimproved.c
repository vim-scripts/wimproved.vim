/*
The MIT License (MIT)

Copyright (c) 2015 Killian Koenig

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include "Windows.h"
#include "stdio.h"

#define EXPECT(condition)                                                      \
    do                                                                         \
    {                                                                          \
        int value = !(condition);                                              \
        if (value)                                                             \
        {                                                                      \
            display_error(#condition, __LINE__, __FILE__);                     \
            goto error;                                                        \
        }                                                                      \
    } while ((void)0, 0)

/* #define VERBOSE_ERRORS */
#if defined(VERBOSE_ERRORS)
static void display_error(const char* error, int line, const char* file)
{
    DWORD last_error = GetLastError();
    char content[MAX_PATH];
    snprintf(content, sizeof(content), "%s(%d)\n%s", file, line, error);
    MessageBoxA(NULL, content, "wimproved.vim", MB_ICONEXCLAMATION);
    if (last_error)
    {
        char formatted[MAX_PATH];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, last_error, 0, formatted, sizeof(formatted), NULL);
        snprintf(content, sizeof(content), "%s(%d)\n%s", file, line, formatted);
        MessageBoxA(NULL, content, "wimproved.vim", MB_ICONEXCLAMATION);
    }
}
#else
#define display_error(error, line, file)
#endif

static BOOL CALLBACK enum_windows_proc(_In_ HWND hwnd, _In_ LPARAM lparam)
{
    DWORD process;
    if (IsWindowVisible(hwnd) && GetWindowThreadProcessId(hwnd, &process) &&
        process == GetCurrentProcessId())
    {
        *((HWND*)lparam) = hwnd;
        return FALSE;
    }

    return TRUE;
}

static BOOL CALLBACK enum_child_windows_proc(_In_ HWND hwnd, _In_ LPARAM lparam)
{
    char class_name[MAX_PATH];
    if (!GetClassNameA(hwnd, class_name,
                       sizeof(class_name) / sizeof(class_name[0])) ||
        strcmp(class_name, "VimTextArea"))
    {
        return TRUE;
    }

    *((HWND*)lparam) = hwnd;
    return FALSE;
}

static HWND get_hwnd(void)
{
    HWND hwnd = NULL;
    EnumWindows(&enum_windows_proc, (LPARAM)&hwnd);
    return hwnd;
}

static HWND get_textarea_hwnd(void)
{
    HWND hwnd = get_hwnd();
    if (hwnd == NULL)
    {
        return NULL;
    }

    HWND child = NULL;
    (void)EnumChildWindows(hwnd, &enum_child_windows_proc, (LPARAM)&child);
    return child;
}

static int force_redraw(HWND hwnd)
{
    return SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE |
                                                    SWP_NOREPOSITION |
                                                    SWP_NOSIZE);
}

static int adjust_exstyle_flags(HWND hwnd, long flags, int predicate)
{
    DWORD style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    EXPECT(style || !GetLastError());

    if (!predicate)
    {
        style |= flags;
    }
    else
    {
        style &= ~flags;
    }

    /* Error code for SetWindowLong is ambiguous see:
     * https://msdn.microsoft.com/en-us/library/windows/desktop/ms633591(v=vs.85).aspx
     */
    SetLastError(0);
    EXPECT(GetLastError() == 0);

    /* TODO : Windows 10 this appears to leak error state */
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, style);
    /* EXPECT(
      SetWindowLongPtr(hwnd, GWL_EXSTYLE, style) || !GetLastError()); */

    return 1;

error:
    return 0;
}

static int adjust_style_flags(HWND hwnd, long flags, int predicate)
{
    DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);
    EXPECT(style || !GetLastError());

    if (!predicate)
    {
        style |= flags;
    }
    else
    {
        style &= ~flags;
    }

    /* Error code for SetWindowLong is ambiguous see:
     * https://msdn.microsoft.com/en-us/library/windows/desktop/ms633591(v=vs.85).aspx
     */
    SetLastError(0);
    EXPECT(SetWindowLongPtr(hwnd, GWL_STYLE, style) || !GetLastError());

    return 1;

error:
    return 0;
}

__declspec(dllexport) int update_window_brush(long arg);

static int set_window_style(int is_clean_enabled, int arg)
{
    EXPECT(update_window_brush(arg));

    HWND child;
    EXPECT((child = get_textarea_hwnd()) != NULL);

    HWND parent;
    EXPECT((parent = get_hwnd()) != NULL);

    EXPECT(adjust_exstyle_flags(child, WS_EX_CLIENTEDGE, is_clean_enabled));
    EXPECT(force_redraw(child));

    RECT parent_cr;
    EXPECT(GetClientRect(parent, &parent_cr));

    RECT child_wr;
    EXPECT(GetWindowRect(child, &child_wr));

    int w = child_wr.right - child_wr.left;
    int h = child_wr.bottom - child_wr.top;
    LONG left, top;
    if (is_clean_enabled)
    {
        /* Center the text area window in the parent window client area */
        left = (parent_cr.right - parent_cr.left - w) / 2;
        top = (parent_cr.bottom - parent_cr.top - h) / 2;

        /* With WS_EX_CLIENTEDGE removed gVim will not fill the entire client
         * area,
         * but we can center it and hide this by using the same background color
         * for both the parent and child window */
        left += 2;
        top += 2;
    }
    else
    {
        left = top = 0;
    }

    EXPECT(SetWindowPos(child, NULL, left, top, w, h,
                        SWP_NOZORDER | SWP_NOACTIVATE));

    return 1;

error:
    return 0;
}

static int set_fullscreen(int should_be_fullscreen, int color)
{
    HWND parent;
    EXPECT((parent = get_hwnd()) != NULL);

    adjust_style_flags(parent, WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX |
                                   WS_MINIMIZEBOX,
                       should_be_fullscreen);
    force_redraw(parent);

    if (should_be_fullscreen)
    {
        RECT window;
        EXPECT(GetWindowRect(parent, &window));

        HMONITOR monitor;
        EXPECT((monitor = MonitorFromRect(&window, MONITOR_DEFAULTTONEAREST)) !=
               NULL);

        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        EXPECT(GetMonitorInfo(monitor, &mi));

        RECT r = mi.rcMonitor;
        EXPECT(SetWindowPos(parent, NULL, r.left, r.top, r.right - r.left,
                            r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE));
    }

    set_window_style(should_be_fullscreen, color);

    return 1;

error:
    return 0;
}

__declspec(dllexport) int set_alpha(long arg)
{
    arg = min(arg, 0xFF);
    arg = max(arg, 0x00);

    HWND hwnd;
    EXPECT((hwnd = get_hwnd()) != NULL);

    /* WS_EX_LAYERED must be set if there is any transparency */
    EXPECT(adjust_exstyle_flags(hwnd, WS_EX_LAYERED, arg == 0xFF));
    EXPECT(SetLayeredWindowAttributes(hwnd, 0, (BYTE)(arg), LWA_ALPHA));

    return 1;

error:
    return 0;
}

__declspec(dllexport) int set_monitor_center(long arg)
{
    arg = min(arg, 100);
    arg = max(arg, 0);

    HWND hwnd;
    EXPECT((hwnd = get_hwnd()) != NULL);

    RECT window;
    EXPECT(GetWindowRect(hwnd, &window));

    HMONITOR monitor;
    EXPECT((monitor = MonitorFromRect(&window, MONITOR_DEFAULTTONEAREST)) !=
           NULL);

    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    EXPECT(GetMonitorInfo(monitor, &mi));

    int w, h;
    if (arg != 0)
    {
        double scale = arg / 100.0;
        w = scale * (mi.rcMonitor.right - mi.rcMonitor.left);
        h = scale * (mi.rcMonitor.bottom - mi.rcMonitor.top);
    }
    else
    {
        w = window.right - window.left;
        h = window.bottom - window.top;
    }

    window.left =
        mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - w) / 2;
    window.top =
        mi.rcMonitor.top + (mi.rcMonitor.bottom - mi.rcMonitor.top - h) / 2;
    window.right = window.left + w;
    window.bottom = window.top + h;

    EXPECT(SetWindowPos(hwnd, NULL, window.left, window.top, w, h,
                        SWP_NOZORDER | SWP_NOACTIVATE));

    return 1;

error:
    return 0;
}

__declspec(dllexport) int set_fullscreen_on(long arg)
{
    return set_fullscreen(1, arg);
}

__declspec(dllexport) int set_fullscreen_off(long arg)
{
    return set_fullscreen(0, arg);
}

__declspec(dllexport) int set_window_style_clean(long arg)
{
    return set_window_style(1, arg);
}

__declspec(dllexport) int set_window_style_default(long arg)
{
    return set_window_style(0, arg);
}

__declspec(dllexport) int update_window_brush(long arg)
{
    /* TODO: Don't leak brush */
    HBRUSH brush;
    COLORREF color = RGB((arg >> 16) & 0xFF, (arg >> 8) & 0xFF, arg & 0xFF);
    EXPECT((brush = CreateSolidBrush(color)) != NULL);

    HWND child;
    EXPECT((child = get_textarea_hwnd()) != NULL);

    EXPECT(SetClassLongPtr(child, GCLP_HBRBACKGROUND, (LONG)brush) ||
           !GetLastError());

    HWND parent;
    EXPECT((parent = get_hwnd()) != NULL);
    EXPECT(SetClassLongPtr(parent, GCLP_HBRBACKGROUND, (LONG)brush) ||
           !GetLastError());

    EXPECT(RedrawWindow(parent, 0, 0, RDW_INVALIDATE));
    EXPECT(force_redraw(parent));

    return 1;

error:
    return 0;
}
