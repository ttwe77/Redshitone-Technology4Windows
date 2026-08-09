// lyzk.cpp — 领域展开 (Domain Expansion)
// 编译: g++ -o lyzk.exe lyzk.cpp -lgdi32 -lgdiplus -luser32 -static -mwindows -O2 -s

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellscalingapi.h>
#include <gdiplus.h>
#include <string>
#include <stdexcept>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shcore.lib")

using namespace Gdiplus;

// ========== 全局状态 ==========
static HINSTANCE g_hInstance = nullptr;
static HDESK     g_hOriginal  = nullptr;
static HDESK     g_hNew       = nullptr;
static int       g_ScreenW    = 0;
static int       g_ScreenH    = 0;
static HWND      g_hWnd       = nullptr;

// 壁纸
static Image*    g_pWallpaper = nullptr;
static bool      g_hasWallpaper = false;

// ========== DPI 感知 ==========
void SetDpiAwareness()
{
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore)
    {
        typedef HRESULT (WINAPI *SetProcessDpiAwareness_t)(int);
        auto pSetProcessDpiAwareness = (SetProcessDpiAwareness_t)
            GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (pSetProcessDpiAwareness)
        {
            // PROCESS_PER_MONITOR_DPI_AWARE = 2
            HRESULT hr = pSetProcessDpiAwareness(2);
            FreeLibrary(hShcore);
            if (SUCCEEDED(hr))
                return;
        }
        else
        {
            FreeLibrary(hShcore);
        }
    }
    // 回退方案
    SetProcessDPIAware();
}

// ========== 获取壁纸路径 ==========
std::wstring GetWallpaperPath()
{
    WCHAR buf[MAX_PATH] = {0};
    if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, buf, 0))
    {
        if (GetFileAttributesW(buf) != INVALID_FILE_ATTRIBUTES)
            return buf;
    }
    return L"";
}

// ========== 窗口过程 ==========
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        if (g_hasWallpaper && g_pWallpaper)
        {
            Graphics graphics(hdc);
            graphics.DrawImage(g_pWallpaper, 0, 0, rc.right, rc.bottom);
        }
        else
        {
            // 纯黑背景 + 中二文字
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);

            SetBkMode(hdc, TRANSPARENT);

            // "✦ 领域展开 ✦"
            HFONT hFontBig = CreateFontW(
                48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
            SelectObject(hdc, hFontBig);
            SetTextColor(hdc, RGB(255, 255, 255));

            RECT rcText = rc;
            rcText.bottom -= 40;
            DrawTextW(hdc, L"\u2726 领域展开 \u2726", -1, &rcText,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            DeleteObject(hFontBig);

            // "——世界将被我的结界笼罩——"
            HFONT hFontSmall = CreateFontW(
                24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
            SelectObject(hdc, hFontSmall);
            SetTextColor(hdc, RGB(128, 128, 128));

            RECT rcSub = rc;
            rcSub.top += 40;
            DrawTextW(hdc, L"\u2014\u2014世界将被我的结界笼罩\u2014\u2014", -1, &rcSub,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            DeleteObject(hFontSmall);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ========== 创建全屏窗口 ==========
HWND CreateFullscreenWindow(HINSTANCE hInstance)
{
    const wchar_t CLASS_NAME[] = L"DomainExpansionWnd";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    g_ScreenW = GetSystemMetrics(SM_CXSCREEN);
    g_ScreenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST,          // 置顶
        CLASS_NAME,
        L"Domain Expansion",
        WS_POPUP,               // 无边框
        0, 0, g_ScreenW, g_ScreenH,
        nullptr, nullptr, hInstance, nullptr
    );

    return hWnd;
}

// ========== 创建安全桌面 ==========
void CreateSecureDesktop()
{
    // 获取当前桌面
    g_hOriginal = OpenInputDesktop(0, FALSE, DESKTOP_SWITCHDESKTOP);
    if (!g_hOriginal)
    {
        DWORD tid = GetCurrentThreadId();
        g_hOriginal = GetThreadDesktop(tid);
    }
    if (!g_hOriginal)
        throw std::runtime_error("无法获取当前桌面句柄");

    // 创建新桌面
    g_hNew = CreateDesktopW(L"DomainExpansion", nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    if (!g_hNew)
        throw std::runtime_error("创建桌面失败");

    if (!SetThreadDesktop(g_hNew))
    {
        DWORD err = GetLastError();
        CloseDesktop(g_hNew);
        g_hNew = nullptr;
        throw std::runtime_error("SetThreadDesktop 失败");
    }

    if (!SwitchDesktop(g_hNew))
    {
        DWORD err = GetLastError();
        SetThreadDesktop(g_hOriginal);
        CloseDesktop(g_hNew);
        g_hNew = nullptr;
        throw std::runtime_error("SwitchDesktop 失败");
    }
}

// ========== 恢复桌面 ==========
void RestoreDesktop()
{
    if (g_hOriginal)
    {
        SetThreadDesktop(g_hOriginal);
        SwitchDesktop(g_hOriginal);
    }
    if (g_hNew)
    {
        CloseDesktop(g_hNew);
        g_hNew = nullptr;
    }
}

// ========== 加载壁纸 ==========
void LoadWallpaper()
{
    std::wstring path = GetWallpaperPath();
    if (!path.empty())
    {
        g_pWallpaper = Image::FromFile(path.c_str());
        if (g_pWallpaper && g_pWallpaper->GetLastStatus() == Ok)
        {
            g_hasWallpaper = true;
            return;
        }
        delete g_pWallpaper;
        g_pWallpaper = nullptr;
    }
    g_hasWallpaper = false;
}

// ========== 主入口 ==========
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    g_hInstance = hInstance;

    // 设置 DPI 感知
    SetDpiAwareness();

    // 初始化 GDI+
    ULONG_PTR gdiplusToken = 0;
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // 尝试创建安全桌面
    bool desktopCreated = false;
    try
    {
        CreateSecureDesktop();
        desktopCreated = true;
    }
    catch (const std::exception& e)
    {
        WCHAR buf[512];
        swprintf(buf, 512, L"无法创建领域: %hs", e.what());
        MessageBoxW(nullptr, buf, L"错误", MB_OK | MB_ICONWARNING);
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    // 加载壁纸
    LoadWallpaper();

    // 创建全屏窗口
    g_hWnd = CreateFullscreenWindow(hInstance);
    if (!g_hWnd)
    {
        RestoreDesktop();
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    // 强制捕获焦点
    SetForegroundWindow(g_hWnd);
    SetFocus(g_hWnd);

    // 弹窗
    MessageBoxW(g_hWnd,
        L"领域展开！\n在此刻，世界将被我的结界笼罩！",
        L"领域展开",
        MB_OK | MB_ICONINFORMATION);

    // 弹窗关闭后退出
    DestroyWindow(g_hWnd);

    // 消息循环（处理 WM_DESTROY 等）
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理
    delete g_pWallpaper;
    RestoreDesktop();
    GdiplusShutdown(gdiplusToken);

    return 0;
}