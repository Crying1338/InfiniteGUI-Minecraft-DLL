
#pragma once
#include <string>

#include "Anim.h"
#include "imgui/imgui.h"

struct keystrokes_element {
    ImVec4 backgroundColor;
    ImVec4 fontColor;
};

enum key_type {
    normal,
    space,
    mouse
};
struct Keybox
{
    key_type type;
    std::string label;
    bool state;
    bool lastState;
    float width;
    float height;
    bool needReturn;
    keystrokes_element color;
    int key;

    Keybox(key_type type, std::string label, float min_box_size, float padding, bool needReturn, int key)
    {
        this->type = type;
        this->label = label;
        this->state = false;
        this->lastState = false;
        SetSize(min_box_size, padding);
        this->needReturn = needReturn;
        this->color = { ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_FrameBg)), ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_Text)) };
        this->key = key;
    }
    void SetSize(float min_box_size, float padding)
    {
        float space_width = min_box_size * 3 + padding * 2;
        float mouse_width = (space_width - padding) / 2.0f;
        switch (type)
        {
        case normal:
            width = min_box_size;
            this->height = min_box_size;
            break;
        case space:
            width = space_width;
            this->height = min_box_size / 2;
            break;
        case mouse:
            width = mouse_width;
            this->height = min_box_size;
            break;
        default:
            width = min_box_size;
            this->height = min_box_size;
            break;
        }
    }
};

class ClassicKeyboxDraw
{
public:
    static void ClassicDraw(Keybox* keybox, ImGuiIO& io, bool& animating)
    {
        ImVec2 pos = ImGui::GetCursorPos();
        ImVec4 targetTextColor = keybox->state ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImGui::GetStyleColorVec4(ImGuiCol_Text);
        ImVec4 targetBgColor = keybox->state ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);

        //计算速度
        float speed_off = 15.0f * std::clamp(io.DeltaTime, 0.0f, 0.05f);
        float speed_on = speed_off * 2.5f;
        keybox->color.fontColor = ImLerp(keybox->color.fontColor, targetTextColor, keybox->state ? speed_on : speed_off);
        keybox->color.backgroundColor = ImLerp(keybox->color.backgroundColor, targetBgColor, keybox->state ? speed_on : speed_off);
        if (Anim::AlmostEqual(keybox->color.fontColor, targetTextColor) && Anim::AlmostEqual(keybox->color.backgroundColor, targetBgColor))
        {
            keybox->color.fontColor = targetTextColor;
            keybox->color.backgroundColor = targetBgColor;
        }
        else
            animating = true;

        DrawBackground(keybox);
        DrawBorder(keybox);
        if (keybox->label == "-----")
            DrawSpaceLine(keybox);
        else
        {
            DrawLabel(keybox);
        }
        ImGui::SetCursorPos(ImVec2(pos.x + keybox->width, pos.y + keybox->height));
        ImGui::Dummy(ImVec2(0, 0));
    }
private:
    static void DrawLabel(Keybox* keybox)
    {
        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 text_size = ImGui::CalcTextSize(keybox->label.c_str());
        ImVec2 text_pos = ImVec2(
            pos.x + (keybox->width - text_size.x) * 0.5f,
            pos.y + (keybox->height - text_size.y) * 0.5f
        );
        ImGui::SetCursorPos(text_pos);
        ImGuiStd::TextColoredShadow(keybox->color.fontColor, keybox->label.c_str());
    }

    static void DrawBackground(Keybox* keybox, const ImDrawFlags& flags = ImDrawFlags_RoundCornersAll)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2(keybox->width, keybox->height);

        draw->AddRectFilled(
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y),
            ImGui::ColorConvertFloat4ToU32(keybox->color.backgroundColor),
            ImGui::GetStyle().FrameRounding, // 圆角
            flags
        );
    }

    static void DrawBorder(Keybox* keybox, const ImDrawFlags& flags = ImDrawFlags_RoundCornersAll, const float borderSize = 1.0f)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2(keybox->width, keybox->height);

        draw->AddRect(
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y),
            ImGui::GetColorU32(ImGuiCol_Border),
            ImGui::GetStyle().FrameRounding, // 圆角
            flags,
            borderSize // 线宽
        );
    }

    static void DrawSpaceLine(Keybox* keybox)
    {
        ImVec2 pos = ImGui::GetCursorPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 spaceFontSize = ImGui::CalcTextSize(keybox->label.c_str());
        ImVec2 text_pos = ImVec2(
            pos.x + (keybox->width - spaceFontSize.x) * 0.5f,
            pos.y + keybox->height * 0.5f
        );
        ImGui::SetCursorPos(text_pos);
        ImVec2 linePos1 = ImGui::GetCursorScreenPos();
        ImVec2 linePos2 = ImVec2(linePos1.x + spaceFontSize.x, linePos1.y);
        //shadow
        ImVec2 linePos3 = ImVec2(linePos1.x + 1.0f, linePos1.y + 1.0f);
        ImVec2 linePos4 = ImVec2(linePos2.x + 1.0f, linePos2.y + 1.0f);
        //shadow
        ImU32 shadowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.6f));
        draw_list->AddLine(linePos3, linePos4, shadowColor, spaceFontSize.y * 0.1f);
        draw_list->AddLine(linePos1, linePos2, ImGui::ColorConvertFloat4ToU32(keybox->color.fontColor), spaceFontSize.y * 0.1f);
        ImGui::Dummy(ImVec2(0, 0));
    }

};