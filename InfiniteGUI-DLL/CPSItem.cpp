#include "CPSItem.h"

#include "Anim.h"
#include "App.h"
#include "CPSDetector.h"
void CPSItem::Toggle()
{
}

void CPSItem::Update()
{
    if (showLeft && cpsLeft != CPSDetector::Instance().GetLeftCPS())
    {
        cpsLeft = CPSDetector::Instance().GetLeftCPS();
    }
    else if (showRight && cpsRight != CPSDetector::Instance().GetRightCPS())
    {
        cpsRight = CPSDetector::Instance().GetRightCPS();
    }
    else return;

    dirtyState.contentDirty = true;
}

void CPSItem::HoverSetting()
{
}

void CPSItem::DrawContent()
{
    if (closed)
    {
        isEnabled = false;
        closed = false;
    }
    bool needMiddleFix = showLeft && showRight;
    //std::string addon = std::to_string(Motionblur::Instance().cur_blurriness_value);
    std::string middleFix = needMiddleFix ? " | " : "";
    std::string left = showLeft ? std::to_string(cpsLeft) : "";
    std::string right = showRight ? std::to_string(cpsRight) : "";
    std::string text = left + middleFix + right;

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

void CPSItem::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    //DrawItemSettings();

    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Checkbox(u8"左键", &showLeft);

    ImGui::SameLine();
    ImGui::SetCursorPosX(bigPadding + centerX);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Checkbox(u8"右键", &showRight);

    DrawAffixSettings(bigPadding, centerX, itemWidth);
    DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void CPSItem::Load(const nlohmann::json& j)
{
    LoadItem(j);
    LoadAffix(j);
    LoadWindow(j);
    if (j.contains("showLeft")) showLeft = j["showLeft"];
    if (j.contains("showRight")) showRight = j["showRight"];
}

void CPSItem::Save(nlohmann::json& j) const
{
    SaveItem(j);
    SaveAffix(j);
    SaveWindow(j);
    j["showLeft"] = showLeft;
    j["showRight"] = showRight;
}
