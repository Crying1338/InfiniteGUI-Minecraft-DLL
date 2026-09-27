#pragma once
#include "WindowStyleModule.h"
#include "GlobalWindowStyle.h"
#include "RenderModule.h"
#include <string>
#include "imgui/imgui.h"
#include "ImGuiStd.h"
#include <nlohmann/json.hpp>

#include "GameStateDetector.h"
#include "MyButton.hpp"
#include "WindowSnapper.h"
#include "opengl_hook.h"
static constexpr float SNAP_DISTANCE = 10.0f;
static bool isSnapping = false;

struct Customize
{
    bool windowRounding = false;
    bool fontSize = false;
    bool fontColor = false;
    bool bgColor = false;
    bool borderColor = false;
};

struct ItemStylePtr
{
    float* windowRounding;
    float* fontSize;
    ImVec4* fontColor;
    ImVec4* bgColor;
    ImVec4* borderColor;
    bool* rainbowFont;
};


class WindowModule : public WindowStyleModule, public RenderModule
{
public:

    WindowModule()
    {
        SetStyle();
    }
    virtual void DrawContent() = 0;       // 绘制内容（文本、图形等）

protected:
    virtual void HoverSetting() = 0;
    void SetStyle()
    {
        if (custom.windowRounding)
            itemStylePtr.windowRounding = &itemStyle.windowRounding;
        else
            itemStylePtr.windowRounding = &GlobalWindowStyle::Instance().GetGlobeStyle().windowRounding;
        if (custom.fontSize)
            itemStylePtr.fontSize = &itemStyle.fontSize;
        else
            itemStylePtr.fontSize = &GlobalWindowStyle::Instance().GetGlobeStyle().fontSize;
        if (custom.fontColor)
        {
            itemStylePtr.fontColor = &itemStyle.fontColor;
            itemStylePtr.rainbowFont = &itemStyle.rainbowFont;
        }
        else
        {
            itemStylePtr.fontColor = &GlobalWindowStyle::Instance().GetGlobeStyle().fontColor;
            itemStylePtr.rainbowFont = &GlobalWindowStyle::Instance().GetGlobeStyle().rainbowFont;
        }
        if (custom.bgColor)
            itemStylePtr.bgColor = &itemStyle.bgColor;
        else
            itemStylePtr.bgColor = &GlobalWindowStyle::Instance().GetGlobeStyle().bgColor;
        if (custom.borderColor)
            itemStylePtr.borderColor = &itemStyle.borderColor;
        else
            itemStylePtr.borderColor = &GlobalWindowStyle::Instance().GetGlobeStyle().borderColor;
    }

    static ImVec4* EditWindowColor(const char* label, ImVec4* color,ImVec4* globalColorPtr, bool& custom, ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel)
    {
        if (ImGuiStd::EditColor(label, *color))
        {
            custom = true;
        }
        if(custom)
        {
            ImGui::SameLine();
            ImGui::PushFont(opengl_hook::gui.iconFont);
            if (ImGui::Button((u8"\uE02E" + std::string("##") + label).c_str()))
            {
                custom = false;
                *color = *globalColorPtr;
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(u8"使用全局样式");
            }
            return color;
        }
            return globalColorPtr;
    }
public:

