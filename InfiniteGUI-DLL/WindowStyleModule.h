#pragma once

#include "imgui\imgui.h"
#include "ImGuiStd.h"
#include <nlohmann/json.hpp>
struct ItemStyle {
    float windowRounding = 3.0f;
    float fontSize = 20.0f;
    ImVec4 fontColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 bgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    ImVec4 borderColor = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    bool rainbowFont = false;
};

class WindowStyleModule {
public:
    WindowStyleModule()
    {
        GetDefaultStyle();
        InitStyle();
    }
    void DrawStyleSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
    {
        //ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
        //ImGui::BeginDisabled();
        //ImGuiStd::TextShadow(u8"窗口样式设置"); 
        //ImGui::EndDisabled();
        //ImGui::PopFont();

        float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(bigItemWidth);

        ImGui::SliderFloat(u8"窗口圆角", &itemStyle.windowRounding, 0.0f, 10.0f, "%.1f");
        //字体大小设置
        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(bigItemWidth);
        ImGui::InputFloat(u8"字体大小", &itemStyle.fontSize, 1.0f, 1.0f, "%.1f");
        //颜色设置
        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(itemWidth);
        ImGuiStd::EditColor(u8"字体颜色", itemStyle.fontColor, defaultStyle.fontColor);
        ImGui::SameLine();
        ImGui::Checkbox(u8"彩虹", &itemStyle.rainbowFont);

        ImGui::SameLine();
        ImGui::SetCursorPosX(centerX + bigPadding);
        ImGui::SetNextItemWidth(itemWidth);
        ImGuiStd::EditColor(u8"背景颜色", itemStyle.bgColor, defaultStyle.bgColor);

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(itemWidth);
        ImGuiStd::EditColor(u8"边框颜色", itemStyle.borderColor, defaultStyle.borderColor);
    }
protected:
    void LoadStyle(const nlohmann::json& j)
    {
        if (j.contains("windowRounding")) itemStyle.windowRounding = j["windowRounding"];
        if (j.contains("fontSize")) itemStyle.fontSize = j["fontSize"];
        if (j.contains("rainbowFont")) itemStyle.rainbowFont = j["rainbowFont"];
        ImGuiStd::LoadImVec4(j, "fontColor", itemStyle.fontColor);
        ImGuiStd::LoadImVec4(j, "bgColor", itemStyle.bgColor);
        ImGuiStd::LoadImVec4(j, "borderColor", itemStyle.borderColor);

    }

    void SaveStyle(nlohmann::json& j) const
    {
        j["windowRounding"] = itemStyle.windowRounding;
        j["fontSize"] = itemStyle.fontSize;
        j["rainbowFont"] = itemStyle.rainbowFont;
        ImGuiStd::SaveImVec4(j, "fontColor", itemStyle.fontColor);
        ImGuiStd::SaveImVec4(j, "bgColor", itemStyle.bgColor);
        ImGuiStd::SaveImVec4(j, "borderColor", itemStyle.borderColor);
    }
    void InitStyle()
    {
        //初始化默认样式
        ImGuiStyle& defaultStyle = ImGui::GetStyle();
        itemStyle.windowRounding = defaultStyle.WindowRounding;
        itemStyle.fontSize = defaultStyle.FontSizeBase;
        itemStyle.fontColor = defaultStyle.Colors[ImGuiCol_Text];
        itemStyle.bgColor = defaultStyle.Colors[ImGuiCol_WindowBg];
        itemStyle.borderColor = defaultStyle.Colors[ImGuiCol_Border];
    }

    static void GetDefaultStyle()
    {
        static bool isInit = false;
        if (isInit) return;
        ImGuiStyle& style = ImGui::GetStyle();
        defaultStyle.windowRounding = style.WindowRounding;
        defaultStyle.fontSize = style.FontSizeBase;
        defaultStyle.fontColor = style.Colors[ImGuiCol_Text];
        defaultStyle.bgColor = style.Colors[ImGuiCol_WindowBg];
        defaultStyle.borderColor = style.Colors[ImGuiCol_Border];
        isInit = true;
    }

    static void processRainbowFont()
    {
        ImVec4 col = ImColor::HSV((float)fmod(ImGui::GetTime() * 0.2f, 1.0f), 1, 1);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
    }
    static void PushRounding(float rounding)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, rounding);
    }

    void ResetWindowStyle()
    {
        itemStyle = defaultStyle;
        itemStyle.fontSize = 24.0f;
    }
    ItemStyle itemStyle;
    inline static ItemStyle defaultStyle;
};
