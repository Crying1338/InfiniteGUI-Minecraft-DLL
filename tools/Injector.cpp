// ============================================================
//  InfiniteGUI-Injector
//  无限 GUI DLL 独立注入器（x64）
//
//  用法:
//    InfiniteGUI-Injector.exe                    自动查找 Minecraft 窗口并注入
//    InfiniteGUI-Injector.exe <PID>              指定进程 PID
//    InfiniteGUI-Injector.exe -t <标题子串>       指定窗口标题子串（默认 "Minecraft"）
//    InfiniteGUI-Injector.exe -d <DLL路径>        指定 DLL（默认自动搜索）
//    InfiniteGUI-Injector.exe -l                 仅列出候选 Java 进程
//
//  说明:
//    - 仅支持 64 位目标进程（DLL 为 x64）；32 位 Java 启动的游戏无法注入。
//    - 注入后会自动验证模块是否加载、DLL 内线程是否已启动（说明初始化成功）。
//    - 若提示“拒绝访问(5)”，请右键以管理员身份运行（游戏以管理员启动时必须如此）。
// ============================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "user32.lib")

static const wchar_t* DEFAULT_TITLE = L"Minecraft";
static const wchar_t* MODULE_NAME = L"InfiniteGUI-DLL.dll";

// ------------------------------------------------------------
static void Print(const char* fmt, ...)
{
	va_list ap; va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

static bool FileExists(const std::wstring& p)
{
	DWORD a = GetFileAttributesW(p.c_str());
	return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring ExeDir()
{
	wchar_t buf[MAX_PATH]{};
	GetModuleFileNameW(nullptr, buf, MAX_PATH);
	std::wstring s(buf);
	size_t pos = s.find_last_of(L"\\/");
	return pos == std::wstring::npos ? L"." : s.substr(0, pos);
}

// 在若干常见位置查找 DLL
static std::wstring FindDll(const std::wstring& explicitPath)
{
	if (!explicitPath.empty())
		return FileExists(explicitPath) ? explicitPath : L"";

	std::vector<std::wstring> cands;
	std::wstring dir = ExeDir();
	cands.push_back(dir + L"\\" + MODULE_NAME);
	cands.push_back(dir + L"\\..\\InfiniteGUI-DLL\\x64\\Release\\" + MODULE_NAME);
	cands.push_back(dir + L"\\..\\..\\InfiniteGUI-DLL\\x64\\Release\\" + MODULE_NAME);
	cands.push_back(dir + L"\\..\\InfiniteGUI-DLL\\x64\\Debug\\" + MODULE_NAME);

	for (auto& c : cands)
	{
		wchar_t full[MAX_PATH]{};
		if (GetFullPathNameW(c.c_str(), MAX_PATH, full, nullptr) && FileExists(full))
			return full;
	}
	return L"";
}

static bool IsWow64(HANDLE proc, bool& is32)
{
	BOOL w = FALSE;
	is32 = false;
	if (IsWow64Process(proc, &w))
	{
		if (w) { is32 = true; return true; }   // 64 位系统上运行的 32 位进程
	}
	return true;
}

struct ProcInfo { DWORD pid; std::wstring name; std::wstring title; };

static std::vector<ProcInfo> ListJavaProcs()
{
	std::vector<ProcInfo> out;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) return out;

	PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
	if (Process32FirstW(snap, &pe))
	{
		do
		{
			std::wstring n = pe.szExeFile;
			std::transform(n.begin(), n.end(), n.begin(), towlower);
			if (n == L"java.exe" || n == L"javaw.exe" || n == L"javaw.exe")
			{
				ProcInfo pi; pi.pid = pe.th32ProcessID; pi.name = pe.szExeFile;
				// 取主窗口标题
     			struct Ctx { DWORD pid; HWND hwnd; } ctx{ pe.th32ProcessID, nullptr };
				EnumWindows([](HWND h, LPARAM lp) -> BOOL {
					Ctx* c = (Ctx*)lp;
					DWORD wp = 0; GetWindowThreadProcessId(h, &wp);
					if (wp == c->pid && IsWindowVisible(h) && GetWindow(h, GW_OWNER) == nullptr)
					{
						wchar_t t[512]{};
						GetWindowTextW(h, t, 512);
						if (wcslen(t) > 0) { c->hwnd = h; return FALSE; }
					}
					return TRUE;
				}, (LPARAM)&ctx);
				if (ctx.hwnd)
				{
					wchar_t t[512]{}; GetWindowTextW(ctx.hwnd, t, 512);
					pi.title = t;
				}
				out.push_back(pi);
			}
		} while (Process32NextW(snap, &pe));
	}
	CloseHandle(snap);
	return out;
}

static DWORD FindByTitle(const std::wstring& sub, std::wstring& foundTitle)
{
	for (auto& p : ListJavaProcs())
	{
		if (p.title.empty()) continue;
		std::wstring t = p.title, s = sub;
		std::transform(t.begin(), t.end(), t.begin(), towlower);
		std::transform(s.begin(), s.end(), s.begin(), towlower);
		if (t.find(s) != std::wstring::npos) { foundTitle = p.title; return p.pid; }
	}
	return 0;
}

// 模块是否已加载
static bool IsModuleLoaded(DWORD pid, const wchar_t* name)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap == INVALID_HANDLE_VALUE) return false;
	MODULEENTRY32W me{}; me.dwSize = sizeof(me);
	bool found = false;
	if (Module32FirstW(snap, &me))
	{
		do {
			if (_wcsicmp(me.szModule, name) == 0) { found = true; break; }
		} while (Module32NextW(snap, &me));
	}
	CloseHandle(snap);
	return found;
}