    void DrawWindowSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
    {
        ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
        ImGui::BeginDisabled();
        ImGuiStd::TextShadow(u8"窗口设置");
        ImGui::EndDisabled();
        ImGui::PopFont();

        float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(itemWidth);

        ImGui::Checkbox(u8"固定", &fixed);
        ImGui::SameLine();
        ImGui::SetCursorPosX(bigPadding + centerX);
        ImGui::SetNextItemWidth(itemWidth);

        ImGui::Checkbox(u8"自定义窗口大小", &isCustomSize);
        if (isCustomSize) {

            ImGui::SetCursorPosX(bigPadding);
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::InputFloat(u8"宽度", &width, 1.0f, 1.0f, "%.1f");
            ImGui::SameLine();
            ImGui::SetCursorPosX(bigPadding + centerX);
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::InputFloat(u8"高度", &height, 1.0f, 1.0f, "%.1f");
        }

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(itemWidth);
        ImGui::InputFloat(u8"窗口 X", &x, 1.0f, 1.0f, "%.1f");
        ImGui::SameLine();
        ImGui::SetCursorPosX(bigPadding + centerX);
        ImGui::SetNextItemWidth(itemWidth);
        ImGui::InputFloat(u8"窗口 Y", &y, 1.0f, 1.0f, "%.1f");

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(bigItemWidth);
        if (ImGui::SliderFloat(u8"窗口圆角", &itemStyle.windowRounding, 0.0f, 10.0f, "%.1f"))
        {
            custom.windowRounding = true;
            //itemStylePtr.windowRounding = &itemStyle.windowRounding;
        }
        if (custom.windowRounding)
        {
            ImGui::SameLine();
            ImGui::PushFont(opengl_hook::gui.iconFont);
            if (ImGui::Button((u8"\uE02E" + std::string("##windowRounding")).c_str()))
            {
                custom.windowRounding = false;
                //itemStyle.windowRounding = GlobalWindowStyle::Instance().GetGlobeStyle().windowRounding;
                //itemStylePtr.windowRounding = &GlobalWindowStyle::Instance().GetGlobeStyle().windowRounding;
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(u8"使用全局样式");
            }
        }

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(bigItemWidth);
        if(ImGui::InputFloat(u8"字体大小", &itemStyle.fontSize, 1.0f, 1.0f, "%.1f"))
        {
            custom.fontSize = true;
            //itemStylePtr.fontSize = &itemStyle.fontSize;
        } 
        if (custom.fontSize)
        {
            ImGui::SameLine();
            ImGui::PushFont(opengl_hook::gui.iconFont);
            if (ImGui::Button((u8"\uE02E" + std::string("##fontSize")).c_str()))
            {
                custom.fontSize = false;
                //itemStyle.fontSize = GlobalWindowStyle::Instance().GetGlobeStyle().fontSize;
                //itemStylePtr.fontSize = &GlobalWindowStyle::Instance().GetGlobeStyle().fontSize;
            }
            ImGui::PopFont();
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(u8"使用全局样式");
            }
        }

        ImVec4* colors = ImGui::GetStyle().Colors;

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(itemWidth);

        itemStylePtr.fontColor = EditWindowColor(u8"字体颜色", &itemStyle.fontColor, &GlobalWindowStyle::Instance().GetGlobeStyle().fontColor, custom.fontColor);
        ImGui::SameLine();
        if (ImGui::Checkbox(u8"彩虹", &itemStyle.rainbowFont))
        {
            custom.fontColor = true;
        }
        //if (custom.fontColor)
        //{
        //    itemStylePtr.fontColor = &itemStyle.fontColor;
        //    itemStylePtr.rainbowFont = &itemStyle.rainbowFont;
        //}
        //else
        //{
        //    itemStylePtr.rainbowFont = &GlobalWindowStyle::Instance().GetGlobeStyle().rainbowFont;
        //    itemStylePtr.fontColor = &GlobalWindowStyle::Instance().GetGlobeStyle().fontColor;
        //}
        ImGui::SameLine();
        ImGui::SetCursorPosX(centerX + bigPadding);
        ImGui::SetNextItemWidth(itemWidth);
        itemStylePtr.bgColor = EditWindowColor(u8"背景颜色", &itemStyle.bgColor, &GlobalWindowStyle::Instance().GetGlobeStyle().bgColor, custom.bgColor);
       
        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(itemWidth);
        itemStylePtr.borderColor = EditWindowColor(u8"边框颜色", &itemStyle.borderColor, &GlobalWindowStyle::Instance().GetGlobeStyle().borderColor, custom.borderColor);
    }

    // ---------------------------
