#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00

#include <windows.h>
#include <cmath>

static const wchar_t* kClassName = L"EscapeWindowClass";
static const wchar_t* kWindowTitle = L"进入软件";

static HWND   g_hwnd = nullptr;
static HWND   g_btnEnter = nullptr;
static HWND   g_lblInfo = nullptr;
static HFONT  g_font = nullptr;

static int    g_dpi = 96;
static int    g_clientW = 420;
static int    g_clientH = 220;

static double g_x = 100.0;
static double g_y = 100.0;
static bool   g_returning = false;
static double g_targetX = 0.0;
static double g_targetY = 0.0;

static int Scale(int v)
{
	return MulDiv(v, g_dpi, 96);
}

static LONG ClampLong(LONG v, LONG lo, LONG hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

static void GetWorkArea(HWND hwnd, RECT& rcWork)
{
	HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	GetMonitorInfoW(hMon, &mi);
	rcWork = mi.rcWork;
}

static void RebuildFont(HWND hwnd)
{
	if (g_font)
	{
		DeleteObject(g_font);
		g_font = nullptr;
	}
	
	g_dpi = GetDpiForWindow(hwnd);
	
	int fontSize = -MulDiv(16, g_dpi, 96);
	g_font = CreateFontW(
						 fontSize, 0, 0, 0, FW_NORMAL,
						 FALSE, FALSE, FALSE,
						 DEFAULT_CHARSET,
						 OUT_DEFAULT_PRECIS,
						 CLIP_DEFAULT_PRECIS,
						 CLEARTYPE_QUALITY,
						 DEFAULT_PITCH | FF_DONTCARE,
						 L"Microsoft YaHei UI"
						 );
	
	if (g_lblInfo)
		SendMessageW(g_lblInfo, WM_SETFONT, (WPARAM)g_font, TRUE);
	
	if (g_btnEnter)
		SendMessageW(g_btnEnter, WM_SETFONT, (WPARAM)g_font, TRUE);
}

static void LayoutControls(HWND hwnd)
{
	RECT rc{};
	GetClientRect(hwnd, &rc);
	
	int cw = rc.right - rc.left;
	int ch = rc.bottom - rc.top;
	
	int btnW = Scale(130);
	int btnH = Scale(38);
	int gap  = Scale(14);
	
	int lblH = Scale(28);
	
	int btnX = (cw - btnW) / 2;
	int btnY = ch - Scale(58);
	
	int lblX = Scale(18);
	int lblY = Scale(20);
	int lblW = cw - Scale(36);
	
	if (g_lblInfo)
		MoveWindow(g_lblInfo, lblX, lblY, lblW, lblH, TRUE);
	
	if (g_btnEnter)
		MoveWindow(g_btnEnter, btnX, btnY, btnW, btnH, TRUE);
}

static void CenterWindow(HWND hwnd)
{
	RECT rcWork{};
	GetWorkArea(hwnd, rcWork);
	
	int winW = g_clientW;
	int winH = g_clientH;
	
	g_x = rcWork.left + ((rcWork.right - rcWork.left) - winW) / 2.0;
	g_y = rcWork.top + ((rcWork.bottom - rcWork.top) - winH) / 2.0;
	
	SetWindowPos(hwnd, nullptr, (int)std::lround(g_x), (int)std::lround(g_y),
				 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

static void UpdateTargetCenter(HWND hwnd)
{
	RECT rcWork{};
	GetWorkArea(hwnd, rcWork);
	g_targetX = rcWork.left + ((rcWork.right - rcWork.left) - g_clientW) / 2.0;
	g_targetY = rcWork.top + ((rcWork.bottom - rcWork.top) - g_clientH) / 2.0;
}

static void TeleportToOppositeSide(HWND hwnd, int dirX, int dirY)
{
	RECT rcWork{};
	GetWorkArea(hwnd, rcWork);
	
	const double margin = Scale(16);
	
	if (dirX > 0)
	{
		// 从右边跑出去后，从左边出现
		g_x = rcWork.left - g_clientW + margin;
	}
	else if (dirX < 0)
	{
		// 从左边跑出去后，从右边出现
		g_x = rcWork.right - margin;
	}
	
	if (dirY > 0)
	{
		// 从下边跑出去后，从上边出现
		g_y = rcWork.top - g_clientH + margin;
	}
	else if (dirY < 0)
	{
		// 从上边跑出去后，从下边出现
		g_y = rcWork.bottom - margin;
	}
	
	g_returning = true;
	UpdateTargetCenter(hwnd);
	
	SetWindowPos(hwnd, nullptr, (int)std::lround(g_x), (int)std::lround(g_y),
				 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

static void MoveAwayFromCursor(HWND hwnd)
{
	POINT pt{};
	GetCursorPos(&pt);
	
	RECT rc{};
	GetWindowRect(hwnd, &rc);
	
	double cx = (rc.left + rc.right) / 2.0;
	double cy = (rc.top + rc.bottom) / 2.0;
	
	double dx = cx - pt.x;
	double dy = cy - pt.y;
	
	double dist2 = dx * dx + dy * dy;
	double danger = (double)Scale(220);
	
	if (dist2 > danger * danger)
		return;
	
	double dist = std::sqrt(dist2);
	if (dist < 1.0)
		dist = 1.0;
	
	double nx = dx / dist;
	double ny = dy / dist;
	
	double step = (double)Scale(22);
	
	g_x += nx * step;
	g_y += ny * step;
	
	SetWindowPos(hwnd, nullptr, (int)std::lround(g_x), (int)std::lround(g_y),
				 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
	
	RECT rcWork{};
	GetWorkArea(hwnd, rcWork);
	
	int dirX = 0;
	int dirY = 0;
	
	if (rc.right < rcWork.left - Scale(8)) dirX = 1;
	else if (rc.left > rcWork.right + Scale(8)) dirX = -1;
	
	if (rc.bottom < rcWork.top - Scale(8)) dirY = 1;
	else if (rc.top > rcWork.bottom + Scale(8)) dirY = -1;
	
	if (dirX != 0 || dirY != 0)
		TeleportToOppositeSide(hwnd, dirX, dirY);
}

static void SmoothReturnToCenter(HWND hwnd)
{
	if (!g_returning)
		return;
	
	double ease = 0.3;
	
	g_x += (g_targetX - g_x) * ease;
	g_y += (g_targetY - g_y) * ease;
	
	if (std::fabs(g_targetX - g_x) < 1.0 && std::fabs(g_targetY - g_y) < 1.0)
	{
		g_x = g_targetX;
		g_y = g_targetY;
		g_returning = false;
	}
	
	SetWindowPos(hwnd, nullptr, (int)std::lround(g_x), (int)std::lround(g_y),
				 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

static void RepositionIfNeeded(HWND hwnd)
{
	RECT rc{};
	GetWindowRect(hwnd, &rc);
	
	RECT work{};
	GetWorkArea(hwnd, work);
	
	int dirX = 0;
	int dirY = 0;
	
	if (rc.right < work.left - Scale(8)) dirX = 1;
	else if (rc.left > work.right + Scale(8)) dirX = -1;
	
	if (rc.bottom < work.top - Scale(8)) dirY = 1;
	else if (rc.top > work.bottom + Scale(8)) dirY = -1;
	
	if (dirX != 0 || dirY != 0)
		TeleportToOppositeSide(hwnd, dirX, dirY);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		g_hwnd = hwnd;
		
		g_lblInfo = CreateWindowExW(
									0, L"STATIC", L"欢迎使用本软件！",
									WS_CHILD | WS_VISIBLE | SS_CENTER,
									0, 0, 0, 0,
									hwnd, nullptr, (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), nullptr
									);
		
		g_btnEnter = CreateWindowExW(
									 0, L"BUTTON", L"进入软件",
									 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
									 0, 0, 0, 0,
									 hwnd, (HMENU)1001, (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), nullptr
									 );
		
		RebuildFont(hwnd);
		LayoutControls(hwnd);
		CenterWindow(hwnd);
		UpdateTargetCenter(hwnd);
		
		SetTimer(hwnd, 1, 16, nullptr);
		return 0;
	}
		
	case WM_NCHITTEST:
		{
			// 禁止拖拽边框调整大小
			LRESULT hit = DefWindowProcW(hwnd, msg, wParam, lParam);
			if (hit == HTLEFT || hit == HTRIGHT || hit == HTTOP || hit == HTBOTTOM ||
				hit == HTTOPLEFT || hit == HTTOPRIGHT || hit == HTBOTTOMLEFT || hit == HTBOTTOMRIGHT)
				return HTCLIENT;
			return hit;
		}
		
	case WM_CTLCOLORSTATIC:
		return (LRESULT)GetStockObject(WHITE_BRUSH);

	case WM_SIZE:
		LayoutControls(hwnd);
		return 0;
		
	case WM_DPICHANGED:
		{
			g_dpi = HIWORD(wParam);
			RECT* suggested = (RECT*)lParam;
			
			SetWindowPos(hwnd, nullptr,
						 suggested->left, suggested->top,
						 suggested->right - suggested->left,
						 suggested->bottom - suggested->top,
						 SWP_NOZORDER | SWP_NOACTIVATE);
			
			RebuildFont(hwnd);
			LayoutControls(hwnd);
			return 0;
		}
		
	case WM_TIMER:
		if (g_returning)
			SmoothReturnToCenter(hwnd);
		else
			MoveAwayFromCursor(hwnd);
		
		RepositionIfNeeded(hwnd);
		return 0;
		
	case WM_COMMAND:
		if (LOWORD(wParam) == 1001 && HIWORD(wParam) == BN_CLICKED)
		{
			SetWindowTextW(g_btnEnter, L"已进入软件");
			MessageBoxW(hwnd, L"已经进入软件。", L"提示", MB_OK | MB_ICONINFORMATION);
			return 0;
		}
		return 0;
		
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(hwnd);
		return 0;
		
	case WM_DESTROY:
		KillTimer(hwnd, 1);
		if (g_font)
		{
			DeleteObject(g_font);
			g_font = nullptr;
		}
		PostQuitMessage(0);
		return 0;
	}
	
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszClassName = kClassName;
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	
	if (!RegisterClassExW(&wc))
		return 0;
	
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	DWORD exStyle = WS_EX_APPWINDOW;
	
	RECT rcClient{ 0, 0, g_clientW, g_clientH };
	AdjustWindowRectEx(&rcClient, style, FALSE, exStyle);
	
	int winW = rcClient.right - rcClient.left;
	int winH = rcClient.bottom - rcClient.top;
	
	HWND hwnd = CreateWindowExW(
								exStyle,
								kClassName,
								kWindowTitle,
								style,
								CW_USEDEFAULT, CW_USEDEFAULT,
								winW, winH,
								nullptr, nullptr, hInstance, nullptr
								);
	
	if (!hwnd)
		return 0;
	
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	
	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return (int)msg.wParam;
}

