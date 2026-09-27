#pragma once

#include <Windows.h>
#include <string>
#include <map>
#include <mutex>
#include <cstdarg>
#include <cstdio>

#include "FileUtils.h"

// ============================================================
// 统一日志：%APPDATA%\InfiniteGUI\logs\infinitegui.log
// 用于排查注入/渲染/崩溃问题（含未捕获 C++ 异常记录）
// ============================================================
inline void InfGuiLog(const char* fmt, ...)
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

// 限流日志：同一 tag 前 5 次 + 之后每 100 次记录一条，避免异常刷屏
inline void InfGuiLogLimited(const char* tag, const char* detail)
{
	static std::mutex s_mutex;
	static std::map<std::string, int> s_counts;

	std::lock_guard<std::mutex> lock(s_mutex);
	int& count = s_counts[tag];
	count++;
	if (count <= 5 || (count % 100) == 0)
		InfGuiLog("[%s] 未捕获 C++ 异常（累计 %d 次）: %s", tag, count, detail);
}