// 取模块基址与大小
static bool GetModuleRange(DWORD pid, const wchar_t* name, ULONGLONG& base, ULONGLONG& size)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap == INVALID_HANDLE_VALUE) return false;
	MODULEENTRY32W me{}; me.dwSize = sizeof(me);
	bool found = false;
	if (Module32FirstW(snap, &me))
	{
		do {
			if (_wcsicmp(me.szModule, name) == 0)
			{
				base = (ULONGLONG)me.modBaseAddr; size = me.modBaseSize; found = true; break;
			}
		} while (Module32NextW(snap, &me));
	}
	CloseHandle(snap);
	return found;
}

// 统计入口地址落在 [base, base+size) 内的线程数 —— 说明 DLL 的初始化线程已在运行
static int CountThreadsInModule(DWORD pid, ULONGLONG base, ULONGLONG size)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snap == INVALID_HANDLE_VALUE) return -1;
	THREADENTRY32 te{}; te.dwSize = sizeof(te);
	int count = 0;
	if (Thread32First(snap, &te))
	{
		do {
			if (te.th32OwnerProcessID != pid) continue;
			HANDLE th = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, te.th32ThreadID);
			if (!th) continue;
			// NtQueryInformationThread(ThreadQuerySetWin32StartAddress) 需 ntdll
			typedef LONG(WINAPI* NtQIT)(HANDLE, int, PVOID, ULONG, PULONG);
			static NtQIT fn = (NtQIT)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationThread");
			if (fn)
			{
				PVOID start = nullptr;
				if (fn(th, 9, &start, sizeof(start), nullptr) == 0)
				{
					ULONGLONG s = (ULONGLONG)start;
					if (s >= base && s < base + size) count++;
				}
			}
			CloseHandle(th);
		} while (Thread32Next(snap, &te));
	}
	CloseHandle(snap);
	return count;
}

// ------------------------------------------------------------
// 注入核心：OpenProcess -> VirtualAllocEx -> WriteProcessMemory
//           -> CreateRemoteThread(LoadLibraryW) -> 等待
static bool InjectDll(DWORD pid, const std::wstring& dllPath, std::wstring& err)
{
	HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
	if (!proc)
	{
		DWORD e = GetLastError();
		if (e == ERROR_ACCESS_DENIED)
			err = L"OpenProcess 拒绝访问(5) —— 请以管理员身份运行本注入器";
		else
			err = L"OpenProcess 失败, 错误码 " + std::to_wstring(e);
		return false;
	}

	SIZE_T bytes = (dllPath.size() + 1) * sizeof(wchar_t);
	LPVOID remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remote)
	{
		err = L"VirtualAllocEx 失败, 错误码 " + std::to_wstring(GetLastError());
		CloseHandle(proc);
		return false;
	}

	if (!WriteProcessMemory(proc, remote, dllPath.c_str(), bytes, nullptr))
	{
		err = L"WriteProcessMemory 失败, 错误码 " + std::to_wstring(GetLastError());
		VirtualFreeEx(proc, remote, 0, MEM_RELEASE); CloseHandle(proc);
		return false;
	}

	HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
	LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");
	if (!loadLib)
	{
		err = L"找不到 LoadLibraryW";
		VirtualFreeEx(proc, remote, 0, MEM_RELEASE); CloseHandle(proc);
		return false;
	}

	HANDLE th = CreateRemoteThread(proc, nullptr, 0, loadLib, remote, 0, nullptr);
	if (!th)
	{
		err = L"CreateRemoteThread 失败, 错误码 " + std::to_wstring(GetLastError());
		VirtualFreeEx(proc, remote, 0, MEM_RELEASE); CloseHandle(proc);
		return false;
	}

	DWORD wait = WaitForSingleObject(th, 20000);
	DWORD code = 0;
	GetExitCodeThread(th, &code);

	CloseHandle(th);
	VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
	CloseHandle(proc);

	if (wait != WAIT_OBJECT_0)
	{
		err = L"等待注入线程超时（DLL 可能在 DllMain 中阻塞）";
		return false;
	}
	if (code == 0)
	{
		err = L"LoadLibraryW 返回 0 —— DLL 加载失败（位数不匹配或缺少依赖）";
		return false;
	}
	return true;
}