//   渲染整个窗口（统一逻辑）
// ---------------------------
    virtual void RenderGui() override
    {
        if (!isWindowShow) return;
        if (!isCustomSize)
            //ImGui::SetNextWindowSizeConstraints(ImVec2(10, 10), ImVec2(2560, 1440));
            ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);

        if (!isMoving)
        {
            ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
            if (isCustomSize)
                ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
        }

        SetStyle();  //明明可以不加这个的，但是不加会崩，我无语...
        if (isTransparentBg)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // 背景透明
            ImGui::PushStyleColor(ImGuiCol_ChildBg, *itemStylePtr.bgColor);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // 边框透明
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, *itemStylePtr.bgColor);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, *itemStylePtr.bgColor);
            ImGui::PushStyleColor(ImGuiCol_Border, *itemStylePtr.borderColor);
        }
        PushRounding(*itemStylePtr.windowRounding);
        if (*itemStylePtr.rainbowFont)
            processRainbowFont();
        else
            ImGui::PushStyleColor(ImGuiCol_Text, *itemStylePtr.fontColor); // 字体颜色
        ImGui::PushFont(NULL, *itemStylePtr.fontSize);

        HWND g_hwnd = opengl_hook::handle_window;
        ImGuiWindowFlags flags = 0;
        //if (!allowResize) flags |= ImGuiWindowFlags_NoResize;
        if (fixed)   flags |= ImGuiWindowFlags_NoMove;
        if (fixed || !isCustomSize || GameStateDetector::Instance().IsInGame()) flags |= ImGuiWindowFlags_NoResize;
        flags |= ImGuiWindowFlags_NoTitleBar;
        flags |= ImGuiWindowFlags_NoScrollbar;
        flags |= ImGuiWindowFlags_NoSavedSettings;
        flags |= ImGuiWindowFlags_NoScrollWithMouse;

        //if (fixed) flags |= ImGuiWindowFlags_NoInputs;

        ImGui::Begin(GetActualWindowName().c_str(), nullptr, flags);
        isHovered = (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) ||
            (ImGui::IsAnyItemActive() && ImGui::IsWindowFocused())) && !GameStateDetector::Instance().IsInGame();
        isMoving = ImGui::IsMouseDragging(0, 1.0f) && isHovered;
        if (isHovered)
        {
            //获取io
            ImGuiIO& io = ImGui::GetIO();
            //计算速度
            float speed = 10.0f * std::clamp(io.DeltaTime, 0.0f, 0.05f);
            alpha = ImLerp(alpha, 0.35f, speed);
            // 判断动画是否结束
            if (Anim::AlmostEqual(alpha, 0.35f))
            {
                alpha = 0.35f;
            }
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);  //添加半透明
        }
        else alpha = 1.0f;

        DrawContent();

        if (isHovered)
        {
            ImGui::PopStyleVar(); //去除半透明
            HoverSetting();
            Sidebar();
        }
        ImGui::PopFont();
        // 判断拖动/修改大小事件
        if (isMoving)
        {
            HandleDrag(g_hwnd);
        }
        else
        {
            // 当前窗口位置大小
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 sz = ImGui::GetWindowSize();

            WindowSnapper::KeepSnapped(pos, sz, (float)opengl_hook::screen_size.x, (float)opengl_hook::screen_size.y, snapState);
            // 设置吸附后的位置
            ImGui::SetWindowPos(pos, ImGuiCond_Always);

            // 保存到 Item
            x = pos.x;
            y = pos.y;
        }

        ImGui::End();
        ImGui::PopStyleVar(7);
        ImGui::PopStyleColor(4);
    }

    virtual void RenderBeforeGui() override
    {

    }

    virtual void RenderAfterGui() override
    {

    }

    bool IsFixed() const {
        return fixed;
    }

    void SetFixed(bool value) {
        fixed = value;
    }
    float x = 100.0f;
    float y = 40.0f;
    float width = 250.0f;
    float height = 80.0f;
    bool isMoving = false;
