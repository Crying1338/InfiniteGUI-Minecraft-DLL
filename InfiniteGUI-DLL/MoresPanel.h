#pragma once
#include "menuRule.h"
#include "ImGuiStd.h"
#include "imgui/imgui.h"
#
class MoresPanel
{
public:
    static void Draw()
    {
        bool exit = false;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 8.0f));
        ImGui::BeginChild("About", ImVec2(-padding + ImGui::GetStyle().WindowPadding.x, -padding + ImGui::GetStyle().WindowPadding.y), true, flags);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));

        // ===== 布局参数 =====
        ImGuiStyle& style = ImGui::GetStyle();
        float basePadding = style.WindowPadding.x;
        float bigPadding = basePadding * 3.0f;

        // 获取窗口可用宽度
        float contentWidth = ImGui::GetContentRegionAvail().x;
        float centerX = contentWidth * 0.5f;
        // 每个输入框宽度（留点余量，避免顶到边）
        float itemWidth = centerX - bigPadding * 4.0f;
        float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

        ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
        ImGui::BeginDisabled();
        ImGuiStd::TextShadow(u8"更多内容敬请期待...");
        ImGui::EndDisabled();
        ImGui::PopFont();

        ImGui::PopStyleVar();
        ImGui::EndChild();

    }
private:

};
