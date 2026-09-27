#pragma once
#include "Item.h"
#include "UpdateModule.h"
#include <windows.h>

#include "GameKeyBind.h"
#include "KeybindModule.h"

struct WindowStyleState {
    LONG style;
    RECT rect;
    bool isBorderlessFullscreen = false;
};


class GameWindowTool : public Item, public UpdateModule, public KeybindModule
{
public:
    GameWindowTool() {
        type = Hidden; // 信息项类型
        name = u8"窗口工具";
        description = u8"";
        icon = "";
        updateIntervalMs = 10;
        lastUpdateTime = std::chrono::steady_clock::now();
        GameWindowTool::Reset();
    }

    ~GameWindowTool() override;

    static GameWindowTool& Instance() {
        static GameWindowTool instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        ResetKeybind();
        isEnabled = true;
        keybinds.insert(std::make_pair(u8"无边框全屏快捷键：", NULL));
        gameKeybinds.insert(std::make_pair(u8"游戏全屏快捷键：", GameKeyBind::Instance().IsSuccess() ? GameKeyBind::Instance().GetVK(GameAction::Fullscreen) : NULL));
    }
    int GetFullscreenVK() const {
        return GameKeyBind::Instance().IsSuccess() ? GameKeyBind::Instance().GetVK(GameAction::Fullscreen) : GetGameKeybindValue(u8"游戏全屏快捷键：", 0);
    }
    void OnKeyEvent(bool state, bool isRepeat, WPARAM key) override;
    void Update() override;
    void Load(const nlohmann::json& j) override;
    void Save(nlohmann::json& j) const override;
    void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override;

private:
    void SetBorderlessFullscreen(HWND hwnd);
    WindowStyleState windowStyleState;
};
