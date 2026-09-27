#include "gui.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_win32.h"
#include "opengl_hook.h"

#include <GL/glew.h>
#include <GL/GL.h>

#include "fonts.h"
#include "ImGuiSty.h"
#include "ItemManager.h"
#include "Menu.h"
#include "GlobalConfig.h"
#include "GuiFrameLimiter.h"
#include "FileUtils.h"

#include <cstdarg>
#include <cstdio>

// ------------------------------------------------------------
// 轻量日志：写入 %APPDATA%\InfiniteGUI\logs\infinitegui.log
// 用于排查注入/渲染问题（渲染是否启动、菜单是否开关、GL 版本等）
// ------------------------------------------------------------
static void GuiLog(const char* fmt, ...)
{
	char msg[1024];
	va_list ap; va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	std::string base = FileUtils::appDataPath.empty() ? std::string(".") : FileUtils::appDataPath;
	CreateDirectoryA((base + "\\logs").c_str(), nullptr);
	std::string path = base + "\\logs\\infinitegui.log";

	SYSTEMTIME st; GetLocalTime(&st);
	char line[1280];
	snprintf(line, sizeof(line), "[%04d-%02d-%02d %02d:%02d:%02d] %s\r\n",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, msg);

	HANDLE f = CreateFileA(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ,
		nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (f != INVALID_HANDLE_VALUE)
	{
		DWORD written = 0;
		WriteFile(f, line, (DWORD)strlen(line), &written, nullptr);
		CloseHandle(f);
	}
}

static ImGuiContext* imGuiContext = nullptr;
static CachedDrawData g_Cache;
void Gui::init()
{
	imGuiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 可选
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // 可选

	// 默认关闭 ImGui 捕获鼠标（只有激活 UI 时允许）
	//io.MouseDrawCursor = false;
	//io.ConfigFlags |= ImGuiConfigFlags_NoMouse; // 禁止 ImGui 捕获鼠标输入（我们在切换时会调整）
	io.IniFilename = nullptr; // 禁止生成 imgui.ini
	SetStyleGray();

	ImGui_ImplWin32_Init(opengl_hook::handle_window);
	ImGui_ImplOpenGL3_Init();
	//ImFontConfig config{};
	//config.FontDataOwnedByAtlas = false;
	//font = io.Fonts->AddFontFromMemoryTTF(Fonts::harmony.data, Fonts::harmony.size, 17.f, &config, io.Fonts->GetGlyphRangesChineseFull());

	ImFontConfig font_cfg;
	font_cfg.FontDataOwnedByAtlas = false;
	font_cfg.OversampleH = 1;
	font_cfg.OversampleV = 1;
	font_cfg.PixelSnapH = true;

	if (GlobalConfig::Instance().fontPath == "default")
		font = io.Fonts->AddFontFromMemoryTTF(Fonts::alibaba.data, Fonts::alibaba.size, 20.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
	else
		font = io.Fonts->AddFontFromFileTTF(GlobalConfig::Instance().fontPath.c_str(), 20.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
	if (font == nullptr) {
		{
			MessageBox(NULL, L"字体加载失败", L"提示", MB_OK);
			font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 20.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
		}
	}
	iconFont = io.Fonts->AddFontFromMemoryTTF(Fonts::icons.data, Fonts::icons.size, 20.0f, &font_cfg);


	io.FontDefault = font;

	if (const GLenum err = glewInit(); GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		GuiLog("gui.init: glewInit FAILED: %s", glewGetErrorString(err));
	}
	else
	{
		GuiLog("gui.init ok: GL_VERSION=%s, GL_RENDERER=%s, font=%s",
			(const char*)glGetString(GL_VERSION),
			(const char*)glGetString(GL_RENDERER),
			font ? "ok" : "FAILED");
	}
	isInit = true;
}
void Gui::clean()
{
	if(!isInit) return;
	isInit = false;
	while (opengl_hook::rendering)
	{
	}
	g_Cache.Clear();
	if (imGuiContext)ImGui::GetIO().Fonts->Clear();
	if ((ImGui::GetCurrentContext() ? (void*)ImGui::GetIO().BackendRendererUserData : nullptr))ImGui_ImplOpenGL3_Shutdown();
	if ((ImGui::GetCurrentContext() ? (void*)ImGui::GetIO().BackendPlatformUserData : nullptr))ImGui_ImplWin32_Shutdown();
	if (imGuiContext)ImGui::DestroyContext(imGuiContext);

}

static std::atomic_flag clipCursor = ATOMIC_FLAG_INIT;
static RECT originalClip;

// ============================================================
// 每帧渲染：由 opengl_hook 的 wglSwapBuffers detour 在“自有 GL 上下文”中调用
//   1. 静止帧复用上一帧 draw data（GlobalConfig::enableOptimization，降低自身开销）
//   2. 否则重建 ImGui 帧：NewFrame -> 各模块渲染 -> Render -> 提交绘制
//   3. 菜单打开时由 ImGui 自绘光标；关闭时恢复游戏原本光标
// ============================================================
void Gui::render()
{
	if (!isInit) return;

	ImGuiIO& io = ImGui::GetIO();
	const bool menuOpen = Menu::Instance().isEnabled;
	const bool optimize = GlobalConfig::Instance().enableOptimization;
	const bool dirty = ItemManager::Instance().IsDirty();

	// 首帧 / 菜单开关 记录日志，便于排查（见 %APPDATA%\InfiniteGUI\logs\infinitegui.log）
	static bool loggedFirstFrame = false;
	static bool loggedMenuState = false;
	if (menuOpen != loggedMenuState)
	{
		loggedMenuState = menuOpen;
		GuiLog("menu %s", menuOpen ? "OPENED" : "CLOSED");
	}

	// 静止帧直接复用缓存：不调用任何 ImGui 帧函数，仅重放上一帧三角形
	if (optimize && !menuOpen && !dirty && g_Cache.drawData.CmdListsCount > 0)
	{
		RenderCachedDrawData(g_Cache);
		return;
	}

	// 菜单打开时游戏会隐藏系统光标，改由 ImGui 自绘
	HCURSOR prevCursor = GetCursor();
	io.MouseDrawCursor = menuOpen;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();

	// 窗口尺寸兜底（全屏切换/客户区读取失败时）
	if (opengl_hook::screen_size.x > 0 && opengl_hook::screen_size.y > 0)
		io.DisplaySize = ImVec2((float)opengl_hook::screen_size.x, (float)opengl_hook::screen_size.y);

	// 菜单关闭时恢复游戏原本的光标，避免 Win32 后端每帧 SetCursor(nullptr) 干扰游戏 GUI
	if (!menuOpen) SetCursor(prevCursor);

	ImGui::NewFrame();

	ItemManager::Instance().RenderAllBeforeGui();
	ItemManager::Instance().RenderAllGui();
	ItemManager::Instance().RenderAllAfterGui();

	ImGui::Render();

	if (!loggedFirstFrame)
	{
		loggedFirstFrame = true;
		GuiLog("first frame rendered: %.0fx%.0f, drawLists=%d, optimize=%d, items=%d",
			io.DisplaySize.x, io.DisplaySize.y,
			ImGui::GetDrawData() ? ImGui::GetDrawData()->CmdListsCount : -1,
			(int)optimize, (int)ItemManager::Instance().GetItems().size());
	}

	if (optimize)
	{
		CacheDrawData(g_Cache, ImGui::GetDrawData());
		RenderCachedDrawData(g_Cache);
	}
	else
	{
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}
