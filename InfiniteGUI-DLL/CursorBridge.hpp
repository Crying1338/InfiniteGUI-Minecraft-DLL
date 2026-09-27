#pragma once

#include <Windows.h>

// ============================================================
// CursorBridge —— 通过 GLFW 释放/抓取鼠标
//
// 为什么需要：
//   Minecraft（LWJGL3/GLFW）在游戏内用 glfwSetInputMode(GLFW_CURSOR_DISABLED)
//   把系统光标抓死；此时 ImGui 收不到鼠标位置，菜单虽然能画出来但点不动。
//   打开菜单时必须把 GLFW 光标切成 GLFW_CURSOR_NORMAL，关闭时再恢复。
//
// 说明：
//   1. 用 GetModuleHandle("glfw.dll") + GetProcAddress 直接调用，
//      不需要解析 Java 侧的 Window.handle，因此不依赖 JNI。
//   2. glfwGetCurrentContext() 返回当前线程的 GLFW 窗口；
//      我们自己用 wglMakeCurrent 切换上下文不会影响 GLFW 的线程跟踪。
//   3. 只在状态变化时调用一次，避免每帧开销。
// ============================================================
namespace CursorBridge
{
	inline HMODULE g_glfw = nullptr;
	inline void* (*g_getCurrentContext)() = nullptr;
	inline void (*g_setInputMode)(void*, int, int) = nullptr;
	inline bool g_loaded = false;
	inline bool g_tried = false;
	inline bool g_released = false;   // 当前是否是我们把光标放开的

	inline constexpr int GLFW_CURSOR = 0x00033001;
	inline constexpr int GLFW_CURSOR_NORMAL = 0x00034001;
	inline constexpr int GLFW_CURSOR_DISABLED = 0x00034003;

	inline bool EnsureLoaded()
	{
		if (g_tried) return g_loaded;
		g_tried = true;

		g_glfw = GetModuleHandleW(L"glfw.dll");
		if (!g_glfw)
			return false;

		g_getCurrentContext = reinterpret_cast<void* (*)()>(
			GetProcAddress(g_glfw, "glfwGetCurrentContext"));
		g_setInputMode = reinterpret_cast<void (*)(void*, int, int)>(
			GetProcAddress(g_glfw, "glfwSetInputMode"));

		g_loaded = (g_getCurrentContext != nullptr && g_setInputMode != nullptr);
		return g_loaded;
	}

	// 菜单打开 -> 放开光标（NORMAL）；菜单关闭 -> 恢复抓取（DISABLED）
	inline void SetMenuCursor(bool menuOpen)
	{
		if (!EnsureLoaded()) return;
		// 打开时每帧强制放开：Minecraft 在若干事件里会把光标重新抓回去，
		// 只改一次会出现“有时候能点、有时候点不了”
		if (menuOpen)
		{
			void* window = g_getCurrentContext();
			if (window)
				g_setInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			g_released = true;
			return;
		}

		// 关闭时只恢复一次
		if (!g_released) return;
		{
			void* window = g_getCurrentContext();
			if (window)
				g_setInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			g_released = false;
		}
	}
}
