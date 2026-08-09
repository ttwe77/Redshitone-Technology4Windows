#include <iostream>
#include <conio.h>      // _kbhit, _getch
#include <windows.h>    // GetStdHandle, SetConsoleOutputCP, Sleep, GetTickCount
#include <algorithm>    // std::min, std::max
#include <cstdlib>      // system

int main() {
    // 1. 设置控制台输出为 UTF-8，保证中文等字符正常显示
    SetConsoleOutputCP(CP_UTF8);

    // 2. 隐藏光标，让动画更整洁
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    // 3. 动画参数
    const char* spinChars = "-\\|/";   // 旋转字符序列：- \ | /
    int charIndex = 0;                // 当前字符索引
    int direction = 1;                // 方向：1 顺时针（正向），-1 逆时针
    int delay = 200;                  // 更新间隔（毫秒），即旋转速度
    const int MIN_DELAY = 10;         // 最快速度（延迟最小）
    const int MAX_DELAY = 10000;       // 最慢速度（延迟最大）
    const int STEP = 10;             // 每次速度变化步长

    DWORD lastUpdate = GetTickCount(); // 上次更新时刻
    int pressCount = 0;                // 空格键按下次数：0=初始, 1=清除提示, >=2=切换信息显示

    // 4. 提示信息（UTF-8 中文）
    std::cout << "旋转动画已启动\n";
    std::cout << "小键盘 ← → 调节速度，小键盘 ↑ 切换方向，空格切换显示，ESC 退出\n";
    std::cout << "当前速度: " << delay << "ms, 方向: 顺时针\n";

    // 5. 主循环
    while (true) {
        // 处理键盘输入
        if (_kbhit()) {
            int ch = _getch();
            // 扩展键（包括小键盘方向键）通常返回 0x00 或 0xE0
            if (ch == 0 || ch == 0xE0) {
                ch = _getch();
                switch (ch) {
                case 0x4B: // 小键盘 ←
                    delay = std::min(delay + STEP, MAX_DELAY);
                    break;
                case 0x4D: // 小键盘 →
                    delay = std::max(delay - STEP, MIN_DELAY);
                    break;
                case 0x48: // 小键盘 ↑
                    direction = -direction;
                    break;
                default:
                    break;
                }
            } else if (ch == 27) {   // ESC 键退出
                break;
            } else if (ch == 32) {   // 空格键切换显示状态
                pressCount++;
                if (pressCount == 1) {
                    // 第一次按下：清屏
                    system("cls");
                }
            }
        }

        // 控制动画刷新频率
        DWORD now = GetTickCount();
        if (now - lastUpdate >= static_cast<DWORD>(delay)) {
            lastUpdate = now;
            // 根据方向更新字符索引（保证在 0~3 内循环）
            charIndex = (charIndex + direction + 4) % 4;
            // 输出动画帧：\r 回到行首
            if (pressCount == 0) {
                // 初始状态：显示完整信息
                std::cout << "\r[" << spinChars[charIndex] << "] "
                          << "速度:" << delay << "ms  "
                          << "方向:" << (direction == 1 ? "顺时针" : "逆时针")
                          << "      " << std::flush;
            } else if (pressCount % 2 == 1) {
                // 奇数次按下（1,3,5...）：显示速度和方向信息
                std::cout << "\r[" << spinChars[charIndex] << "] "
                          << "速度:" << delay << "ms  "
                          << "方向:" << (direction == 1 ? "顺时针" : "逆时针")
                          << "      " << std::flush;
            } else {
                // 偶数次按下（2,4,6...）：只显示 spinChars，尾部空格清除旧内容
                std::cout << "\r[" << spinChars[charIndex] << "]                             " << std::flush;
            }
        }

        // 短暂休眠，降低 CPU 占用，同时保证按键响应及时
        Sleep(1);
    }

    // 6. 清理与退出
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    std::cout << "\n动画已结束。\n";
    return 0;
}