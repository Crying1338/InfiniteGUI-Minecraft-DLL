#include "opengl_hook.h"
#include "FileUtils.h"
#include "ConfigManager.h"
#include "AudioManager.h"
#include <thread>
#include <atomic>

#include "App.h"
#include "ClickSound.h"
#include "GameKeyBind.h"
#include "HttpUpdateWorker.h"
#include "ItemManager.h"
#include "GuiFrameLimiter.h"
#include "NotificationItem.h"
inline HMODULE g_hModule = NULL;
inline std::thread g_updateThread;
inline bool g_uninitialized = false;

inline static std::atomic_bool g_running = ATOMIC_VAR_INIT(true);
// 线程函数：更新所有 item 状态
inline void UpdateThread() {
	while (g_running.load()) {
		if(opengl_hook::gui.isInit) ItemManager::Instance().UpdateAll();  // 调用UpdateAll()来更新所有item
		std::this_thread::sleep_for(std::chrono::milliseconds(1));  // 休眠1ms，可以根据实际需求调整
	}
}

// 启动更新线程
inline void StartThreads() {
	g_updateThread = std::thread(UpdateThread);
	g_updateThread.detach();  // 将线程设为后台线程

}

// 停止更新线程
inline void StopThreads() {
	if (g_updateThread.joinable()) {
		g_updateThread.join();
	}
}

inline void Uninit() {
	if (g_uninitialized) return;
	opengl_hook::remove_hook();
	opengl_hook::clean();
	g_running = false;
	StopThreads();
	AudioManager::Instance().Shutdown();
	g_uninitialized = true;
}


inline DWORD WINAPI MainApp(LPVOID)
{
    FileUtils::InitPaths(g_hModule);
	//加载配置文件
	ConfigManager::Instance().Init();
	ConfigManager::Instance().LoadGlobal();
	GuiFrameLimiter::Instance().Init();
    opengl_hook::init();
	while (!opengl_hook::gui.isInit)
	{
		std::this_thread::yield();
	}
	ConfigManager::Instance().LoadProfile();
	//初始化音频管理器
	AudioManager::Instance().Init();
	ClickSound::PlayIntroSound();
	StartThreads();
	App::Instance().GetAnnouncement();
	GameKeyBind::Instance().Load(FileUtils::optionsPath);
	if(!GameKeyBind::Instance().IsSuccess())
		NotificationItem::Instance().AddNotification(NotificationType_Warning, u8"读取游戏快捷键失败！\n请在设置中手动绑定游戏快捷键。", 10000);
	while (!opengl_hook::gui.done)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (opengl_hook::handle_window != WindowFromDC(opengl_hook::handle_device_ctx))
		{
			opengl_hook::lwjgl2FullscreenHandler();
		}
	}
	if(opengl_hook::exitByMenu) std::this_thread::sleep_for(std::chrono::milliseconds(300));
	Uninit();
	FreeLibraryAndExitThread(g_hModule, 0);
}