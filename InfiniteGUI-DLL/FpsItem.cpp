#include "FpsItem.h"

#include "Anim.h"
#include "ImGuiStd.h"


void FpsItem::Toggle()
{
}

void FpsItem::Update()
{
	FPS = float(frameCount) / (float(updateIntervalMs) / 1000.0f);
	frameCount = 0;
	if (showGuiFPS)
	{
		guiFPS = float(guiFrameCount) / (float(updateIntervalMs) / 1000.0f);
		guiFrameCount = 0;
	}
	dirtyState.contentDirty = true;
}

void FpsItem::HoverSetting()
{
}

void FpsItem::DrawContent()
{
	if (closed)
	{
		isEnabled = false;
		closed = false;
	}
	guiFrameCount++; //gui帧率
	int FPS = int(this->FPS);
	std::string fpsText;
	if (showGuiFPS)
	{
		int guiFps = int(this->guiFPS);
		fpsText = prefix + std::to_string(FPS) + " | " + std::to_string(guiFps) + suffix;
	}
	else
		fpsText = prefix + std::to_string(FPS) + suffix;

	ImVec4 targetTextColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
	//获取io
	ImGuiIO& io = ImGui::GetIO();
	//计算速度
	float speed = 3.0f * std::clamp(io.DeltaTime, 0.0f, 0.05f);
	color.color = ImLerp(color.color, targetTextColor, speed);
	// 判断动画是否结束
	if (Anim::AlmostEqual(color.color, targetTextColor))
	{
		color.color = targetTextColor;
		dirtyState.animating = false;
	}
	ImGuiStd::TextColoredShadow(color.color, fpsText.c_str());
}

void FpsItem::RenderBeforeGui()
{
	frameCount++; //游戏帧率
	auto now = Clock::now();
	deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastFrameTime).count();
	lastFrameTime = now;
}

void FpsItem::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
	//DrawItemSettings();
	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::Checkbox(u8"显示无限Gui的FPS", &showGuiFPS);
	ImGui::SameLine(); ImGuiStd::HelpMarker(
		u8"开启限帧优化后适用，显示无限Gui的ui计算频率。");
	DrawAffixSettings(bigPadding, centerX, itemWidth);
	DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void FpsItem::Load(const nlohmann::json& j)
{
	LoadItem(j);
	LoadAffix(j);
	LoadWindow(j);
	if (j.contains("showGuiFPS")) showGuiFPS = j.at("showGuiFPS").get<bool>();

}
void FpsItem::Save(nlohmann::json& j) const
{
	SaveItem(j);
	SaveAffix(j);
	SaveWindow(j);
	j["showGuiFPS"] = showGuiFPS;
}