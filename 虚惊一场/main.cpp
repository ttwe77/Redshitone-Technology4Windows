// g++ -o fake_uninstall.exe fake_uninstall.cpp -mwindows -lcomctl32 -static-libgcc -static-libstdc++
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>   
#include <algorithm>  

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")

#define WM_UPDATE_PROGRESS   (WM_USER + 100)
#define WM_UPDATE_LIST       (WM_USER + 101)
#define WM_UNINSTALL_DONE    (WM_USER + 102)

// 全局句柄
HWND hProgress, hList, hBtnFinish;
HANDLE hThread = NULL;
BOOL g_bUninstallDone = FALSE;

// 模拟“正在移除”的文件列表（若遍历失败则用此）
const char* g_fakeFiles[] = {
	"ntoskrnl.exe", "hal.dll", "winload.exe", "ntdll.dll",
	"kernel32.dll", "user32.dll", "gdi32.dll", "shell32.dll",
	"cmd.exe", "notepad.exe", "mspaint.exe", "calc.exe",
	"explorer.exe", "svchost.exe", "winlogon.exe", "csrss.exe"
};

// 获取自身完整路径（宽字符）
std::wstring GetSelfPath() {
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(nullptr, path, MAX_PATH);
	return std::wstring(path);
}

// 向列表框添加一行（ANSI 文本）
void AddListLine(const char* text) {
	SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)text);
	int count = (int)SendMessageA(hList, LB_GETCOUNT, 0, 0);
	if (count > 0)
		SendMessageA(hList, LB_SETTOPINDEX, count - 1, 0);
}

// 删除自身（通过生成批处理文件）
void DeleteSelf() {
	// 获取自身宽字符路径
	std::wstring wSelfPath = GetSelfPath();
	
	// 将宽字符路径转换为 ANSI 编码，用于批处理文件
	char szSelfPath[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, wSelfPath.c_str(), -1, szSelfPath, MAX_PATH, NULL, NULL);
	
	char szBatPath[MAX_PATH];
	char szCmdLine[1024];
	
	// 生成临时批处理文件
	GetTempPathA(MAX_PATH, szBatPath);
	strcat_s(szBatPath, MAX_PATH, "del_me.bat");
	
	FILE* f = fopen(szBatPath, "w");
	if (f) {
		fprintf(f, "@echo off\r\n");
		fprintf(f, ":loop\r\n");
		fprintf(f, "del \"%s\"\r\n", szSelfPath);
		fprintf(f, "if exist \"%s\" goto loop\r\n", szSelfPath);
		fprintf(f, "del \"%%~f0\"\r\n");  // 批处理自删除
		fclose(f);
		
		// 以隐藏窗口方式执行批处理
		sprintf_s(szCmdLine, sizeof(szCmdLine), "/c \"%s\"", szBatPath);
		ShellExecuteA(NULL, "open", "cmd.exe", szCmdLine, NULL, SW_HIDE);
	}
}

