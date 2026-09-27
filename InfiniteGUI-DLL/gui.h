#pragma once
#include <chrono>
#include <vector>
#include "imgui/imgui.h"
#include <GL/glew.h>
#include <GL/GL.h>
#include "imgui/imgui_impl_opengl3.h"

struct Texture
{
	GLuint id = 0;
	int width;
	int height;
};

struct CachedDrawData
{
	ImDrawData drawData;                     // 拷贝后的 drawData
	std::vector<ImDrawList*> drawLists;      // 指针表
	std::vector<ImDrawList> drawListStorage; // 真正的数据
	void Clear()
	{
		drawData.Clear();
		drawLists.clear();
		drawListStorage.clear();
	}
};

class Gui
{
public:
	bool isInit = false;
	bool done = false;
	void init();
	void clean();
	void render();
	ImFont* font;
	ImFont* iconFont;
	Texture logoTexture;

private:
	static void CacheDrawData(CachedDrawData& cache, ImDrawData* src)
	{
		cache.drawListStorage.clear();
		cache.drawData = *src;

		cache.drawListStorage.reserve(src->CmdListsCount);
		cache.drawData.CmdLists.clear();

		for (int i = 0; i < src->CmdListsCount; i++)
		{
			ImDrawList* srcList = src->CmdLists[i];

			ImDrawList dstList(srcList->_Data);

			dstList.VtxBuffer = srcList->VtxBuffer;
			dstList.IdxBuffer = srcList->IdxBuffer;
			dstList.CmdBuffer = srcList->CmdBuffer;

			cache.drawListStorage.push_back(std::move(dstList));
		}

		// 重建 CmdLists（ImVector）
		for (auto& list : cache.drawListStorage)
		{
			cache.drawData.CmdLists.push_back(&list);
		}

		cache.drawData.CmdListsCount = (int)cache.drawData.CmdLists.size();
	}
	
	static void RenderCachedDrawData(CachedDrawData& cache)
	{
		if (cache.drawData.CmdListsCount == 0)
			return;

		ImGui_ImplOpenGL3_RenderDrawData(&cache.drawData);
	}
};

