#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <cstdio>
#include <cstring>
#define UNICODE
#define _UNICODE
#include "resource.h"

// 全局状态
HINSTANCE g_hInst;
HWND     g_hWnd = NULL;
HMENU    g_hTrayMenu = NULL;
NOTIFYICONDATA g_nid = {0};

bool     g_borderVisible = true;
bool     g_locked = false;
bool     g_topmost = false;          // 窗口总在最前
COLORREF g_bgColor = RGB(255, 255, 255);
HBRUSH   g_hBgBrush = NULL;
BYTE     g_opacity = 255;            // 窗口透明度，范围10~255

// 拖动相关
bool  g_bDragging = false;
POINT g_ptDragStart;
RECT  g_rcDragStart;

// 解析十六进制颜色，支持带或不带#前缀，长度必须为6
bool ParseHexColor(const char* str, COLORREF& outColor) {
    if (!str) return false;
    while (*str == '#' || *str == ' ') ++str;
    if (strlen(str) != 6) return false;
    unsigned int r, g, b;
    if (sscanf(str, "%02x%02x%02x", &r, &g, &b) != 3) {
        if (sscanf(str, "%02X%02X%02X", &r, &g, &b) != 3)
            return false;
    }
    outColor = RGB(r, g, b);
    return true;
}

// 更新窗口背景画刷
void UpdateBackgroundBrush() {
    if (g_hBgBrush) DeleteObject(g_hBgBrush);
    g_hBgBrush = CreateSolidBrush(g_bgColor);
    if (g_hWnd) InvalidateRect(g_hWnd, NULL, TRUE);
}

// 应用边框样式
void ApplyBorderStyle() {
    if (!g_hWnd) return;
    LONG_PTR style = GetWindowLongPtr(g_hWnd, GWL_STYLE);
    if (g_borderVisible) {
        style |= (WS_CAPTION | WS_SYSMENU | WS_THICKFRAME);
        style &= ~WS_POPUP;
    } else {
        style &= ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME);
        style |= WS_POPUP;
    }
    SetWindowLongPtr(g_hWnd, GWL_STYLE, style);
    SetWindowPos(g_hWnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    ShowWindow(g_hWnd, SW_SHOW);
}

