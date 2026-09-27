#include "TimeItem.h"
#include <iomanip>
#include <sstream>

#include "Anim.h"
#include "ImGuiStd.h"

void TimeItem::Toggle()
{
}

void TimeItem::Update()
{
    // 获取系统当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &t);
#else
    localtime_r(&t, &localTime);
#endif

    // 转换成字符串
    std::stringstream ss;
    if (showDate)
        ss << std::put_time(&localTime, "%Y-%m-%d | %H:%M:%S");
    else
        ss << std::put_time(&localTime, "%H:%M:%S");
    currentTimeStr = ss.str();
    dirtyState.contentDirty = true;
}

void TimeItem::HoverSetting()
{
}

void TimeItem::DrawContent()
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
    ImGuiStd::TextColoredShadow(color.color, (prefix + currentTimeStr + suffix).c_str());
}

void TimeItem::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    //DrawItemSettings();
    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Checkbox(u8"显示日期", &showDate);
    DrawAffixSettings(bigPadding, centerX, itemWidth);
    DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void TimeItem::Load(const nlohmann::json& j)
{
    LoadItem(j);
    LoadAffix(j);
    LoadWindow(j);
    if(j.contains("showDate")) showDate = j.at("showDate").get<bool>();
}

void TimeItem::Save(nlohmann::json& j) const
{
    SaveItem(j);
    SaveAffix(j);
    SaveWindow(j);
    j["showDate"] = showDate;
}