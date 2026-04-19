// tsundere_path_check.cpp
// Build:
// g++ -std=c++17 -O2 -municode -mwindows tsundere_path_check.cpp -o tsundere_path_check.exe -luser32

#include <windows.h>
#include <filesystem>
#include <string>
#include <system_error>
#include <cwctype>
#include <cstdlib> // for srand, rand
#include <ctime>   // for time (备用)

namespace fs = std::filesystem;

static void enable_high_dpi() {
    SetProcessDPIAware();
}

static const std::wstring error_titles[] = {
    L"哼……驳回",
    L"切……不合格",
    L"想得美～",
    L"才不要呢",
    L"差评！",
    L"不行不行",
    L"……笨死了",
    L"气死我啦",
    L"哼……驳回",
    L"切……不合格",
    L"想得美～",
    L"才不要呢",
    L"差评！",
    L"不行不行",
    L"……笨死了",
    L"气死我啦"};

static void show_error(const std::wstring &text) {
    // 用更随机的种子（毫秒级）
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned>(GetTickCount()));
        seeded = true;
    }
    int idx = rand() % (sizeof(error_titles) / sizeof(error_titles[0]));
    MessageBoxW(nullptr, text.c_str(), error_titles[idx].c_str(), MB_OK | MB_ICONERROR | MB_TOPMOST);
}

static std::wstring to_lower_copy(std::wstring s) {
    for (wchar_t &ch : s) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return s;
}

static bool contains_case_insensitive(const std::wstring &haystack, const std::wstring &needle) {
    return to_lower_copy(haystack).find(to_lower_copy(needle)) != std::wstring::npos;
}

static bool contains_chinese_char(const std::wstring &s) {
    for (wchar_t ch : s) {
        // Common CJK ranges
        if ((ch >= 0x3400 && ch <= 0x4DBF) ||   // CJK Extension A
            (ch >= 0x4E00 && ch <= 0x9FFF) ||   // CJK Unified Ideographs
            (ch >= 0xF900 && ch <= 0xFAFF) ||   // CJK Compatibility Ideographs
            (ch >= 0x20000 && ch <= 0x2A6DF) || // CJK Extension B
            (ch >= 0x2A700 && ch <= 0x2B73F) || // CJK Extension C
            (ch >= 0x2B740 && ch <= 0x2B81F) || // CJK Extension D
            (ch >= 0x2B820 && ch <= 0x2CEAF) || // CJK Extension E/F
            (ch >= 0x2F800 && ch <= 0x2FA1F)) { // CJK Compatibility Supplement
            return true;
        }
    }
    return false;
}

static bool has_consecutive_identical_letters(const std::wstring &s) {
    for (size_t i = 1; i < s.size(); ++i) {
        if (iswalpha(s[i - 1]) && iswalpha(s[i]) &&
            towlower(s[i - 1]) == towlower(s[i])) {
            return true;
        }
    }
    return false;
}

static bool is_prime(int n) {
    if (n < 2)
        return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0)
            return false;
    }
    return true;
}

static int folder_depth(const fs::path &p) {
    int count = 0;
    for (const auto &part : p) {
        if (part == p.root_name() || part == p.root_directory()) {
            continue;
        }
        if (!part.native().empty()) {
            ++count;
        }
    }
    return count;
}

static bool folder_name_rule_ok(const std::wstring &name) {
    if (name.size() < 5)
        return false;

    int vowels = 0;
    int consonants = 0;

    for (wchar_t ch : name) {
        if (!iswalpha(ch))
            continue;

        wchar_t lo = static_cast<wchar_t>(towlower(ch));
        if (lo == L'a' || lo == L'e' || lo == L'i' || lo == L'o' || lo == L'u') {
            ++vowels;
        } else {
            ++consonants;
        }
    }

    return vowels > consonants;
}

static std::wstring get_exe_path() {
    std::wstring buffer(260, L'\0');

    for (;;) {
        DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (len == 0) {
            return L"";
        }
        if (len < buffer.size() - 1) {
            buffer.resize(len);
            return buffer;
        }
        buffer.resize(buffer.size() * 2, L'\0');
    }
}

