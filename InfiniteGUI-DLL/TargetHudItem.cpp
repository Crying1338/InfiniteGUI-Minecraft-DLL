#include "TargetHudItem.h"

#include "Anim.h"
#include "GameStateDetector.h"
#include "opengl_hook.h"

#include <algorithm>
#include <cstdio>

// ============================================================
// TargetHudItem · Drip Lite 风格目标面板
// ============================================================

void TargetHudItem::Toggle()
{
}

void TargetHudItem::Reset()
{
	ResetWindow();
	isEnabled = false;

	// 默认点击跟踪：JNI 实时读取在新版 JVM + 模组加载器组合下仍属实验性，
	// 需要真实目标数据时可在设置里切到「仅 JNI 实时数据」
	dataMode = Mode_Manual;
	manualName = u8"Target";
	manualHealth = 20.0f;
	showHpText = true;
	showCombo = true;
	customBarColor = false;
	barColor = ImVec4(0.92f, 0.20f, 0.32f, 1.0f);

	{
		std::lock_guard<std::mutex> lock(snapshotMutex);
		lastJni = JniTargetSnapshot();
		jniHasTarget = false;
	}
	comboJni = 0;
	comboManual = 0;
	lastEntityId = -1;
	lastEntityHealth = -1.0f;
	targetVisible = false;
	usingJni = false;
	displayAlpha = 0.0f;
	displayHealth = 20.0f;
	displayMaxHealth = 20.0f;
	positionInitialized = false;

	itemStyle.fontSize = 20.0f;

	dirtyState.contentDirty = true;
	dirtyState.animating = true;
}

void TargetHudItem::UpdateJniTracking(std::chrono::steady_clock::time_point now, bool& jniVisible)
{
	JniTargetSnapshot snapshot;
	bool fresh = MinecraftJniReader::Instance().GetTarget(snapshot);

	if (fresh)
	{
		std::lock_guard<std::mutex> lock(snapshotMutex);
		lastJni = snapshot;
		jniHasTarget = true;
		lastJniFreshTime = now;

		// 连击：同一目标血量下降视为一次命中
		if (snapshot.entityId != lastEntityId)
		{
			comboJni = 0;
		}
		else if (lastEntityHealth > 0.0f && snapshot.health < lastEntityHealth - 0.01f)
		{
			comboJni++;
			lastHurtFlash = now;
		}
		lastEntityId = snapshot.entityId;
		lastEntityHealth = snapshot.health;
	}

	// 目标信息保留 800ms（准星短暂移开时面板不闪烁）
	jniVisible = jniHasTarget && (now - lastJniFreshTime) < std::chrono::milliseconds(800);
}

void TargetHudItem::UpdateManualTracking(std::chrono::steady_clock::time_point now, bool& manualVisible)
{
	if (GameStateDetector::Instance().IsInGameWindow() && GameStateDetector::Instance().IsInGame())
	{
		if (keyStateHelper.GetKeyClick(VK_LBUTTON))
		{
			// 超过 2 秒没有攻击则重置连击
			if (now - lastManualAttack > std::chrono::milliseconds(2000))
				comboManual = 0;
			comboManual++;
			lastManualAttack = now;
		}
	}

	manualVisible = (now - lastManualAttack) < std::chrono::milliseconds(1500);
}

void TargetHudItem::Update()
{
	auto now = std::chrono::steady_clock::now();

	bool jniVisible = false;
	bool manualVisible = false;

	if (dataMode != Mode_Manual)
		UpdateJniTracking(now, jniVisible);
	if (dataMode != Mode_Jni)
		UpdateManualTracking(now, manualVisible);

	bool newUsingJni = jniVisible || (jniHasTarget && dataMode == Mode_Jni);
	if (dataMode == Mode_Auto && !jniVisible && manualVisible)
		newUsingJni = false;

	// JNI 数据长时间不更新则丢弃（避免陈旧名称残留）
	if (jniHasTarget && !jniVisible && (now - lastJniFreshTime) > std::chrono::milliseconds(3000))
	{
		std::lock_guard<std::mutex> lock(snapshotMutex);
		jniHasTarget = false;
		lastJni = JniTargetSnapshot();
	}

	targetVisible = jniVisible || manualVisible;
	usingJni = newUsingJni && jniHasTarget;

	dirtyState.contentDirty = true;
}

