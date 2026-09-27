#include "CounterItem.h"
#include "AudioManager.h"
#include "imgui\imgui.h"
#include "ImGuiStd.h"
#include "imgui\imgui_internal.h"
#include <string>
#include <windows.h>

#include "Anim.h"
#include "GameStateDetector.h"
#include "NotificationItem.h"

void CounterItem::Toggle()
{
}

void CounterItem::OnKeyEvent(bool state, bool isRepeat, WPARAM key)
{
    if(key == NULL || !GameStateDetector::Instance().IsInGame()) return;
    if(state) //按键按下
    {
        if (key == keybinds.at(u8"增加快捷键："))
        {
            count++;
            NotificationItem::Instance().AddNotification(NotificationType_Info, u8"计数器：+1。");
        }
        else if (key == keybinds.at(u8"减少快捷键："))
        {
            count--;
            NotificationItem::Instance().AddNotification(NotificationType_Info, u8"计数器：-1。");
        }
        else if (key == keybinds.at(u8"清空快捷键："))
        {
            count = 0;
            NotificationItem::Instance().AddNotification(NotificationType_Info, u8"计数器：清零。");
        }
        if (count > lastCount)
        {
            color.color = ImVec4(0.1f, 1.0f, 0.1f, 1.0f); //绿色
            lastCount = count;
            if (isPlaySound) AudioManager::Instance().playSound("counter\\counter_up.wav", soundVolume);
        }
        else if (count < lastCount)
        {
            color.color = ImVec4(1.0f, 0.1f, 0.1f, 1.0f); //红色
            lastCount = count;
            if (isPlaySound) AudioManager::Instance().playSound("counter\\counter_down.wav", soundVolume);
        }
        else return;

        dirtyState.contentDirty = true;
        dirtyState.animating = true;
    }
}

void CounterItem::HoverSetting()
{
}

void CounterItem::DrawContent()
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

    ImGuiStd::TextColoredShadow(color.color, (prefix + std::to_string(count) + suffix).c_str());

}

void CounterItem::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    //DrawItemSettings();

    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::InputInt(u8"计数值", &count);

    DrawKeybindSettings(bigPadding, centerX, itemWidth);
    DrawAffixSettings(bigPadding, centerX, itemWidth);
    DrawSoundSettings(bigPadding, centerX, itemWidth);
    DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void CounterItem::Load(const nlohmann::json& j)
{
    LoadItem(j);
    LoadAffix(j);
    LoadWindow(j);
    LoadSound(j);
    LoadKeybind(j);
    if (j.contains("count")) count = j["count"];
    lastCount = count;

}

void CounterItem::Save(nlohmann::json& j) const
{
    SaveItem(j);
    SaveAffix(j);
    SaveWindow(j);
    SaveSound(j);
    SaveKeybind(j);
    j["count"] = count;
}