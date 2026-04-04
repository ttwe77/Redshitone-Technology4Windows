#include <windows.h>
#include <string>

#define ID_EDIT   1001
#define ID_BUTTON 1002
#define FAIL_LIMIT 3

HWND g_hEdit = nullptr;
WNDPROC g_oldEditProc = nullptr;
int g_failCount = 0;
bool g_isReplacing = false;   // 防止递归替换

LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CHAR:
		if (wParam == L'2')
		{
			SendMessageW(hwnd, EM_REPLACESEL, TRUE, (LPARAM)L"3");
			return 0;
		}
		else if (wParam == L'二')
		{
			SendMessageW(hwnd, EM_REPLACESEL, TRUE, (LPARAM)L"三");
			return 0;
		}
		break;
	}
	return CallWindowProcW(g_oldEditProc, hwnd, msg, wParam, lParam);
}

void ShowHint(HWND hwnd)
{
	MessageBoxW(hwnd, L"验证失败次数过多，请稍后再试。（提示：可以用其它语言或其它中文形式）", L"提示", MB_OK | MB_ICONWARNING);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CTLCOLORSTATIC:
	{
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}
		
	
	case WM_CREATE:
	{
		CreateWindowW(L"STATIC", L"请回答问题证明你不是机器人：1+1 = ？",
					  WS_CHILD | WS_VISIBLE,
					  20, 20, 300, 25,
					  hwnd, nullptr, nullptr, nullptr);
		
		g_hEdit = CreateWindowW(L"EDIT", L"",
								WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
								20, 55, 240, 28,
								hwnd, (HMENU)ID_EDIT, nullptr, nullptr);
		
		CreateWindowW(L"BUTTON", L"验证",
					  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					  270, 55, 80, 28,
					  hwnd, (HMENU)ID_BUTTON, nullptr, nullptr);
		
		g_oldEditProc = (WNDPROC)SetWindowLongPtrW(g_hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
		break;
	}
		
	case WM_COMMAND:
		{
			// 处理编辑框内容改变（实时替换 "two" -> "three"）
			if (LOWORD(wParam) == ID_EDIT && HIWORD(wParam) == EN_CHANGE && !g_isReplacing)
			{
				wchar_t buf[256] = {};
				GetWindowTextW(g_hEdit, buf, 255);
				std::wstring text(buf);
				// 新增功能：替换全角数字 "２" 为半角 "3"
				size_t posFullWidth = text.find(L"２"); // 全角数字2
				if (posFullWidth != std::wstring::npos)
				{
					g_isReplacing = true;
					text.replace(posFullWidth, 1, L"3"); // 替换1个字符
					SetWindowTextW(g_hEdit, text.c_str());
					// 将光标放到替换后的末尾
					int newPos = posFullWidth + 1; // "3"长度为1
					SendMessageW(g_hEdit, EM_SETSEL, newPos, newPos);
					g_isReplacing = false;
				}
				// 替换日语 "に" 为 "さん"
				size_t posJapanese = text.find(L"に");
				if (posJapanese != std::wstring::npos)
				{
					g_isReplacing = true;
					text.replace(posJapanese, 1, L"さん");
					SetWindowTextW(g_hEdit, text.c_str());
					// 将光标放到替换后的末尾
					int newPosJapanese = posJapanese + 2; // "さん"长度2（日文字符占一个位置但代码点计数为2？注意：实际中日文字符是2个wchar_t）
					SendMessageW(g_hEdit, EM_SETSEL, newPosJapanese, newPosJapanese);
					g_isReplacing = false;
				}
				size_t pos = text.find(L"two");
				if (pos != std::wstring::npos)
				{
					g_isReplacing = true;
					text.replace(pos, 3, L"three");
					SetWindowTextW(g_hEdit, text.c_str());
					// 将光标放到替换后的末尾
					int newPos = pos + 5; // "three"长度5
					SendMessageW(g_hEdit, EM_SETSEL, newPos, newPos);
					g_isReplacing = false;
				}
			}
			else if (LOWORD(wParam) == ID_BUTTON)
			{
				wchar_t buf[256] = {};
				GetWindowTextW(g_hEdit, buf, 255);
				std::wstring input(buf);
				
				if (input == L"贰")
				{
					MessageBoxW(hwnd, L"验证通过。", L"结果", MB_OK | MB_ICONINFORMATION);
					g_failCount = 0;
				}
				else
				{
					g_failCount++;
					MessageBoxW(hwnd, L"答案不正确。", L"结果", MB_OK | MB_ICONERROR);
					SetWindowTextW(g_hEdit, L"");
					if (g_failCount >= FAIL_LIMIT)
					{
						ShowHint(hwnd);
						g_failCount = 0;
					}
				}
			}
			break;
		}
		
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	const wchar_t szClassName[] = L"CaptchaWindowClass";
	
	WNDCLASSW wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = szClassName;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	
	if (!RegisterClassW(&wc))
	{
		MessageBoxW(nullptr, L"窗口类注册失败。", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}
	
	HWND hwnd = CreateWindowW(
							  szClassName,
							  L"人机验证",
							  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
							  CW_USEDEFAULT, CW_USEDEFAULT, 390, 150,
							  nullptr, nullptr, hInstance, nullptr
							  );
	
	if (!hwnd)
	{
		MessageBoxW(nullptr, L"窗口创建失败。", L"错误", MB_OK | MB_ICONERROR);
		return 0;
	}
	
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	
	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	
	return 0;
}
