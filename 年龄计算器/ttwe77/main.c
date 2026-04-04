#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// 控件ID
#define IDC_TITLE          1000
#define IDC_EDIT_AGE       1001
#define IDC_BTN_SUBMIT     1002
#define IDC_LBL_STATUS     1003
#define IDC_LBL_PROMPT     1004

// 全局变量
HWND g_hMainWnd = NULL;
HWND g_hTitle = NULL;
HWND g_hPromptLabel = NULL;
HWND g_hEdit = NULL;
HWND g_hButton = NULL;
HWND g_hStatusLabel = NULL;
HFONT g_hTitleFont = NULL;

// 函数声明
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL IsIntegerString(const char* str);
void LayoutControls(HWND hWnd);
void ShowMessageBoxWide(HWND hWnd, const wchar_t* text, const wchar_t* caption, UINT type);
void CenterMainWindow(HWND hWnd, int width, int height);

// ========================
// 入口函数 - 高DPI适配
// ========================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// 启用高DPI感知
	SetProcessDPIAware();
	
	// 注册窗口类
	WNDCLASSEXA wc = {0};
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = "AgeCalculatorClass";
	
	if (!RegisterClassExA(&wc)) {
		MessageBoxA(NULL, "窗口类注册失败！", "错误", MB_ICONERROR);
		return 1;
	}
	
	// 创建主窗口
	g_hMainWnd = CreateWindowExA(
								 0,
								 "AgeCalculatorClass",
								 "年龄计算器",
								 WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
								 CW_USEDEFAULT, CW_USEDEFAULT,
								 540, 380,
								 NULL, NULL, hInstance, NULL
								 );
	
	if (!g_hMainWnd) {
		MessageBoxA(NULL, "窗口创建失败！", "错误", MB_ICONERROR);
		return 1;
	}
	
	CenterMainWindow(g_hMainWnd, 540, 380);
	
	ShowWindow(g_hMainWnd, nCmdShow);
	UpdateWindow(g_hMainWnd);
	
	// 消息循环
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return (int)msg.wParam;
}

// ========================
// 工具函数
// ========================
BOOL IsIntegerString(const char* str) {
	if (str == NULL || *str == '\0') return FALSE;
	for (int i = 0; str[i]; i++) {
		if (!isdigit((unsigned char)str[i])) return FALSE;
	}
	return TRUE;
}

void ShowMessageBoxWide(HWND hWnd, const wchar_t* text, const wchar_t* caption, UINT type) {
	MessageBoxW(hWnd, text, caption, type);
}

