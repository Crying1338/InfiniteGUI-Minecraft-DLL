#pragma once
#include <Windows.h>

#include "imgui\imgui.h"
#include "Item.h"
#include "WindowModule.h"
#include "AffixModule.h"
#include "UpdateModule.h"
#include "KeybindModule.h"
#include "SoundModule.h"
#include <string>
#include <chrono>
#include "KeyState.h"
enum SprintState {
    Idle,
    Sprinting,
    Sneaking,
    Walking,
    OutOfWindow
};

struct sprint_element {
    ImVec4 color;
    ImVec4 targetTextColor;
};

class Sprint : public WindowModule, public UpdateModule, public KeybindModule, public AffixModule, public Item, public SoundModule
{
public:
    Sprint() {
        type = Util; // 信息项类型
        name = u8"强制疾跑";
        description = u8"强制疾跑";
        icon = "o";
        updateIntervalMs = 5;
        lastUpdateTime = std::chrono::steady_clock::now();
        Sprint::Reset();
    }

    static Sprint& Instance() {
        static Sprint instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        ResetWindow();
        ResetKeybind();
        ResetAffix();
        ResetSound();
        isEnabled = false; // 是否启用

        keybinds.insert(std::make_pair(u8"激活键：", 'I'));
        gameKeybinds.insert(std::make_pair(u8"前进键：", 'W'));
        gameKeybinds.insert(std::make_pair(u8"疾跑键：", VK_CONTROL));
        gameKeybinds.insert(std::make_pair(u8"潜行键：", VK_SHIFT));

        isActivated = false;
        isWalking = false;
        lastState = Idle;
        state = Idle;

        inputMode = SC_SEND_INPUT;

        color = { ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_Text)) };
        dirtyState.contentDirty = true;
        dirtyState.animating = true;
    }
    void OnKeyEvent(bool state, bool isRepeat, WPARAM key) override;

    void Update() override;
    void HoverSetting() override;
    void DrawContent() override;
    void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override;
    void Load(const nlohmann::json& j) override;
    void Save(nlohmann::json& j) const override;

private:

    void GetWalking();
    void GetSneaking();
    void SetSprinting() const;
    bool isActivated = false;
    bool isWalking = false;
    SprintState lastState = Idle;
    SprintState state = Idle;
    int inputMode = SC_SEND_INPUT;
    sprint_element color = { ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_Text)) };
};