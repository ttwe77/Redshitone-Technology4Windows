#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON  (WM_APP + 1)
#define ID_TRAY_EXIT 1001

NOTIFYICONDATAW nid = {};
HMENU hTrayMenu = NULL;

// 显示托盘右键菜单
void ShowTrayMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hTrayMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
}

// 添加系统托盘图标
void AddTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1));
    lstrcpyW(nid.szTip, L"阻止关机 - 运行中");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

// 移除系统托盘图标
void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            hTrayMenu = CreatePopupMenu();
            AppendMenuW(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"退出");
            AddTrayIcon(hwnd);

            // 设置阻止关机原因（系统会向用户显示此信息）
            ShutdownBlockReasonCreate(hwnd, L"阻止关机程序正在运行，请退出后再关机");
            break;
        }
        case WM_TRAYICON: {
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            }
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                RemoveTrayIcon();
                DestroyWindow(hwnd);
            }
            break;
        }
        case WM_QUERYENDSESSION: {
            // 阻止关机：返回 FALSE
            return FALSE;
        }
        case WM_DESTROY: {
            RemoveTrayIcon();
            // 清除阻止关机原因
            ShutdownBlockReasonDestroy(hwnd);
            if (hTrayMenu) DestroyMenu(hTrayMenu);
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // DPI 感知
    SetProcessDPIAware();

    // 单实例检查
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Global\\ShutdownBlocker_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        MessageBoxW(NULL, L"阻止关机程序已在运行中，请查看系统托盘。",
                    L"阻止关机", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // 注册窗口类
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.lpszClassName = L"ShutdownBlockerClass";

    if (!RegisterClassW(&wc)) {
        return 1;
    }

    // 创建隐藏窗口（用于接收 WM_QUERYENDSESSION 和托盘消息）
    HWND hwnd = CreateWindowExW(
        0, L"ShutdownBlockerClass", L"ShutdownBlocker",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        return 1;
    }

    // 隐藏窗口，只显示托盘图标
    ShowWindow(hwnd, SW_HIDE);

    // 消息循环
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}