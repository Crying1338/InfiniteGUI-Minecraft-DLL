#pragma once

#include <string>
#include <mutex>
#include <chrono>

#include "Item.h"
#include "WindowModule.h"
#include "UpdateModule.h"
#include "KeyState.h"
#include "ImGuiStd.h"
#include "MinecraftJniReader.h"
#include <nlohmann/json.hpp>

// ============================================================
// TargetHudItem
// Drip Lite 风格目标信息面板：
//   - 目标名称
//   - 血量条（平滑动画 + 受击闪白）
//   - HP 数字
//   - 连击计数
// 数据来源：
//   - JNI 实时读取（Forge 1.17+/NeoForge 官方映射运行时）
//   - 点击跟踪（纯叠加，攻击时显示，名称/血量由设置提供）
// ============================================================

class TargetHudItem : public Item, public WindowModule, public UpdateModule
{
public:
	enum DataMode
	{
		Mode_Auto = 0,   // 自动：JNI 优先，失败回退点击跟踪
		Mode_Jni = 1,    // 仅 JNI 实时数据
		Mode_Manual = 2, // 仅点击跟踪
	};

	TargetHudItem() {
		type = Visual;
		name = u8"TargetHUD";
		description = u8"Drip 风格目标面板：显示目标名称、血量与连击";
		icon = "T";
		updateIntervalMs = 50;
		lastUpdateTime = std::chrono::steady_clock::now();
		TargetHudItem::Reset();
	}

	static TargetHudItem& Instance() {
		static TargetHudItem instance;
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
	void UpdateJniTracking(std::chrono::steady_clock::time_point now, bool& jniVisible);
	void UpdateManualTracking(std::chrono::steady_clock::time_point now, bool& manualVisible);

	// ---- 数据快照 ----
	std::mutex snapshotMutex;
	JniTargetSnapshot lastJni;
	bool jniHasTarget = false;
	std::chrono::steady_clock::time_point lastJniFreshTime{};

	// ---- 连击 ----
	int comboJni = 0;
	int comboManual = 0;
	int lastEntityId = -1;
	float lastEntityHealth = -1.0f;
	std::chrono::steady_clock::time_point lastHurtFlash{};

	// ---- 点击跟踪 ----
	KeyState keyStateHelper;
	std::chrono::steady_clock::time_point lastManualAttack{};

	// ---- 渲染状态（渲染线程推进）----
	bool targetVisible = false;   // Update 线程写入
	bool usingJni = false;        // 当前显示的数据来源
	float displayAlpha = 0.0f;
	float displayHealth = 20.0f;
	float displayMaxHealth = 20.0f;

	// ---- 设置 ----
	int dataMode = Mode_Auto;   // 默认自动：优先 JNI 读取准星前的玩家，失败回退点击跟踪
	std::string manualName = u8"Target";
	float manualHealth = 20.0f;
	bool showHpText = true;
	bool showCombo = true;
	bool customBarColor = false;
	ImVec4 barColor = ImVec4(0.92f, 0.20f, 0.32f, 1.0f);

	bool positionInitialized = false;
};