void TargetHudItem::RenderGui()
{
	if (!positionInitialized)
	{
		// 默认位置：屏幕中下方
		float screenW = (float)opengl_hook::screen_size.x;
		float screenH = (float)opengl_hook::screen_size.y;
		x = screenW * 0.5f - 130.0f;
		y = screenH - 230.0f;
		positionInitialized = true;
	}

	if (displayAlpha <= 0.01f && !targetVisible)
	{
		dirtyState.animating = false;
		return; // 完全隐藏时不渲染窗口
	}

	WindowModule::RenderGui();
}

void TargetHudItem::HoverSetting()
{
}

void TargetHudItem::DrawContent()
{
	if (closed)
	{
		isEnabled = false;
		closed = false;
	}

	ImGuiIO& io = ImGui::GetIO();
	float dt = std::clamp(io.DeltaTime, 0.0f, 0.05f);
	auto now = std::chrono::steady_clock::now();

	// ---- 取当前显示数据 ----
	bool haveTarget = false;
	std::string name;
	float hp = 20.0f;
	float maxHp = 20.0f;
	int combo = 0;

	if (usingJni)
	{
		std::lock_guard<std::mutex> lock(snapshotMutex);
		if (jniHasTarget)
		{
			haveTarget = true;
			name = lastJni.name.empty() ? u8"未知实体" : lastJni.name;
			hp = lastJni.health;
			maxHp = lastJni.maxHealth;
			combo = comboJni;
		}
	}
	if (!haveTarget)
	{
		// 点击跟踪模式
		if ((now - lastManualAttack) < std::chrono::milliseconds(1500))
		{
			haveTarget = true;
			name = manualName.empty() ? u8"Target" : manualName;
			hp = manualHealth;
			maxHp = 20.0f;
			combo = comboManual;
		}
	}

	// ---- 动画推进 ----
	Anim::SmoothLerp(displayAlpha, targetVisible && haveTarget ? 1.0f : 0.0f, 12.0f, dt);
	Anim::SmoothLerp(displayHealth, hp, 10.0f, dt);
	Anim::SmoothLerp(displayMaxHealth, maxHp, 10.0f, dt);

	bool animating = !Anim::AlmostEqual(displayAlpha, (targetVisible && haveTarget) ? 1.0f : 0.0f, 0.005f)
		|| !Anim::AlmostEqual(displayHealth, hp, 0.05f);
	if (animating)
		dirtyState.animating = true;
	else
		dirtyState.animating = false;

	if (displayAlpha <= 0.01f || !haveTarget)
		return;

	const float alpha = displayAlpha;
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 windowPos = ImGui::GetWindowPos();
	const auto vadd = [](const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); };

	float fontSize = ImGui::GetFontSize();
	float pad = 10.0f;
	float barHeight = 8.0f;
	float panelWidth = 240.0f;

	bool showComboRow = showCombo && combo > 0;
	ImVec2 nameSize = ImGui::CalcTextSize(name.c_str());
	char hpText[32];
	snprintf(hpText, sizeof(hpText), "%.1f", displayHealth);
	ImVec2 hpSize = ImGui::CalcTextSize(hpText);

	float contentHeight = pad + fontSize + 8.0f + barHeight + pad;
	if (showComboRow)
		contentHeight += fontSize * 0.9f + 2.0f;

	// 撑起窗口大小
	ImGui::Dummy(ImVec2(panelWidth, contentHeight));

	float panelAlpha = 0.78f * alpha;
	float slideOffset = (1.0f - alpha) * 14.0f;

	// ---- 面板背景 ----
	ImVec2 pMin = vadd(windowPos, ImVec2(0.0f, slideOffset));
	ImVec2 pMax = vadd(windowPos, ImVec2(panelWidth, contentHeight + slideOffset));
	ImVec4 panelBg = ImVec4(0.055f, 0.055f, 0.085f, panelAlpha);
	ImVec4 panelBorderColor = ImVec4(barColor.x, barColor.y, barColor.z, 0.55f * alpha);
	drawList->AddRectFilled(pMin, pMax, ImGui::GetColorU32(panelBg), 6.0f);
	drawList->AddRect(pMin, pMax, ImGui::GetColorU32(panelBorderColor), 6.0f, 0, 1.0f);

	// ---- 名称（带阴影）----
	ImVec2 namePos = vadd(pMin, ImVec2(pad, pad - 2.0f));
	drawList->AddText(vadd(namePos, ImVec2(1.0f, 1.0f)),
		ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.85f * alpha)), name.c_str());
	drawList->AddText(namePos,
		ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, alpha)), name.c_str());

	// ---- HP 数字（右侧）----
	if (showHpText)
	{
		ImVec2 hpPos = vadd(pMin, ImVec2(panelWidth - pad - hpSize.x, pad - 2.0f));
		drawList->AddText(vadd(hpPos, ImVec2(1.0f, 1.0f)),
			ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.85f * alpha)), hpText);
		drawList->AddText(hpPos,
			ImGui::GetColorU32(ImVec4(0.95f, 0.95f, 0.95f, alpha)), hpText);
	}

	// ---- 血量条 ----
	ImVec2 barMin = vadd(pMin, ImVec2(pad, pad + fontSize + 6.0f));
	ImVec2 barMax = vadd(pMin, ImVec2(panelWidth - pad, pad + fontSize + 6.0f + barHeight));

	// 血条底
	drawList->AddRectFilled(barMin, barMax,
		ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.55f * alpha)), 3.0f);

	// 血条填充（红 -> 绿 按血量比例）
	float fraction = std::clamp(displayHealth / (displayMaxHealth > 1.0f ? displayMaxHealth : 20.0f), 0.0f, 1.0f);
	ImVec4 lowColor = ImVec4(0.92f, 0.18f, 0.25f, 1.0f);
	ImVec4 highColor = ImVec4(0.24f, 0.90f, 0.42f, 1.0f);
	ImVec4 fillColor;
	if (customBarColor)
	{
		fillColor = barColor;
	}
	else
	{
		fillColor = ImVec4(
			lowColor.x + (highColor.x - lowColor.x) * fraction,
			lowColor.y + (highColor.y - lowColor.y) * fraction,
			lowColor.z + (highColor.z - lowColor.z) * fraction,
			1.0f);
	}

	// 受击闪白
	float sinceHurt = std::chrono::duration<float>(now - lastHurtFlash).count();
	if (sinceHurt < 0.18f)
	{
		float flash = 1.0f - sinceHurt / 0.18f;
		fillColor = ImVec4(
			fillColor.x + (1.0f - fillColor.x) * flash,
			fillColor.y + (1.0f - fillColor.y) * flash,
			fillColor.z + (1.0f - fillColor.z) * flash,
			1.0f);
	}

	float fillWidth = (barMax.x - barMin.x) * fraction;
	if (fillWidth > 1.0f)
	{
		ImVec4 fillHi = ImVec4((std::min)(fillColor.x * 1.35f + 0.08f, 1.0f),
			(std::min)(fillColor.y * 1.35f + 0.08f, 1.0f),
			(std::min)(fillColor.z * 1.35f + 0.08f, 1.0f), 1.0f);
		ImVec2 fillMin = barMin;
		ImVec2 fillMax = ImVec2(barMin.x + fillWidth, barMax.y);
		drawList->AddRectFilledMultiColor(fillMin, fillMax,
			ImGui::GetColorU32(ImVec4(fillHi.x, fillHi.y, fillHi.z, alpha)),
			ImGui::GetColorU32(ImVec4(fillHi.x, fillHi.y, fillHi.z, alpha)),
			ImGui::GetColorU32(ImVec4(fillColor.x, fillColor.y, fillColor.z, alpha)),
			ImGui::GetColorU32(ImVec4(fillColor.x, fillColor.y, fillColor.z, alpha)));
	}

	// ---- 连击 ----
	if (showComboRow)
	{
		char comboText[48];
		snprintf(comboText, sizeof(comboText), u8"连击 x %d", combo);
		ImVec2 comboPos = ImVec2(pMin.x + pad, barMax.y + 4.0f);
		drawList->AddText(ImVec2(comboPos.x + 1.0f, comboPos.y + 1.0f),
			ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.85f * alpha)), comboText);
		drawList->AddText(comboPos,
			ImGui::GetColorU32(ImVec4(barColor.x, barColor.y, barColor.z, alpha)), comboText);
	}
}

