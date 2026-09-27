#pragma once

#include <string>
#include <vector>
#include <mutex>

#include "Item.h"
#include "WindowModule.h"
#include "UpdateModule.h"
#include "ImGuiStd.h"
#include <nlohmann/json.hpp>

// ============================================================
// ArrayListModule
// Drip 风格模块列表：在屏幕边缘显示所有已开启的模块
//   渐变底条 + 白色文字，按宽度或名称排序，开关有滑动淡入淡出动画
//   默认配色为紫色渐变（可改成任意两色渐变 / 纯色 / 彩虹）
// ============================================================

class ArrayListModule : public Item, public WindowModule, public UpdateModule
{
public:
	enum ListSide
	{
		Side_Right = 0,
		Side_Left = 1,
	};

	enum SortMode
	{
		Sort_Width = 0,    // 按文字宽度排序（Drip 风格：长在上）
		Sort_Alphabet = 1, // 按名称排序
	};

	enum StyleMode
	{
		Style_Bars = 0, // 渐变底条 + 白字
		Style_Rise = 1, // Rise Modern：彩色渐变文字 + 右侧细高亮条（默认）
	};

	ArrayListModule() {
		type = Visual;
		name = u8"ArrayList";
		description = u8"Drip 风格模块列表：在屏幕边缘显示当前开启的模块";
		icon = "L";
		updateIntervalMs = 100;
		lastUpdateTime = std::chrono::steady_clock::now();
		ArrayListModule::Reset();
	}

	static ArrayListModule& Instance() {
		static ArrayListModule instance;
		return instance;
	}

	void Toggle() override;
	void Reset() override;
	void Update() override;
	void HoverSetting() override;
	void DrawContent() override;
	void RenderGui() override;
	void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override;
	void Load(const nlohmann::json& j) override;
	void Save(nlohmann::json& j) const override;

private:
	struct Entry
	{
		const Item* item = nullptr;
		std::string name;
		float anim = 0.0f;   // 0 = 隐藏, 1 = 完全显示
		bool visible = false;
	};

	void RebuildEntries();
	ImVec4 GetAccentColor() const;

	std::mutex entriesMutex;
	std::vector<Entry> entries;
	std::string lastSignature;

	int listSide = Side_Right;
	int sortMode = Sort_Width;
	int styleMode = Style_Rise;   // 默认 Rise Modern 观感

	// ---- 配色（默认 Rise 的 MAGIC 紫色渐变 #4A00E0 -> #8E2DE2）----
	ImVec4 gradientStart = ImVec4(0.290f, 0.000f, 0.878f, 1.00f); // #4A00E0
	ImVec4 gradientEnd = ImVec4(0.557f, 0.176f, 0.886f, 1.00f);   // #8E2DE2
	bool useGradient = true;
	bool gradientAcrossList = true;  // 整列共用一段渐变（Rise 的流动效果）
	bool gradientAnimated = false;   // 渐变随时间流动
	float gradientSpeed = 0.05f;
	bool rainbowAccent = false;

	// ---- Rise Modern 风格选项 ----
	bool textGradient = true;        // 文字使用渐变彩色（关闭则白色）
	bool showSidebar = true;         // 文字右侧的细高亮条
	bool entryBackground = false;    // 每条深色圆角底
	ImVec4 entryBgColor = ImVec4(0.078f, 0.055f, 0.118f, 0.43f); // #14121E

	float barWidth = 220.0f;
	float barRounding = 0.0f;
	float rowSpacing = 2.0f;         // 条目间距
	bool positionInitialized = false;
};
