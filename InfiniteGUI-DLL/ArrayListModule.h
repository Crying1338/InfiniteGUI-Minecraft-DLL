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
// Drip Lite 风格模块列表：在屏幕边缘显示所有已开启的模块
// 深色半透明条 + 彩色强调边，按宽度/字母排序，开关时有滑动淡入淡出动画
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
		Sort_Width = 0, // 按文字宽度排序（Drip 风格，长在上）
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
	ImVec4 accentColor = ImVec4(0.92f, 0.20f, 0.32f, 1.0f); // Drip 红
	bool gradientAccent = true;
	bool rainbowAccent = false;
	float barWidth = 220.0f;

	bool positionInitialized = false;
};