void TargetHudItem::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
	float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

	ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
	ImGui::BeginDisabled();
	ImGuiStd::TextShadow(u8"TargetHUD 设置");
	ImGui::EndDisabled();
	ImGui::PopFont();

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(bigItemWidth);
	const char* modeNames[] = { u8"自动 (JNI 优先)", u8"仅 JNI 实时数据", u8"仅点击跟踪" };
	ImGui::Combo(u8"数据来源", &dataMode, modeNames, IM_ARRAYSIZE(modeNames));
	ImGui::SameLine();
	ImGuiStd::HelpMarker(u8"JNI 模式通过注入的 JVM 读取准星指向的目标信息（需要 Forge 1.17+ / NeoForge 等使用官方映射的运行时）。点击跟踪模式在左键攻击时显示面板，名称与血量由下方设置提供。");

	// JNI 状态
	{
		ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
		ImGui::SetCursorPosX(bigPadding);
		MinecraftJniReader::Status st = MinecraftJniReader::Instance().GetStatus();
		ImVec4 stColor = st == MinecraftJniReader::Status_Ready
			? ImVec4(0.3f, 1.0f, 0.4f, 1.0f)
			: (st == MinecraftJniReader::Status_NotTried ? ImVec4(0.7f, 0.7f, 0.7f, 1.0f) : ImVec4(1.0f, 0.45f, 0.4f, 1.0f));
		ImGuiStd::TextColoredShadow(stColor, u8"JNI 状态：%s", MinecraftJniReader::Instance().GetStatusText());
		ImGui::PopFont();
	}

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(bigItemWidth);
	ImGuiStd::InputTextStd(u8"跟踪目标名称", manualName);
	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(bigItemWidth);
	ImGui::SliderFloat(u8"手动血量", &manualHealth, 0.0f, 40.0f, "%.1f");

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::Checkbox(u8"显示血量数字", &showHpText);
	ImGui::SameLine();
	ImGui::SetCursorPosX(bigPadding + centerX);
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::Checkbox(u8"显示连击数", &showCombo);

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::Checkbox(u8"自定义血条颜色", &customBarColor);
	if (customBarColor)
	{
		ImGui::SameLine();
		ImGui::SetCursorPosX(bigPadding + centerX);
		ImGui::SetNextItemWidth(itemWidth);
		ImGuiStd::EditColor(u8"血条颜色", barColor);
	}

	DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void TargetHudItem::Load(const nlohmann::json& j)
{
	LoadItem(j);
	LoadWindow(j);
	if (j.contains("dataMode")) dataMode = j["dataMode"];
	if (j.contains("manualName")) manualName = j["manualName"].get<std::string>();
	if (j.contains("manualHealth")) manualHealth = j["manualHealth"];
	if (j.contains("showHpText")) showHpText = j["showHpText"];
	if (j.contains("showCombo")) showCombo = j["showCombo"];
	if (j.contains("customBarColor")) customBarColor = j["customBarColor"];
	ImGuiStd::LoadImVec4(j, "barColor", barColor);
}

void TargetHudItem::Save(nlohmann::json& j) const
{
	SaveItem(j);
	SaveWindow(j);
	j["dataMode"] = dataMode;
	j["manualName"] = manualName;
	j["manualHealth"] = manualHealth;
	j["showHpText"] = showHpText;
	j["showCombo"] = showCombo;
	j["customBarColor"] = customBarColor;
	ImGuiStd::SaveImVec4(j, "barColor", barColor);
}
