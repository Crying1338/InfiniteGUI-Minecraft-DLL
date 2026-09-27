#pragma once
#include "Item.h"
#include "UpdateModule.h"
#include <Windows.h>

#include "RenderModule.h"
enum GameState {
    InGameMenu,
    InMenu,
    InGame,
};

enum WindowState {
    NormalWindow,
    FullScreen,
};

class GameStateDetector : public UpdateModule, public Item, public RenderModule{
public:


    GameStateDetector() {
        type = Hidden; // 信息项类型
        name = u8"游戏状态检测";
        description = u8"检测游戏当前状态";
        icon = u8"\uE039";
        updateIntervalMs = 10;
        lastUpdateTime = std::chrono::steady_clock::now();
        GameStateDetector::Reset();
    }

    static GameStateDetector& Instance()
    {
        static GameStateDetector instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        isEnabled = true;
        bool hideItemInGui = true;
        dirtyState.contentDirty = true;
    }
    void Update() override;
    void RenderGui() override
    {
    }
    void RenderBeforeGui() override
    {
    }
    void RenderAfterGui() override
    {
    }
    void Load(const nlohmann::json& j) override;
    void Save(nlohmann::json& j) const override;
    void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override;

    bool IsInGame() const;          // 在游戏世界中
    bool IsNeedHide() const;        // 是否需要隐藏信息项;
    GameState GetCurrentState() const;
    WindowState GetWindowState() const;
    bool IsInGameWindow() const;
    void ProcessMouseMovement(int dx, int dy);
    bool IsCameraMoving() const;
    float GetCameraSpeed() const;
    bool IsFullScreenClicked() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastClickTime).count() < fullscreenIntervalMs;
    }

private:
    static bool IsMouseCursorVisible();

    int fullscreenIntervalMs = 1000;
    std::chrono::steady_clock::time_point lastClickTime = std::chrono::steady_clock::now(); 

    bool hideItemInGui = true;

    GameState currentState = InGameMenu;
    GameState lastState = InGameMenu;
    WindowState windowState = NormalWindow;
    bool isInGameWindow = true;
    int centerLevel = 1;

    float movementThreshold = 1.0f; // 小于这个视为静止，用于防抖动
    float cameraSpeed;    // 鼠标移动速度
    bool cameraMoving;
};
