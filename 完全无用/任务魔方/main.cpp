// main.cpp (任务魔方)
// 编译: g++ -o 任务魔方.exe main.cpp -mwindows -lole32 -lcomctl32 -luuid -lgdi32 -std=c++17

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <objbase.h>
#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "gdi32.lib")

// ---------- 补充旧版 MinGW 缺少的 DPI 类型定义 ----------
#ifndef PROCESS_DPI_AWARENESS
typedef enum PROCESS_DPI_AWARENESS {
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
#endif

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

// ----------------------------- 全局变量 -----------------------------
HINSTANCE g_hInst;
HWND g_hWnd = nullptr;
ITaskbarList3* g_pTaskbarList = nullptr;
BOOL g_bComInitialized = FALSE;

enum {
    IDC_BTN_NONE = 101,
    IDC_BTN_NORMAL,
    IDC_BTN_PAUSED,
    IDC_BTN_ERROR,
    IDC_BTN_INDETERMINATE,
    IDC_SLIDER_PROGRESS,
    IDC_STATIC_PROGRESS,
    // 新增控件 ID
    IDC_EDIT_DELAY = 201,
    IDC_BTN_FLASH,
    IDC_EDIT_INTERVAL,      // 动态进度间隔(毫秒)
    IDC_BTN_ANIMATE,        // 开始/停止动画
};

#define TIMER_FLASH_DELAY   1000   // 闪烁延迟定时器ID
#define TIMER_ANIMATE       1001   // 动画定时器ID

int g_nProgress = 50;
TBPFLAG g_currentTaskbarFlag = TBPF_NOPROGRESS;

// 动画控制
BOOL g_bAnimating = FALSE;
int  g_nAnimDir = 1;        // 1: 递增, -1: 递减

// ----------------------------- DPI 缩放 -----------------------------
int GetSystemScaleFactor() {
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(nullptr, hdc);
    return dpi;
}
int ScaleX(int x) { return MulDiv(x, GetSystemScaleFactor(), 96); }
int ScaleY(int y) { return MulDiv(y, GetSystemScaleFactor(), 96); }

// ----------------------------- 高 DPI 初始化 -----------------------------
void InitDPIAwareness() {
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    if (hUser32) {
        using SetProcessDpiAwarenessContext_t = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
        auto pSetProcessDpiAwarenessContext = (SetProcessDpiAwarenessContext_t)
            GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetProcessDpiAwarenessContext) {
            pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }
    HMODULE hShcore = LoadLibrary(L"shcore.dll");
    if (hShcore) {
        using SetProcessDpiAwareness_t = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
        auto pSetProcessDpiAwareness = (SetProcessDpiAwareness_t)
            GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (pSetProcessDpiAwareness) {
            pSetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
            FreeLibrary(hShcore);
            return;
        }
        FreeLibrary(hShcore);
    }
    SetProcessDPIAware();
}

// ----------------------------- 任务栏控制 -----------------------------
void SetTaskbarState(TBPFLAG flag) {
    if (g_pTaskbarList && g_hWnd)
        g_pTaskbarList->SetProgressState(g_hWnd, flag);
}

void SetTaskbarValue(ULONGLONG completed, ULONGLONG total) {
    if (g_pTaskbarList && g_hWnd)
        g_pTaskbarList->SetProgressValue(g_hWnd, completed, total);
}

void UpdateTaskbar(TBPFLAG flag) {
    g_currentTaskbarFlag = flag;
    if (flag == TBPF_INDETERMINATE) {
        SetTaskbarState(TBPF_INDETERMINATE);
    }
    else if (flag == TBPF_NOPROGRESS) {
        SetTaskbarState(TBPF_NOPROGRESS);
    }
    else {
        SetTaskbarState(flag);
        SetTaskbarValue(g_nProgress, 100);
    }
}

// 更新进度显示（滑块+文本+任务栏）
void UpdateProgressDisplay(HWND hWnd, int progress) {
    g_nProgress = progress;
    // 更新滑块
    HWND hSlider = GetDlgItem(hWnd, IDC_SLIDER_PROGRESS);
    SendMessage(hSlider, TBM_SETPOS, TRUE, progress);
    // 更新文本
    std::wstringstream wss;
    wss << progress << L"%";
    SetDlgItemText(hWnd, IDC_STATIC_PROGRESS, wss.str().c_str());
    // 更新任务栏
    if (g_currentTaskbarFlag == TBPF_NORMAL ||
        g_currentTaskbarFlag == TBPF_PAUSED ||
        g_currentTaskbarFlag == TBPF_ERROR) {
        SetTaskbarValue(progress, 100);
    }
}

// 停止动画
void StopAnimation(HWND hWnd) {
    if (g_bAnimating) {
        KillTimer(hWnd, TIMER_ANIMATE);
        g_bAnimating = FALSE;
        SetDlgItemText(hWnd, IDC_BTN_ANIMATE, L"开始动态");
    }
}

// 开始动画
void StartAnimation(HWND hWnd) {
    if (g_bAnimating) return;
    // 读取间隔
    wchar_t buf[16] = {0};
    GetDlgItemText(hWnd, IDC_EDIT_INTERVAL, buf, 16);
    int interval = _wtoi(buf);
    if (interval <= 0) interval = 100;       // 默认100ms
    if (interval > 10000) interval = 10000;  // 上限10秒

    // 确保方向合理
    if (g_nProgress >= 100) g_nAnimDir = -1;
    else if (g_nProgress <= 0) g_nAnimDir = 1;

    SetTimer(hWnd, TIMER_ANIMATE, interval, nullptr);
    g_bAnimating = TRUE;
    SetDlgItemText(hWnd, IDC_BTN_ANIMATE, L"停止动态");
}

// ----------------------------- 窗口过程 -----------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CTLCOLORSTATIC:
    {
        // 将静态文本背景设为白色，避免灰色背景
        HDC hdcStatic = (HDC)wParam;
        SetBkColor(hdcStatic, RGB(255, 255, 255));
        SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }

    case WM_CREATE: {
        g_hWnd = hWnd;
        if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
            g_bComInitialized = TRUE;

        if (FAILED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_ITaskbarList3, (void**)&g_pTaskbarList))) {
            MessageBox(hWnd, L"无法获取 ITaskbarList3 接口", L"错误", MB_ICONERROR);
            return -1;
        }
        g_pTaskbarList->HrInit();

        HINSTANCE hInst = g_hInst;
        int margin = ScaleX(20);
        int xLeft = margin;
        int yPos = margin;
        int groupWidth = ScaleX(300);  // 控件区宽度
        int btnWidth = ScaleX(160);
        int btnHeight = ScaleY(38);
        int gap = ScaleY(10);

        // ---- 1. 状态按钮组 ----
        CreateWindow(L"BUTTON", L"任务栏状态", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                     xLeft, yPos, groupWidth, ScaleY(250), hWnd, nullptr, hInst, nullptr);
        yPos += ScaleY(25);

        CreateWindow(L"BUTTON", L"无进度 (隐藏)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     xLeft + ScaleX(15), yPos, btnWidth, btnHeight, hWnd, (HMENU)IDC_BTN_NONE, hInst, nullptr);
        yPos += btnHeight + gap;

        CreateWindow(L"BUTTON", L"绿色正常", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     xLeft + ScaleX(15), yPos, btnWidth, btnHeight, hWnd, (HMENU)IDC_BTN_NORMAL, hInst, nullptr);
        yPos += btnHeight + gap;

        CreateWindow(L"BUTTON", L"黄色暂停", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     xLeft + ScaleX(15), yPos, btnWidth, btnHeight, hWnd, (HMENU)IDC_BTN_PAUSED, hInst, nullptr);
        yPos += btnHeight + gap;

        CreateWindow(L"BUTTON", L"红色错误", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     xLeft + ScaleX(15), yPos, btnWidth, btnHeight, hWnd, (HMENU)IDC_BTN_ERROR, hInst, nullptr);
        yPos += btnHeight + gap;

        CreateWindow(L"BUTTON", L"不确定 (脉冲)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     xLeft + ScaleX(15), yPos, btnWidth, btnHeight, hWnd, (HMENU)IDC_BTN_INDETERMINATE, hInst, nullptr);
        yPos += btnHeight + ScaleY(20);

        // ---- 2. 进度控制组 ----
        xLeft = margin;
        int yGroup2 = yPos;
        CreateWindow(L"BUTTON", L"进度控制", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                     xLeft, yPos, groupWidth, ScaleY(140), hWnd, nullptr, hInst, nullptr);
        yPos += ScaleY(25);

        // 滑块标签
        CreateWindow(L"STATIC", L"手动进度:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                     xLeft + ScaleX(15), yPos, ScaleX(80), ScaleY(22), hWnd, nullptr, hInst, nullptr);
        // 滑块
        HWND hSlider = CreateWindow(TRACKBAR_CLASS, nullptr,
                                    WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_BOTTOM,
                                    xLeft + ScaleX(100), yPos - ScaleY(2),
                                    ScaleX(180), ScaleY(30), hWnd, (HMENU)IDC_SLIDER_PROGRESS, hInst, nullptr);
        SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessage(hSlider, TBM_SETPOS, TRUE, g_nProgress);
        yPos += ScaleY(30);

        // 百分比显示
        CreateWindow(L"STATIC", L"50%", WS_CHILD | WS_VISIBLE | SS_CENTER,
                     xLeft + ScaleX(100), yPos, ScaleX(180), ScaleY(22),
                     hWnd, (HMENU)IDC_STATIC_PROGRESS, hInst, nullptr);
        yPos += ScaleY(30);

        // ---- 3. 动态进度动画组 ----
        xLeft = margin;
        int yGroup3 = yPos;
        CreateWindow(L"BUTTON", L"动态进度动画", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                     xLeft, yPos, groupWidth, ScaleY(85), hWnd, nullptr, hInst, nullptr);
        yPos += ScaleY(25);

        CreateWindow(L"STATIC", L"间隔(ms):", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                     xLeft + ScaleX(15), yPos + ScaleY(4), ScaleX(70), ScaleY(22), hWnd, nullptr, hInst, nullptr);
        CreateWindow(L"EDIT", L"100", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                     xLeft + ScaleX(95), yPos, ScaleX(70), ScaleY(24), hWnd, (HMENU)IDC_EDIT_INTERVAL, hInst, nullptr);
        CreateWindow(L"BUTTON", L"开始动态", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     xLeft + ScaleX(180), yPos - ScaleY(2), ScaleX(100), btnHeight,
                     hWnd, (HMENU)IDC_BTN_ANIMATE, hInst, nullptr);
        yPos += ScaleY(60);  // 组结束

        // ---- 4. 窗口闪烁组 ----
        xLeft = margin;
        int yGroup4 = yPos;
        CreateWindow(L"BUTTON", L"窗口闪烁", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                     xLeft, yPos, groupWidth, ScaleY(85), hWnd, nullptr, hInst, nullptr);
        yPos += ScaleY(25);

        CreateWindow(L"STATIC", L"延迟(秒):", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                     xLeft + ScaleX(15), yPos + ScaleY(4), ScaleX(70), ScaleY(22), hWnd, nullptr, hInst, nullptr);
        CreateWindow(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                     xLeft + ScaleX(95), yPos, ScaleX(70), ScaleY(24), hWnd, (HMENU)IDC_EDIT_DELAY, hInst, nullptr);
        CreateWindow(L"BUTTON", L"延迟闪烁", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     xLeft + ScaleX(180), yPos - ScaleY(2), ScaleX(100), btnHeight,
                     hWnd, (HMENU)IDC_BTN_FLASH, hInst, nullptr);

        UpdateTaskbar(TBPF_NOPROGRESS);
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDC_BTN_NONE:          UpdateTaskbar(TBPF_NOPROGRESS); break;
        case IDC_BTN_NORMAL:        UpdateTaskbar(TBPF_NORMAL); break;
        case IDC_BTN_PAUSED:        UpdateTaskbar(TBPF_PAUSED); break;
        case IDC_BTN_ERROR:         UpdateTaskbar(TBPF_ERROR); break;
        case IDC_BTN_INDETERMINATE: UpdateTaskbar(TBPF_INDETERMINATE); break;

        case IDC_BTN_ANIMATE: {
            if (g_bAnimating) {
                StopAnimation(hWnd);
            } else {
                StartAnimation(hWnd);
            }
            break;
        }

        case IDC_BTN_FLASH: {
            wchar_t buffer[16] = {0};
            GetDlgItemText(hWnd, IDC_EDIT_DELAY, buffer, 16);
            int delaySeconds = _wtoi(buffer);
            if (delaySeconds <= 0) delaySeconds = 1;
            if (delaySeconds > 60) delaySeconds = 60;

            KillTimer(hWnd, TIMER_FLASH_DELAY);
            SetTimer(hWnd, TIMER_FLASH_DELAY, delaySeconds * 1000, nullptr);
            break;
        }
        }
        return 0;
    }

    case WM_HSCROLL: {
        HWND hSlider = (HWND)lParam;
        if (hSlider == GetDlgItem(hWnd, IDC_SLIDER_PROGRESS)) {
            int newProgress = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
            // 用户手动拖动时停止动画
            StopAnimation(hWnd);
            UpdateProgressDisplay(hWnd, newProgress);
        }
        return 0;
    }

    case WM_TIMER: {
        if (wParam == TIMER_FLASH_DELAY) {
            KillTimer(hWnd, TIMER_FLASH_DELAY);

            FLASHWINFO fwi = {0};
            fwi.cbSize = sizeof(FLASHWINFO);
            fwi.hwnd = hWnd;
            fwi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
            fwi.uCount = 5;
            fwi.dwTimeout = 0;
            FlashWindowEx(&fwi);
        }
        else if (wParam == TIMER_ANIMATE) {
            // 动态调整进度
            g_nProgress += g_nAnimDir;
            if (g_nProgress >= 100) {
                g_nProgress = 100;
                g_nAnimDir = -1;
            }
            else if (g_nProgress <= 0) {
                g_nProgress = 0;
                g_nAnimDir = 1;
            }
            UpdateProgressDisplay(hWnd, g_nProgress);
        }
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_FLASH_DELAY);
        KillTimer(hWnd, TIMER_ANIMATE);
        if (g_pTaskbarList) {
            g_pTaskbarList->SetProgressState(hWnd, TBPF_NOPROGRESS);
            g_pTaskbarList->Release();
            g_pTaskbarList = nullptr;
        }
        if (g_bComInitialized)
            CoUninitialize();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ----------------------------- 入口 -----------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow) {
    g_hInst = hInstance;
    InitDPIAwareness();

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    const wchar_t CLASS_NAME[] = L"TaskbarColorDemo";
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  // 白色背景
    RegisterClass(&wc);

    RECT rcWork;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
    int screenW = rcWork.right - rcWork.left;
    int screenH = rcWork.bottom - rcWork.top;
    int winW = ScaleX(400);   // 固定合适宽度
    int winH = ScaleY(620);
    int winX = rcWork.left + (screenW - winW) / 2;
    int winY = rcWork.top + (screenH - winH) / 2;

    HWND hWnd = CreateWindowEx(0, CLASS_NAME, L"任务魔方 - 任务栏颜色演示 + 动态进度",
                               WS_OVERLAPPEDWINDOW, winX, winY, winW, winH,
                               nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}