// 模拟卸载线程
DWORD WINAPI UninstallThread(LPVOID lpParam) {
	char szPath[MAX_PATH];
	char szMsg[512];
	WIN32_FIND_DATAA fd;
	HANDLE hFind;
	int fileCount = 0;
	int totalFiles = 100;
	
	GetWindowsDirectoryA(szPath, MAX_PATH);
	strcat_s(szPath, MAX_PATH, "\\Fonts\\*.*");
	
	// 统计文件数
	hFind = FindFirstFileA(szPath, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				totalFiles++;
		} while (FindNextFileA(hFind, &fd));
		FindClose(hFind);
	} else {
		totalFiles = sizeof(g_fakeFiles) / sizeof(g_fakeFiles[0]);
	}
	
	SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, totalFiles));
	SendMessage(hProgress, PBM_SETPOS, 0, 0);
	
	// 实际更新 UI
	hFind = FindFirstFileA(szPath, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			sprintf_s(szMsg, sizeof(szMsg), "正在移除 C:\\Windows\\System32\\%s", fd.cFileName);
			SendMessageA(GetParent(hProgress), WM_UPDATE_LIST, 0, (LPARAM)szMsg);
			fileCount++;
			SendMessage(hProgress, PBM_SETPOS, fileCount, 0);
			//Sleep(50);
		} while (FindNextFileA(hFind, &fd));
		FindClose(hFind);
	} else {
		for (int i = 0; i < totalFiles; i++) {
			sprintf_s(szMsg, sizeof(szMsg), "正在移除 C:\\Windows\\System32\\%s", g_fakeFiles[i]);
			SendMessageA(GetParent(hProgress), WM_UPDATE_LIST, 0, (LPARAM)szMsg);
			fileCount++;
			SendMessage(hProgress, PBM_SETPOS, fileCount, 0);
			//Sleep(80);
		}
	}
	
	SendMessage(hProgress, PBM_SETPOS, totalFiles, 0);
	Sleep(200);
	SendMessage(GetParent(hProgress), WM_UNINSTALL_DONE, 0, 0);
	return 0;
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE: {
		// === 窗口居中 ===
		RECT rc;
		GetWindowRect(hWnd, &rc);
		int winWidth = rc.right - rc.left;
		int winHeight = rc.bottom - rc.top;
		
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		
		int x = (screenWidth - winWidth) / 2;
		int y = (screenHeight - winHeight) / 2;
		
		SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		
		// 移除窗口关闭按钮（禁用系统菜单中的关闭项）
		LONG style = GetWindowLong(hWnd, GWL_STYLE);
		style &= ~WS_SYSMENU;
		SetWindowLong(hWnd, GWL_STYLE, style);
		
		// 创建静态文本：显示卸载提示
		CreateWindowExA(0, "STATIC", "正在卸载本程序...",
						WS_CHILD | WS_VISIBLE | SS_LEFT,
						20, 5, 480, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
		
		// 创建进度条
		hProgress = CreateWindowExA(0, PROGRESS_CLASSA, NULL,
									WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
									20, 30, 480, 25, hWnd, NULL, GetModuleHandle(NULL), NULL);
		
		// 创建列表框
		hList = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", NULL,
								WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
								20, 65, 480, 350, hWnd, NULL, GetModuleHandle(NULL), NULL);
		
		// 启动卸载线程
		hThread = CreateThread(NULL, 0, UninstallThread, hWnd, 0, NULL);
		break;
	}
		
	case WM_CLOSE:
		// 禁止关闭（窗口无法通过常规手段关闭）
		return 0;
		
		case WM_UPDATE_LIST: {
			AddListLine((const char*)lParam);
			break;
		}
		
		case WM_UNINSTALL_DONE: {
			g_bUninstallDone = TRUE;
			AddListLine("========================================");
			AddListLine("卸载完成！系统已成功摧毁");
			
			// 弹出宽字符提示框
			int ret = MessageBoxW(hWnd,
								  L"本程序以及您的系统已完全从此电脑上移除。",
								  L"卸载完成",
								  MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
			
			// 无论点击什么按钮，都执行自删除并退出
			DeleteSelf();
			PostQuitMessage(0);
			break;
		}
		
		case WM_COMMAND: {
			// 完成按钮实际上不会用到（因为弹窗会自动处理），但保留以备扩展
			if (LOWORD(wParam) == 101 && g_bUninstallDone) {
				// 理论上不会走到这里，因为弹窗已处理
			}
			break;
		}
		
		case WM_DESTROY: {
			if (hThread) {
				TerminateThread(hThread, 0);
				CloseHandle(hThread);
			}
			PostQuitMessage(0);
			break;
		}
		
	default:
		return DefWindowProcA(hWnd, msg, wParam, lParam);
	}
	return 0;
}

// 设置 DPI 感知（适配高 DPI）
void SetDPIAware() {
	typedef BOOL(WINAPI *SetProcessDPIAwareFunc)(void);
	HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
	if (hUser32) {
		SetProcessDPIAwareFunc pSetProcessDPIAware =
		(SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
		if (pSetProcessDPIAware) {
			pSetProcessDPIAware();
		}
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	SetDPIAware();  // 适配高 DPI
	
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icex);
	
	WNDCLASSEXA wc = {0};
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = "FakeUninstallClass";
	
	if (!RegisterClassExA(&wc)) return 0;
	
	HWND hWnd = CreateWindowExA(0, "FakeUninstallClass", "Uninstall",
								WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX,
								CW_USEDEFAULT, CW_USEDEFAULT, 520, 480,
								NULL, NULL, hInstance, NULL);
	
	if (!hWnd) return 0;
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	
	MSG msg;
	while (GetMessageA(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return (int)msg.wParam;
}
