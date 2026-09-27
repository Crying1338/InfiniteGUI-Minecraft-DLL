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

	// ---- 配色（默认紫色渐变）----
	ImVec4 gradientStart = ImVec4(0.36f, 0.09f, 0.92f, 0.90f); // 深紫
	ImVec4 gradientEnd = ImVec4(0.78f, 0.38f, 1.00f, 0.90f);   // 亮紫
	bool useGradient = true;
	bool gradientAcrossList = false; // true = 整列共用一段渐变（每根条取一段）
	bool rainbowAccent = false;

	float barWidth = 220.0f;
	float barRounding = 0.0f;
	bool positionInitialized = false;
};