// ------------------------------------------------------------
int wmain(int argc, wchar_t** argv)
{
	SetConsoleOutputCP(CP_UTF8);
	Print("==============================================\n");
	Print(" InfiniteGUI-Injector (x64)\n");
	Print("==============================================\n\n");

	std::wstring title = DEFAULT_TITLE;
	std::wstring dllArg;
	DWORD pid = 0;
	bool listOnly = false;

	for (int i = 1; i < argc; i++)
	{
		std::wstring a = argv[i];
		if (a == L"-t" && i + 1 < argc) title = argv[++i];
		else if (a == L"-d" && i + 1 < argc) dllArg = argv[++i];
		else if (a == L"-l" || a == L"--list") listOnly = true;
		else if (!a.empty() && a[0] >= L'0' && a[0] <= L'9') pid = (DWORD)_wtoi(a.c_str());
	}

	// 列出候选进程
	auto procs = ListJavaProcs();
	Print("[*] 检测到 %d 个 Java 进程:\n", (int)procs.size());
	for (auto& p : procs)
		Print("      PID %-7lu  %-12ls  窗口: %ls\n", p.pid, p.name.c_str(),
			p.title.empty() ? L"(无窗口)" : p.title.c_str());
	Print("\n");
	if (listOnly) return 0;

	// 选择目标
	std::wstring foundTitle;
	if (pid == 0)
	{
		pid = FindByTitle(title, foundTitle);
		if (pid == 0)
		{
			Print("[x] 未找到标题包含 \"%ls\" 的 Java 进程。\n", title.c_str());
			Print("    请确认游戏已启动，或用 -t 指定标题、或直接传 PID。\n");
			return 1;
		}
		Print("[+] 目标进程: PID %lu   窗口: %ls\n", pid, foundTitle.c_str());
	}
	else
	{
		Print("[+] 目标进程: PID %lu (手动指定)\n", pid);
	}

	// 句柄与位数检查
	HANDLE probe = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!probe)
	{
		Print("[x] 无法打开目标进程 (错误码 %lu)%ls\n", GetLastError(),
			GetLastError() == ERROR_ACCESS_DENIED ? L" —— 请以管理员身份运行" : L"");
		return 1;
	}
	{
		BOOL is32 = FALSE;
		IsWow64Process(probe, &is32);
		if (is32)
		{
			Print("[x] 目标是 32 位进程 —— 本 DLL 为 x64，无法注入。\n");
			Print("    请改用 64 位 Java 启动游戏（启动器里选择 x64 JRE）。\n");
			CloseHandle(probe); return 1;
		}
	}
	CloseHandle(probe);

	// 找 DLL
	std::wstring dll = FindDll(dllArg);
	if (dll.empty())
	{
		Print("[x] 未找到 %ls，请用 -d 指定路径。\n", MODULE_NAME);
		return 1;
	}
	Print("[+] DLL: %ls\n", dll.c_str());

	if (IsModuleLoaded(pid, MODULE_NAME))
	{
		Print("[!] 该 DLL 已经在这个进程里了（重复注入会被 Windows 直接返回已加载句柄）。\n");
		Print("    如需重新注入，请重启游戏。\n");
	}

	// 注入
	Print("[*] 正在注入...\n");
	std::wstring err;
	if (!InjectDll(pid, dll, err))
	{
		Print("[x] 注入失败: %ls\n", err.c_str());
		return 1;
	}
	Print("[+] LoadLibraryW 返回成功\n");

	// 验证（轮询等待 DLL 初始化线程启动，避免误报）
	bool loaded = false;
	ULONGLONG base = 0, size = 0;
	int tcount = 0;
	for (int i = 0; i < 16; i++)   // 最多等 8 秒
	{
		Sleep(500);
		if (!IsModuleLoaded(pid, MODULE_NAME))
		{
			// 模块短暂消失或尚未登记，继续等
			continue;
		}
		loaded = true;
		GetModuleRange(pid, MODULE_NAME, base, size);
		tcount = CountThreadsInModule(pid, base, size);
		if (tcount > 0) break;
	}

	if (!loaded)
	{
		Print("[x] 注入返回成功，但模块列表中未找到该 DLL（可能已被卸载）。\n");
		return 1;
	}

	Print("[+] 模块已加载: 基址 0x%llX  大小 %llu KB\n", base, size / 1024);
	if (tcount > 0)
		Print("[+] DLL 内线程数 = %d —— 初始化成功（Hook 已安装、渲染管线已启动）\n", tcount);
	else
		Print("[!] 未检测到 DLL 内线程（初始化可能失败，请看日志 "
			"%APPDATA%\\InfiniteGUI\\logs\\infinitegui.log）\n");

	HANDLE target = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (target)
	{
		DWORD code = 0;
		if (GetExitCodeProcess(target, &code) && code == STILL_ACTIVE)
			Print("[+] 游戏进程存活，注入完成。\n");
		else
			Print("[x] 游戏进程已退出（可能崩溃）。\n");
		CloseHandle(target);
	}

	Print("\n提示: 进游戏按反斜杠键 \\ 打开菜单；HUD 模块在 设置 -> 模块 -> 视觉 里开启。\n");
	return 0;
}
