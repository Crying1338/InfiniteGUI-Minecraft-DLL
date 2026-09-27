#pragma once

#include <Windows.h>
#include "imgui\imgui.h"
#include "Item.h"
#include "WindowModule.h"
#include "UpdateModule.h"

#include <chrono>

#include "Keybox.h"

class KeystrokesItem : public Item, public WindowModule, public UpdateModule
{
public:
    KeystrokesItem() {
        type = Hud; // 信息项类型
        name = u8"按键显示";
        description = u8"显示按键状态";
        icon = u8"\uE05D";
        updateIntervalMs = 5;
        lastUpdateTime = std::chrono::steady_clock::now();
        KeystrokesItem::Reset();
    }

    static KeystrokesItem& Instance() {
        static KeystrokesItem instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        ResetWindow();
        isTransparentBg = true;
        isCustomSize = false;
        isEnabled = false;
        key_boxes.clear();
        key_boxes.emplace_back(Keybox(normal, "W", min_box_size, padding, true, 'W'));
        key_boxes.emplace_back(Keybox(normal, "A", min_box_size, padding, false, 'A'));
        key_boxes.emplace_back(Keybox(normal, "S", min_box_size, padding, false, 'S'));
        key_boxes.emplace_back(Keybox(normal, "D", min_box_size, padding, true, 'D'));
        key_boxes.emplace_back(Keybox(space, "-----", min_box_size, padding, true, VK_SPACE));
        key_boxes.emplace_back(Keybox(mouse, "LMB", min_box_size, padding, false, VK_LBUTTON));
        key_boxes.emplace_back(Keybox(mouse, "RMB", min_box_size, padding, true, VK_RBUTTON));
        showSpace = true;
        showMouse = true;
        showCps = false;
        padding = 6.0f;
        min_box_size = 32.0f;
        dirtyState.contentDirty = true;
        dirtyState.animating = true;
    }
    void Update() override;
    void HoverSetting() override;
    void DrawContent() override;
    void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override;
    void Load(const nlohmann::json& j) override;
    void Save(nlohmann::json& j) const override;

private:
    bool showSpace = true;
    bool showMouse = true;
    bool showCps = false;
    std::vector<Keybox> key_boxes;
    float padding = 6.0f;
    float min_box_size = 32.0f;
};
