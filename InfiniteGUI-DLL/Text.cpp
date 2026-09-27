#include "Text.h"

#include "Anim.h"
#include "ImGuiStd.h"

void Text::Toggle()
{
}

void Text::HoverSetting()
{
}

void Text::DrawContent()
{
    if (closed)
    {
        isEnabled = false;
        closed = false;
    }
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
    ImGuiStd::TextColoredShadow(color.color, (prefix + text + suffix).c_str());
}

void Text::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;
    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Checkbox(u8"启用", &isEnabled);

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(bigItemWidth);
    ImGuiStd::InputTextStd(u8"文本内容", text);
    DrawAffixSettings(bigPadding, centerX, itemWidth);
    DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void Text::Load(const nlohmann::json& j)
{
    LoadAffix(j);
    LoadWindow(j);
    if (j.contains("Enabled")) isEnabled = j["Enabled"];
    if (j.contains("text")) text = j["text"];
}

void Text::Save(nlohmann::json& j) const
{
    SaveWindow(j);
    SaveAffix(j);
    j["Enabled"] = isEnabled;
    j["text"] = text;
}