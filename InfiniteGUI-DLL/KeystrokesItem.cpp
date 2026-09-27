#include "KeystrokesItem.h"
#include "imgui\imgui.h"
#include "imgui\imgui_internal.h"
#include "ImGuiStd.h"
#include <string>
#include <windows.h>

#include "Anim.h"
#include "GameStateDetector.h"

void KeystrokesItem::Toggle()
{
}
void KeystrokesItem::Update()
{
    if(!GameStateDetector::Instance().IsInGameWindow())
        return;
    for (auto& key_box : key_boxes)
    {
        if(GetAsyncKeyState(key_box.key) & 0x8000)
            key_box.state = true;
        else
            key_box.state = false;
        if (key_box.lastState != key_box.state)
        {
            dirtyState.animating = true;
            key_box.lastState = key_box.state;
        }
    }
}

void KeystrokesItem::HoverSetting()
{
}

void KeystrokesItem::DrawContent()
{
    if (closed)
    {
        isEnabled = false;
        closed = false;
    }
    //获取io
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
    ImVec2 cursorPos;
    cursorPos.x = min_box_size + padding + window_padding.x;
    cursorPos.y = window_padding.y;
    ImGui::PushStyleColor(ImGuiCol_Border, *itemStylePtr.borderColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, *itemStylePtr.windowRounding);
    //ImGui::PushStyleColor(ImGuiCol_ChildBg, *itemStylePtr.bgColor);
    bool animating = false;
    for (auto& key_box : key_boxes)
    {
        if(key_box.type == space && !showSpace) continue;
        if(key_box.type == mouse && !showMouse) continue;
        ImGui::SetCursorPos(cursorPos);
        
        ClassicKeyboxDraw::ClassicDraw(&key_box, io, animating);

        if (key_box.needReturn)
        {
            cursorPos.y += key_box.height + padding;
            cursorPos.x = window_padding.x;
        }
        else
            cursorPos.x += key_box.width + padding;
    }
    if (!animating) dirtyState.animating = false;
    ImGui::PopStyleVar(); //窗口圆角
    ImGui::PopStyleColor(); //是否显示边框
}

void KeystrokesItem::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Checkbox(u8"显示空格", &showSpace);
    ImGui::SameLine();
    ImGui::SetCursorPosX(bigPadding + centerX);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Checkbox(u8"显示鼠标", &showMouse);
    //ImGui::Checkbox(u8"显示CPS", &showCps);
    //ImGui::Separator();
    bool needResize = false;
    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(bigItemWidth);
    if(ImGui::SliderFloat(u8"按键边长", &min_box_size, 12.0f, 108.0f, "%.1f"))
    {
        needResize = true;
    }
    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(bigItemWidth);
    if(ImGui::SliderFloat(u8"按键间隔", &padding, 1.0f, 18.0f, "%.1f"))
    {
        needResize = true;
    }
    if(needResize)
    {
        for (auto& key_box : key_boxes)
        {
            key_box.SetSize(min_box_size, padding);
        }
    }
    DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void KeystrokesItem::Load(const nlohmann::json& j)
{
    LoadItem(j);
    if (j.contains("showSpace")) showSpace = j["showSpace"];
    if (j.contains("showMouse")) showMouse = j["showMouse"];
    if (j.contains("showCps")) showCps = j["showCps"];
    if (j.contains("min_box_size")) min_box_size = j["min_box_size"];
    if (j.contains("padding")) padding = j["padding"];
    for (auto& key_box : key_boxes)
    {
        key_box.SetSize(min_box_size, padding);
    }
    LoadWindow(j);
}

void KeystrokesItem::Save(nlohmann::json& j) const
{
    SaveItem(j);
    SaveWindow(j);
    j["showSpace"] = showSpace;
    j["showMouse"] = showMouse;
    j["showCps"] = showCps;
    j["min_box_size"] = min_box_size;
    j["padding"] = padding;
}
