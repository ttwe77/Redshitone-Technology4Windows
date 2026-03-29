#include <windows.h>
#include <commctrl.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "comctl32.lib")

#define IDC_AGE_EDIT 101
#define IDC_CALC_BUTTON 102
#define WM_DELAY_MESSAGE (WM_USER + 1)

HINSTANCE hInst;
HWND hEdit;
HWND hButton;

void SimulateDelay(HWND hwnd) {
    std::this_thread::sleep_for(std::chrono::seconds(30));
    PostMessage(hwnd, WM_DELAY_MESSAGE, 0, 0);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // 创建编辑框
            hEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                50, 50, 200, 30,
                hwnd,
                (HMENU)IDC_AGE_EDIT,
                hInst,
                NULL
            );
            
            // 设置编辑框提示文字
            SetWindowTextW(hEdit, L"");
            
            // 创建计算按钮
            hButton = CreateWindowW(
                L"BUTTON",
                L"Calculate",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                270, 50, 80, 30,
                hwnd,
                (HMENU)IDC_CALC_BUTTON,
                hInst,
                NULL
            );
            
            break;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_CALC_BUTTON) {
                // 获取编辑框文本长度
                int textLength = GetWindowTextLengthW(hEdit);
                
                if (textLength == 0) {
                    MessageBoxW(hwnd, L"Please enter your age", L"Error", MB_OK | MB_ICONERROR);
                    return 0;
                }
                
                // 创建新线程模拟延迟
                std::thread delayThread(SimulateDelay, hwnd);
                delayThread.detach();
                
                // 禁用界面元素，造成无响应效果
                EnableWindow(hEdit, FALSE);
                EnableWindow(hButton, FALSE);
            }
            break;
        }
        
        case WM_DELAY_MESSAGE: {
            // 30秒后显示消息
            MessageBoxW(hwnd, L"I don't know, learn when you grow up", L"Notice", MB_OK | MB_ICONINFORMATION);
            
            // 重新启用界面
            EnableWindow(hEdit, TRUE);
            EnableWindow(hButton, TRUE);
            
            break;
        }
        
        case WM_CLOSE: {
            DestroyWindow(hwnd);
            break;
        }
        
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    
    // 启用高DPI感知
    SetProcessDPIAware();
    
    // 注册窗口类
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"AgeCalculatorClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassExW(&wc);
    
    // 创建主窗口
    HWND hwnd = CreateWindowExW(
        0,
        L"AgeCalculatorClass",
        L"Age Calculator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (!hwnd) {
        return 1;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