// 设置窗口总在最前
void ApplyTopmost() {
    if (!g_hWnd) return;
    SetWindowPos(g_hWnd, g_topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

// 托盘图标添加/更新
void UpdateTrayIcon(HWND hWnd, bool add = true) {
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_APP + 1;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(g_nid.szTip, TEXT("空白窗口"));
    if (add)
        Shell_NotifyIcon(NIM_ADD, &g_nid);
    else
        Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

// 汉化托盘菜单项
void LocalizeTrayMenu() {
    HMENU hPopup = GetSubMenu(g_hTrayMenu, 0);
    if (!hPopup) return;
    ModifyMenu(hPopup, ID_TRAY_SHOWBORDER, MF_BYCOMMAND | MF_STRING,
               ID_TRAY_SHOWBORDER, TEXT("显示边框"));
    ModifyMenu(hPopup, ID_TRAY_LOCKPOS, MF_BYCOMMAND | MF_STRING,
               ID_TRAY_LOCKPOS, TEXT("锁定位置"));
    ModifyMenu(hPopup, ID_TRAY_TOPMOST, MF_BYCOMMAND | MF_STRING,
               ID_TRAY_TOPMOST, TEXT("总在最前"));
    ModifyMenu(hPopup, ID_TRAY_COLOR, MF_BYCOMMAND | MF_STRING,
               ID_TRAY_COLOR, TEXT("背景颜色..."));
    ModifyMenu(hPopup, ID_TRAY_EXIT, MF_BYCOMMAND | MF_STRING,
               ID_TRAY_EXIT, TEXT("退出"));
}

// ========== 颜色选择对话框过程（唯一正确版本）==========
INT_PTR CALLBACK ColorDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static COLORREF* pColor = nullptr;
    switch (msg) {
        case WM_INITDIALOG: {
            pColor = reinterpret_cast<COLORREF*>(lParam);
            
            // 1. 将当前颜色显示到编辑框 (ASCII 方式)
            char buf[10];
            sprintf(buf, "%02X%02X%02X", GetRValue(*pColor),
                    GetGValue(*pColor), GetBValue(*pColor));
            SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, buf);
            
            // 2. 手动设置对话框上的所有中文文字（解决乱码）
            SetWindowTextW(hDlg, L"选择颜色");
            SetDlgItemTextW(hDlg, -1, L"输入十六进制颜色（例如 FF8000）：");
            SetDlgItemTextW(hDlg, IDC_PRESET_RED,    L"红");
            SetDlgItemTextW(hDlg, IDC_PRESET_GREEN,  L"绿");
            SetDlgItemTextW(hDlg, IDC_PRESET_BLUE,   L"蓝");
            SetDlgItemTextW(hDlg, IDC_PRESET_WHITE,  L"白");
            SetDlgItemTextW(hDlg, IDC_PRESET_BLACK,  L"黑");
            SetDlgItemTextW(hDlg, IDC_PRESET_YELLOW, L"黄");
            SetDlgItemTextW(hDlg, IDC_PRESET_CYAN,   L"青");
            SetDlgItemTextW(hDlg, IDC_PRESET_MAGENTA,L"品红");
            SetDlgItemTextW(hDlg, IDOK,     L"确定");
            SetDlgItemTextW(hDlg, IDCANCEL, L"取消");
            
            return TRUE;
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            if (id == IDOK) {
                char buf[10] = {0};
                GetDlgItemTextA(hDlg, IDC_EDIT_COLOR, buf, sizeof(buf));
                COLORREF clr;
                if (ParseHexColor(buf, clr)) {
                    *pColor = clr;
                    EndDialog(hDlg, IDOK);
                } else {
                    MessageBoxA(hDlg, "无效的十六进制颜色。\n请输入6位十六进制数字（例如 FF8000）。",
                                "错误", MB_ICONERROR);
                }
            } else if (id == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
            } else if (id == IDC_PRESET_RED) {
                SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, "FF0000");
            } else if (id == IDC_PRESET_GREEN) {
                SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, "00FF00");
            } else if (id == IDC_PRESET_BLUE) {
                SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, "0000FF");
            } else if (id == IDC_PRESET_WHITE) {
                SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, "FFFFFF");
            } else if (id == IDC_PRESET_BLACK) {
                SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, "000000");
            } else if (id == IDC_PRESET_YELLOW) {
                SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, "FFFF00");
            } else if (id == IDC_PRESET_CYAN) {
                SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, "00FFFF");
            } else if (id == IDC_PRESET_MAGENTA) {
                SetDlgItemTextA(hDlg, IDC_EDIT_COLOR, "FF00FF");
            }
            break;
        }
    }
    return FALSE;
}
// =================================================

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hWnd = hWnd;
            g_hBgBrush = CreateSolidBrush(g_bgColor);

            // 启用分层窗口以支持透明度
            SetWindowLong(hWnd, GWL_EXSTYLE,
                          GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
            SetLayeredWindowAttributes(hWnd, 0, g_opacity, LWA_ALPHA);

            UpdateTrayIcon(hWnd);
            LocalizeTrayMenu();     // 将菜单文字改为中文
            return 0;
        }
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, g_hBgBrush);
            return 1;
        }
        case WM_LBUTTONDOWN: {
            if (!g_locked) {
                SetCapture(hWnd);
                g_bDragging = true;
                GetCursorPos(&g_ptDragStart);
                GetWindowRect(hWnd, &g_rcDragStart);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (g_bDragging) {
                POINT pt;
                GetCursorPos(&pt);
                int dx = pt.x - g_ptDragStart.x;
                int dy = pt.y - g_ptDragStart.y;
                SetWindowPos(hWnd, NULL,
                             g_rcDragStart.left + dx,
                             g_rcDragStart.top + dy,
                             0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (g_bDragging) {
                ReleaseCapture();
                g_bDragging = false;
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            // 仅当窗口拥有焦点时，滚轮调整透明度
            if (GetFocus() == hWnd) {
                int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                int step = zDelta / WHEEL_DELTA;   // 通常为±1
                int newOpacity = g_opacity + step * 10;
                if (newOpacity < 10) newOpacity = 10;
                if (newOpacity > 255) newOpacity = 255;
                g_opacity = (BYTE)newOpacity;
                SetLayeredWindowAttributes(hWnd, 0, g_opacity, LWA_ALPHA);
            }
            return 0;
        }
        case WM_CLOSE: {
            // 隐藏到托盘，而不是退出
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        case WM_DESTROY: {
            UpdateTrayIcon(hWnd, false);
            if (g_hBgBrush) DeleteObject(g_hBgBrush);
            PostQuitMessage(0);
            return 0;
        }
        case WM_APP + 1: { // 托盘图标消息
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP: {
                    POINT pt;
                    GetCursorPos(&pt);
                    SetForegroundWindow(hWnd);
                    HMENU hPopup = GetSubMenu(g_hTrayMenu, 0);
                    CheckMenuItem(hPopup, ID_TRAY_SHOWBORDER,
                                  MF_BYCOMMAND | (g_borderVisible ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(hPopup, ID_TRAY_LOCKPOS,
                                  MF_BYCOMMAND | (g_locked ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(hPopup, ID_TRAY_TOPMOST,
                                  MF_BYCOMMAND | (g_topmost ? MF_CHECKED : MF_UNCHECKED));
                    TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                   pt.x, pt.y, 0, hWnd, NULL);
                    PostMessage(hWnd, WM_NULL, 0, 0);
                    break;
                }
                // 单击托盘图标切换窗口显示/隐藏
                case WM_LBUTTONUP: {
                    if (IsWindowVisible(hWnd)) {
                        ShowWindow(hWnd, SW_HIDE);
                    } else {
                        ShowWindow(hWnd, SW_SHOW);
                        SetForegroundWindow(hWnd);
                    }
                    break;
                }
                // 保留双击显示窗口（可选，也可去掉）
                case WM_LBUTTONDBLCLK: {
                    ShowWindow(hWnd, SW_SHOW);
                    SetForegroundWindow(hWnd);
                    break;
                }
            }
            return 0;
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            switch (id) {
                case ID_TRAY_SHOWBORDER:
                    g_borderVisible = !g_borderVisible;
                    ApplyBorderStyle();
                    break;
                case ID_TRAY_LOCKPOS:
                    g_locked = !g_locked;
                    break;
                case ID_TRAY_TOPMOST:    // 总在最前
                    g_topmost = !g_topmost;
                    ApplyTopmost();
                    break;
                case ID_TRAY_COLOR: {
                    COLORREF newColor = g_bgColor;
                    if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_COLOR_DLG),
                                       hWnd, ColorDlgProc, (LPARAM)&newColor) == IDOK) {
                        g_bgColor = newColor;
                        UpdateBackgroundBrush();
                    }
                    break;
                }
                case ID_TRAY_EXIT:
                    DestroyWindow(hWnd);
                    break;
            }
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// 设置高 DPI 感知
void SetDPIAwareness() {
    HMODULE hUser32 = GetModuleHandle(TEXT("user32.dll"));
    if (hUser32) {
        typedef BOOL (WINAPI *SetProcessDpiAwarenessContext_t)(DPI_AWARENESS_CONTEXT);
        auto pSetProcessDpiAwarenessContext =
            (SetProcessDpiAwarenessContext_t)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetProcessDpiAwarenessContext) {
            pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }
    SetProcessDPIAware();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    SetDPIAwareness();
    InitCommonControls();

    g_hInst = hInstance;
    g_hTrayMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_TRAYMENU));

    // 注册窗口类
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = TEXT("BlankWindowClass");
    RegisterClassEx(&wc);

    // 创建窗口，标题改为中文
    HWND hWnd = CreateWindowEx(
        0, TEXT("BlankWindowClass"), TEXT("空白窗口"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyMenu(g_hTrayMenu);
    return (int)msg.wParam;
}