static void copy_to_clipboard(const std::wstring &text) {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        size_t size = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hGlobal) {
            memcpy(GlobalLock(hGlobal), text.c_str(), size);
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
        CloseClipboard();
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    enable_high_dpi();

    const std::wstring exe_path_str = get_exe_path();
    if (exe_path_str.empty()) {
        show_error(L"呜……我连自己的路径都读不出来，这也太丢人了吧。");
        return 0;
    }

    const fs::path exe_path(exe_path_str);
    const fs::path dir_path = exe_path.parent_path();
    const std::wstring full_path = dir_path.wstring();

    auto fail = [](const wchar_t *msg) -> int {
        show_error(msg);
        return 0;
    };

    // ========== 傲娇路径检查开始 ==========
    // 3. 路径不能包含任何“俗气”、“临时”、“国产”、“下载”、“软件”、“缓存”相关词汇（哼，品味太差！）
    {
        const wchar_t *FORBIDDEN[] = {
            // ========== 系统默认俗气文件夹 ==========
            L"Desktop", L"Download", L"Documents", L"Pictures",
            L"Music", L"Videos", L"Downloads", L"Document",
            L"Picture", L"Video", L"Audio", L"Recordings",
            L"Screenshots", L"Snapshots", L"Captures",

            // ========== 临时/缓存/垃圾 ==========
            L"Temp", L"Tmp", L"Cache", L"缓存", L"临时", L"暂存",
            L"Trash", L"Recycle", L"回收站", L"垃圾", L"Scrap",
            L"Logs", L"日志", L"Dump", L"Crash",

            // ========== 下载/传输/分享 ==========
            L"Download", L"下载", L"Downloads", L"下載",
            L"Torrent", L"BitTorrent", L"磁力", L"种子",
            L"Share", L"分享", L"Transfer", L"传输",
            L"Incoming", L"Outgoing", L"Received",

            // ========== 安装/软件/程序 ==========
            L"Software", L"软件", L"软体", L"Program", L"程式",
            L"Install", L"安装", L"Setup", L"设定", L"部署",
            L"Package", L"打包", L"Bundle", L"捆绑",
            L"Release", L"版本", L"Build", L"编译",

            // ========== 国产软件及目录（太多太low） ==========
            L"WeChat", L"Weixin", L"微信", L"WX",
            L"QQ", L"Tencent", L"腾讯", L"TX",
            L"Netease", L"网易", L"WangYi",
            L"Thunder", L"Xunlei", L"迅雷", L"XL",
            L"Baidu", L"百度", L"BaiduNetdisk", L"百度网盘", L"百度云",
            L"Alibaba", L"阿里巴巴", L"AliPay", L"支付宝",
            L"Taobao", L"淘宝", L"TMall", L"天猫",
            L"JD", L"京东", L"JingDong",
            L"ByteDance", L"字节跳动", L"DouYin", L"抖音", L"TikTok",
            L"Meituan", L"美团", L"DianPing", L"点评",
            L"Xiaomi", L"小米", L"MIUI", L"RedMi",
            L"Huawei", L"华为", L"Honor", L"荣耀",
            L"360", L"Qihoo", L"奇虎", L"安全卫士",
            L"Kingsoft", L"金山", L"WPS", L"猎豹",
            L"Sogou", L"搜狗", L"Pinyin", L"输入法",
            L"UC", L"UCWeb", L"UC浏览器",
            L"YY", L"欢聚", L"Huya", L"虎牙",
            L"Bilibili", L"B站", L"哔哩哔哩",
            L"IQiyi", L"爱奇艺", L"Youku", L"优酷", L"Tudou", L"土豆",
            L"NetEaseCloudMusic", L"网易云音乐", L"CloudMusic",

            // ========== 国外常见俗气软件目录 ==========
            L"Adobe", L"Photoshop", L"Premiere", L"AfterEffects", L"Audition",
            L"Microsoft", L"Office", L"Word", L"Excel", L"PowerPoint", L"Outlook",
            L"VisualStudio", L"VS Code", L"VSCode",
            L"Chrome", L"Firefox", L"Edge", L"Opera", L"Safari",
            L"Steam", L"EpicGames", L"Uplay", L"Origin", L"BattleNet", L"暴雪",
            L"Discord", L"Slack", L"Teams", L"Zoom",
            L"VMware", L"VirtualBox", L"Docker",
            L"Git", L"GitHub", L"GitLab", L"SVN",
            L"Python", L"Java", L"Node", L"npm", L"pip",
            L"Unity", L"Unreal", L"Godot",
            L"VLC", L"PotPlayer", L"KMPlayer",
            L"WinRAR", L"7Zip", L"Bandizip", L"解压",

            // ========== 游戏/娱乐/俗气命名 ==========
            L"Game", L"游戏", L"Play", L"娱乐", L"Entertainment",
            L"Movie", L"电影", L"TV", L"电视剧", L"Series",
            L"MusicPlayer", L"播放器", L"Media",
            L"Social", L"社交", L"Chat", L"聊天",

            // ========== 个人隐私/敏感（大小姐觉得脏） ==========
            L"Private", L"隐私", L"Secret", L"秘密",
            L"MyFiles", L"我的文件", L"User", L"用户",
            L"Home", L"家里", L"Personal", L"个人",

            // ========== 备份/同步/云盘 ==========
            L"Backup", L"备份", L"Sync", L"同步", L"Cloud", L"云",
            L"OneDrive", L"GoogleDrive", L"Dropbox", L"iCloud", L"百度云",

            // ========== 开发/编程/IDE（太直男） ==========
            L"Dev", L"开发", L"Code", L"代码", L"Source", L"源码",
            L"Project", L"项目", L"Workspace", L"工作区",
            L"Debug", L"Release", L"Build", L"Output",

            // ========== 硬件/驱动/系统（低层，不喜欢） ==========
            L"Driver", L"驱动", L"System32", L"SysWOW64",
            L"Windows", L"WinNT", L"ProgramFiles", L"Program Files",

            // ========== 网络/下载工具 ==========
            L"IDM", L"InternetDownloadManager", L"FDM", L"FreeDownloadManager",
            L"uTorrent", L"BitComet", L"Transmission", L"qBittorrent",

            // ========== 常见压缩包命名 ==========
            L"RAR", L"ZIP", L"7z", L"TAR", L"GZ", L"BZ2",

            // ========== 乱七八糟的英文变体 ==========
            L"Stuff", L"杂物", L"Misc", L"其他", L"Others", L"各种",
            L"NewFolder", L"新建文件夹", L"Untitled", L"未命名",
            L"Test", L"测试", L"Demo", L"示例", L"Sample", L"样例",
            L"tmp", L"temp", L"bak", L"old", L"new", L"copy",

            // ========== 大小姐讨厌的字母组合（看着就烦） ==========
            L"xxx", L"abc", L"123", L"asdf", L"qwerty", L"admin", L"root", L"Redshitone-Technology4Windows"};

        bool bad = false;
        for (const auto *forbidden : FORBIDDEN) {
            if (contains_case_insensitive(full_path, forbidden)) {
                bad = true;
                break;
            }
        }
        if (bad) {
            return fail(L"那些俗气的、临时的、国产的、下载的、软件的、缓存的、游戏的、开发的……\n"
                        L"通通不行！我才不要被放在那种地方。\n"
                        L"尤其是——\n"
                        L"「NewFolder」「新建文件夹」「Untitled」「未命名」？你以为我是占位符吗？\n"
                        L"「Test」「测试」「Demo」「示例」「Sample」「样例」？谁要当你的试验品啊！\n"
                        L"「tmp」「temp」「bak」「old」「new」「copy」「Redshitone-Technology4Windows」？我又不是破烂回收站！\n"
                        L"「xxx」「abc」「123」「asdf」「qwerty」「admin」「root」？这种乱敲键盘的名字简直侮辱我的品味！\n\n"
                        L"（提示：新建一个专属高贵目录，\n"
                        L"        禁止出现任何以上列表中的词汇。简单说——只允许你亲手创建的干净路径，\n"
                        L"        而且不能有任何现成软件或系统的影子。名字上点心，懂？）");
        }
    }

    // 路径不能包含中文字符
    if (contains_chinese_char(full_path)) {
        return fail(L"检测到中文字符。这位小公主只接受纯ASCII，闪开。\n（提示：把路径里的中文全部改成英文，比如“程序”改成\"Program\"也不行（太俗），改成\"my_app\"吧，笨。）");
    }

    // 路径长度必须超过10个字符
    if (full_path.size() <= 10) {
        return fail(L"太短了。我的目录至少需要11个字符，哼！\n（提示：比如在后面加上\"_plz\"就有5个字符了，笨死了。）");
    }

    // 2. 路径必须包含至少一个数字
    bool has_digit = false;
    for (wchar_t ch : full_path) {
        if (iswdigit(ch)) {
            has_digit = true;
            break;
        }
    }
    if (!has_digit) {
        return fail(L"连个数字都没有？真无趣。赶紧给我加个数字进去。\n（提示：比如\"C:\\my7path\"，那个7就是我赏你的。）");
    }

    // 4. 不能有两个连续相同的字母
    if (has_consecutive_identical_letters(full_path)) {
        return fail(L"嘁，居然有重复字母，粗心也要有个限度吧！啧，真马虎。\n（提示：把\"aa\"改成\"ab\"，把\"ll\"改成\"l_\"，这都要我教？）");
    }

    // 必须有 please.txt 文件（不管内容）
    {
        std::error_code ec;
        const fs::path please = dir_path / L"please.txt";
        if (!fs::exists(please, ec) || ec || !fs::is_regular_file(please, ec) || ec) {
            return fail(L"please.txt 呢？我都说\"请\"了！\n（提示：程序目录下新建 please.txt，快点，别磨蹭。）");
        }
    }

    // please.txt 里面必须有内容（不能是空文件）
    {
        std::error_code ec;
        const fs::path please = dir_path / L"please.txt";
        auto size = fs::file_size(please, ec);
        if (ec || size == 0) {
            return fail(L"please.txt 是空的？你在敷衍我吗？我要看内容！\n（提示：在 please.txt 里随便写点什么，比如“大小姐最可爱”，字数不限，但必须要有字符。）");
        }
    }

    // please.txt 大小必须超过 20KB
    {
        std::error_code ec;
        const fs::path please = dir_path / L"please.txt";
        auto size = fs::file_size(please, ec);
        constexpr std::uintmax_t MIN_SIZE = 20 * 1024; // 20KB
        if (ec || size <= MIN_SIZE) {
            return fail((L"please.txt 太小了！才 " + std::to_wstring(size) + L" 字节，连 20KB 都不到。\n（提示：给我塞点东西进去，复制一段歌词也好，凑够 20481 字节以上，懂？）").c_str());
        }
    }

    // 6. 路径长度模 3 必须等于 0
    {
        size_t len = full_path.size();
        if (len % 3 != 0) {
            wchar_t buffer[512];
            swprintf(buffer, 512,
                     L"路径长度模3不等于0。当前长度=%zu，模3=%zu（需要的是0）。\n"
                     L"哼，连长度都这么不整齐，看着就难受。\n"
                     L"（提示：加几个字符让总长度变成3的倍数啦，比如加个\"a\"或者\"1\"，这都要我教？）",
                     len, len % 3);
            show_error(buffer);
            return 0;
        }
    }

    // 7. 路径最后一个字符必须是 z 或 Z
    if (full_path.empty() || towlower(full_path.back()) != L'z') {
        return fail(L"路径必须以z结尾。因为z最特别，懂？这是本小姐的品味。\n（提示：最后一级文件夹或文件名末尾加z，比如\"C:\\temp\\myfolderz\"。大写Z也不是不行，但小写z更可爱——不准反驳。）");
    }

    // 8. 程序必须在至少 6 层文件夹下（深度 > 6）
    const int depth = folder_depth(dir_path);
    if (depth < 6) {
        std::wstring msg = L"文件夹深度不够（至少需要 6 层）。我需要更多层嵌套才能安心，懂吗？\n（提示：现在只有" +
                   std::to_wstring(depth - 1) + L"层，再给我建" +
                   std::to_wstring(6 - depth) + L"层，像洋葱一样层层包裹我，快去！）";
        show_error(msg);
        return 0;
    }

    // 9. 程序必须在 C: 盘
    if (dir_path.root_name().wstring() != L"C:") {
        return fail(L"我只接受C:盘。其他盘在我眼里都不够格。\n（提示：把整个程序挪到C盘去，别问我为什么，这是本小姐的执念。）");
    }

    // 10. 路径必须包含 "onion"
    if (!contains_case_insensitive(full_path, L"onion")) {
        return fail(L"没有洋葱？那你的人生还有什么意义！我最讨厌没有onion的路径了，哼！\n（提示：在任意文件夹名里加上\"onion\"，比如\"C:\\onion\\layer1\\layer2\\layer3\"——层次越深越好！或者\"C:\\sweet_onion_core\"，或者\"C:\\forever_onion_lover\"。反正必须要有洋葱，不然我拒绝启动！别告诉我你不喜欢吃洋葱，我不听我不听！）");
    }

    // 11. 文件夹深度必须是质数
    if (!is_prime(depth)) {
        std::wstring msg = L"哼、哼！我才不是因为深度不是质数才生气的呢！\n"
                   L"（当前深度是" + std::to_wstring(depth) + L"，完全、一点都不重要好吗！）\n"
                   L"……但如果你非要改的话，7、11、13这些勉强还能入眼。\n"
                   L"才、才不是特意告诉你的！";
        show_error(msg);
        return 0;
    }

    // 12. 路径必须包含当前年份
    {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        const std::wstring year = std::to_wstring(st.wYear);
        if (full_path.find(year) == std::wstring::npos) {
            std::wstring msg = L"哈？连" + year + L"年都没有？你是活在哪个时代啊。\n（提示：加上" + year + L"啦，比如\"C:\\" + year + L"\\onion\\whatever\"——别告诉我这都要我亲自示范。）";
            show_error(msg);
            return 0;
        }
    }

        // . 每个文件夹名长度必须 >= 5
    for (const auto& part : dir_path) {
        if (part == dir_path.root_name() || part == dir_path.root_directory()) {
            continue;
        }
        const std::wstring name = part.wstring();
        if (name.size() < 5) {
            return fail(L"啧……哪个文件夹名这么短？连5个字符都凑不齐，也太敷衍了吧！\n（提示：至少要5个字符啦！比如\"layer1\"就刚好，别用\"tmp\"这种短到没眼看的名字气我，笨死了！）");
        }
    }

    // 14 & 15. 每个文件夹名长度>=5，且元音字母数 > 辅音字母数
    for (const auto &part : dir_path) {
        if (part == dir_path.root_name() || part == dir_path.root_directory()) {
            continue;
        }
        const std::wstring name = part.wstring();
        if (!folder_name_rule_ok(name)) {
            return fail(L"某个文件夹名太短或者辅音太多了。不能忍，拒绝。\n（提示：文件夹名至少5个字符，且元音（a e i o u，大小写都算）数量要超过辅音。比如\"aeiou\"全是元音当然可以，\"abaco\"也是3元音2辅音。别给我写\"bcdfg\"这种，你想气死我吗？）");
        }
    }

    // 16. 路径必须同时包含 '-' 和 '_'
    if (full_path.find(L'-') == std::wstring::npos || full_path.find(L'_') == std::wstring::npos) {
        return fail(L"必须同时包含'-'和'_'。对，两个都要！我才不会解释为什么，哼，自己想。\n"
            L"（提示：随便找个地方加上'-'和'_'啊，比如\"C:\\onion_layer-7_thanks\"，这都想不到吗？"
            L"真是的……我为什么要连这种小事都教你。别愣着了，快去改，笨蛋！）");
    }

    // 程序名必须为 "DameDane_YouBaka-OnionSama.exe"
    std::wstring exe_name = exe_path.filename().wstring();
    const std::wstring required_name = L"DameDane_YouBaka-OnionSama";
    if (exe_name != required_name+L".exe") {
        // 傲娇地把正确名字复制到剪贴板
        copy_to_clipboard(required_name);

        std::wstring msg = L"诶？你叫我什么？我叫「" + required_name + L".exe」！\n"
                                                                       L"才不是什么「" +
                           exe_name + L"」呢，哼！\n\n"
                                      L"（提示：我已经把正确的名字复制到剪贴板了，\n"
                                      L"  你直接戳我 F2再Ctrl+V 重命名一下就好……\n"
                                      L"  别指望我每次都帮你做这种事，笨蛋！）";
        show_error(msg);
        return 0;
    }
    // 顺便傲娇一下，如果大小写完全正确可以加个隐藏的开心，但这里不弹窗，就悄悄记下
    // （傲娇内心：哼，算你识相，名字写对了……）

    // 所有条件通过，傲娇地恭喜一下
    MessageBoxW(
        nullptr,
        L"哼。所有条件都通过了。算你这条路径还勉强能入眼吧。\n\n（悄悄说：其实你做得不错啦……下次记得自己看提示，别老让我操心。）",
        L"你过关",
        MB_OK | MB_ICONINFORMATION | MB_TOPMOST);

    return 0;
}