private:

    void Sidebar()
    {
        ImGui::PushFont(opengl_hook::gui.iconFont);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        if (fixed)
        {
            ImVec2 lockPos = ImVec2(ImGui::GetWindowWidth() - ImGui::GetFontSize() - ImGui::GetStyle().WindowPadding.x, ImGui::GetStyle().WindowPadding.y);
            ImGui::SetCursorPos(lockPos);
            ImGuiStd::TextShadow(u8"\uE013");
            if (ImGui::IsItemClicked()) fixed = false;
        }
        else
        {
            ImVec2 lockPos = ImVec2(ImGui::GetWindowWidth() - ImGui::GetFontSize() * 2 - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().ItemSpacing.x, ImGui::GetStyle().WindowPadding.y);
            ImGui::SetCursorPos(lockPos);
            ImGuiStd::TextShadow(u8"\uE014");
            if (ImGui::IsItemClicked()) fixed = true;
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.52f, 0.52f, 1.0f)); // 字体颜色
            ImGuiStd::TextShadow("9");
            ImGui::PopStyleColor();
            if (ImGui::IsItemClicked()) closed = true;
        }
        ImGui::PopStyleVar();
        ImGui::PopFont();
    }

    std::string GetActualWindowName() const {
        return "##" + std::to_string((uintptr_t)this);
    }

    void HandleDrag(HWND g_hwnd)
    {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000 || GetAsyncKeyState(VK_LSHIFT) & 0x8000)
            isSnapping = true;
        else
            isSnapping = false;

        // 当前窗口位置大小
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 sz = ImGui::GetWindowSize();

        SnapResult snap;
        if (isSnapping)
        {
            // 计算吸附
            snap = WindowSnapper::ComputeSnap(pos, sz, (float)opengl_hook::screen_size.x, (float)opengl_hook::screen_size.y, SNAP_DISTANCE);
            // 画吸附线
            WindowSnapper::DrawGuides(snap, (float)opengl_hook::screen_size.x, (float)opengl_hook::screen_size.y, sz);
            //WindowSnapper::ComputeSnapWithWindows(sz, SNAP_DISTANCE, ItemManager::Instance().GetItems(), snap);
        }
        else
            snap = WindowSnapper::ComputeSnap(pos, sz, (float)opengl_hook::screen_size.x, (float)opengl_hook::screen_size.y, 0.0f);

        // 设置吸附后的位置
        ImGui::SetWindowPos(snap.snappedPos, ImGuiCond_Always);

        // 保存到 Item
        x = snap.snappedPos.x;
        y = snap.snappedPos.y;
        snapState = snap.snapState;

        //保存窗口大小
        width = ImGui::GetWindowSize().x;
        height = ImGui::GetWindowSize().y;
    }

protected:
    void LoadWindow(const nlohmann::json& j)
    {
        if (j.contains("isCustomSize")) isCustomSize = j["isCustomSize"];
        if (j.contains("x")) x = j["x"];
        if (j.contains("y")) y = j["y"];
        if (j.contains("width")) width = j["width"];
        if (j.contains("height")) height = j["height"];

        if (j.contains("clickThrough")) fixed = j["clickThrough"];

        if (j.contains("snapState")) snapState = j["snapState"];

        if (j.contains("custom"))
        {
            custom.windowRounding = j["custom"]["windowRounding"];
            custom.fontSize = j["custom"]["fontSize"];
            custom.fontColor = j["custom"]["fontColor"];
            custom.bgColor = j["custom"]["bgColor"];
            custom.borderColor = j["custom"]["borderColor"];
        }
        LoadStyle(j);
        SetStyle(); 
    }
    void SaveWindow(nlohmann::json& j) const
    {
        j["isCustomSize"] = isCustomSize;
        j["x"] = x;
        j["y"] = y;
        j["width"] = width;
        j["height"] = height;

        j["clickThrough"] = fixed;

        j["snapState"] = snapState;

        j["custom"] = {
            {"windowRounding", custom.windowRounding},
            {"fontSize", custom.fontSize},
            {"fontColor", custom.fontColor},
            {"bgColor", custom.bgColor},
            {"borderColor", custom.borderColor}
        };
        SaveStyle(j);
    }

    void ResetWindow()
    {
        isCustomSize = false;
        x = 100.0f;
        y = 40.0f;
        snapState = SNAP_NONE;
        width = 250.0f;
        height = 80.0f;
        fixed = false;
        custom.windowRounding = false;
        custom.fontSize = false;
        custom.fontColor = false;
        custom.bgColor = false;
        custom.borderColor = false;
        SetStyle();
        ResetWindowStyle();
    }
    bool isWindowShow = true;

    bool isCustomSize = false;    //是否自定义大小

    SnapState snapState = SNAP_NONE;

    bool allowResize = true;
    bool fixed = false;

    bool isHovered = true;
    float alpha = 1.0f;

    bool isTransparentBg = false;

    Customize custom;
    ItemStylePtr itemStylePtr;
    
    bool closed = false;

};