void CenterMainWindow(HWND hWnd, int width, int height) {
	int screenW = GetSystemMetrics(SM_CXSCREEN);
	int screenH = GetSystemMetrics(SM_CYSCREEN);
	int x = (screenW - width) / 2;
	int y = (screenH - height) / 2;
	SetWindowPos(hWnd, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

// ========================
// 布局函数：使所有控件居中
// ========================
void LayoutControls(HWND hWnd) {
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);
	int clientWidth = rcClient.right - rcClient.left;
	int clientHeight = rcClient.bottom - rcClient.top;
	
	// 1. 大标题
	if (g_hTitle) {
		int titleWidth = clientWidth - 40;
		int titleHeight = 54;
		int titleX = (clientWidth - titleWidth) / 2;
		int titleY = 18;
		SetWindowPos(g_hTitle, NULL, titleX, titleY, titleWidth, titleHeight, SWP_NOZORDER);
	}
	
	// 2. 提示标签
	if (g_hPromptLabel) {
		int labelWidth = 260;
		int labelHeight = 24;
		int labelX = (clientWidth - labelWidth) / 2;
		int labelY = 92;
		SetWindowPos(g_hPromptLabel, NULL, labelX, labelY, labelWidth, labelHeight, SWP_NOZORDER);
	}
	
	// 3. 年龄编辑框
	if (g_hEdit) {
		int editWidth = 240;
		int editHeight = 28;
		int editX = (clientWidth - editWidth) / 2;
		int editY = 126;
		SetWindowPos(g_hEdit, NULL, editX, editY, editWidth, editHeight, SWP_NOZORDER);
	}
	
	// 4. 提交按钮
	if (g_hButton) {
		int btnWidth = 110;
		int btnHeight = 34;
		int btnX = (clientWidth - btnWidth) / 2;
		int btnY = 172;
		SetWindowPos(g_hButton, NULL, btnX, btnY, btnWidth, btnHeight, SWP_NOZORDER);
	}
	
	// 5. 状态标签
	if (g_hStatusLabel) {
		int statusWidth = 260;
		int statusHeight = 24;
		int statusX = (clientWidth - statusWidth) / 2;
		int statusY = 220;
		SetWindowPos(g_hStatusLabel, NULL, statusX, statusY, statusWidth, statusHeight, SWP_NOZORDER);
	}
}

// ========================
// 窗口过程
// ========================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_CREATE: {
		// 创建更大的标题字体
		g_hTitleFont = CreateFontA(
								   -34, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
								   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
								   CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
								   DEFAULT_PITCH | FF_DONTCARE, "Microsoft YaHei"
								   );
		
		// 创建大标题控件
		g_hTitle = CreateWindowExA(
								   0, "STATIC", "年龄计算器",
								   WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | SS_NOPREFIX,
								   0, 0, 0, 0,
								   hWnd, (HMENU)IDC_TITLE, GetModuleHandle(NULL), NULL
								   );
		if (g_hTitle && g_hTitleFont) {
			SendMessage(g_hTitle, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);
		}
		
		// 创建提示标签
		g_hPromptLabel = CreateWindowExA(
										 0, "STATIC", "请输入您的年龄：",
										 WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | SS_NOPREFIX,
										 0, 0, 0, 0,
										 hWnd, (HMENU)IDC_LBL_PROMPT, GetModuleHandle(NULL), NULL
										 );
		
		// 创建编辑框
		g_hEdit = CreateWindowExA(
								  WS_EX_CLIENTEDGE, "EDIT", "",
								  WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_NUMBER,
								  0, 0, 0, 0,
								  hWnd, (HMENU)IDC_EDIT_AGE, GetModuleHandle(NULL), NULL
								  );
		
		// 创建提交按钮
		g_hButton = CreateWindowExA(
									0, "BUTTON", "提交",
									WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
									0, 0, 0, 0,
									hWnd, (HMENU)IDC_BTN_SUBMIT, GetModuleHandle(NULL), NULL
									);
		
		// 创建状态标签，默认隐藏
		g_hStatusLabel = CreateWindowExA(
										 0, "STATIC", "",
										 WS_CHILD | SS_CENTER | SS_CENTERIMAGE | SS_NOPREFIX,
										 0, 0, 0, 0,
										 hWnd, (HMENU)IDC_LBL_STATUS, GetModuleHandle(NULL), NULL
										 );
		ShowWindow(g_hStatusLabel, SW_HIDE);
		
		// 初始布局
		LayoutControls(hWnd);
		
		// 默认焦点放到输入框
		SetFocus(g_hEdit);
		break;
	}
		
		case WM_SIZE: {
			LayoutControls(hWnd);
			break;
		}
		
		case WM_CTLCOLORSTATIC: {
			HDC hdc = (HDC)wParam;
			HWND hCtrl = (HWND)lParam;
			
			if (hCtrl == g_hTitle || hCtrl == g_hPromptLabel || hCtrl == g_hStatusLabel) {
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, RGB(25, 25, 25));
				return (INT_PTR)GetStockObject(NULL_BRUSH);
			}
			break;
		}
		
		case WM_ERASEBKGND: {
			HDC hdc = (HDC)wParam;
			RECT rc;
			GetClientRect(hWnd, &rc);
			FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
			return 1;
		}
		
		case WM_COMMAND: {
			if (LOWORD(wParam) == IDC_BTN_SUBMIT && HIWORD(wParam) == BN_CLICKED) {
				// 获取并验证年龄
				char ageText[32] = {0};
				GetWindowTextA(g_hEdit, ageText, sizeof(ageText));
				
				if (strlen(ageText) == 0) {
					MessageBoxA(hWnd, "请先输入您的年龄！", "提示", MB_OK | MB_ICONWARNING);
					SetFocus(g_hEdit);
					break;
				}
				if (!IsIntegerString(ageText)) {
					MessageBoxA(hWnd, "请输入有效的年龄数字！", "提示", MB_OK | MB_ICONWARNING);
					SetFocus(g_hEdit);
					break;
				}
				
				// 禁用输入控件，防止重复点击
				EnableWindow(g_hEdit, FALSE);
				EnableWindow(g_hButton, FALSE);
				
				// 显示状态
				if (g_hStatusLabel) {
					SetWindowTextA(g_hStatusLabel, "正在处理中...");
					ShowWindow(g_hStatusLabel, SW_SHOW);
				}
				
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
				
				// 阻塞式 Sleep()
				Sleep(1500);
				
				// 处理完成
				MessageBeep(MB_ICONERROR);
				ShowMessageBoxWide(hWnd, L"我不会，长大后在学", L"提示", MB_OK | MB_ICONERROR);
				
				// 恢复输入，支持多次计算
				EnableWindow(g_hEdit, TRUE);
				EnableWindow(g_hButton, TRUE);
				
				// 清空输入，重新开始
				SetWindowTextA(g_hEdit, "");
				if (g_hStatusLabel) {
					SetWindowTextA(g_hStatusLabel, "处理完成");
					ShowWindow(g_hStatusLabel, SW_SHOW);
				}
				
				LayoutControls(hWnd);
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
				
				SetFocus(g_hEdit);
				SendMessage(g_hEdit, EM_SETSEL, 0, -1);
			}
			break;
		}
		
		case WM_DESTROY: {
			if (g_hTitleFont) {
				DeleteObject(g_hTitleFont);
				g_hTitleFont = NULL;
			}
			PostQuitMessage(0);
			break;
		}
		
	default:
		return DefWindowProcA(hWnd, message, wParam, lParam);
	}
	return 0;
}
