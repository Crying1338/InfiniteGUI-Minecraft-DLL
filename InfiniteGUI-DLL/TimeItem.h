#pragma once
#include "Item.h"
#include "AffixModule.h"
#include "UpdateModule.h"
#include "WindowModule.h"
#include "nlohmann/json.hpp"

#include <string>
#include <chrono>
#include <ctime>

struct time_element {
    ImVec4 color;
};

class TimeItem : public Item, public AffixModule , public UpdateModule, public WindowModule {
public:
    TimeItem() {
        type = Hud; // 信息项类型
        name = u8"时间显示";
        description = u8"显示当前时间和日期";
        icon = "a";
        updateIntervalMs = 1000;
        lastUpdateTime = std::chrono::steady_clock::now();
        TimeItem::Reset();
    }
    //Instance()
    static TimeItem& Instance() {
        static TimeItem instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        ResetWindow();
        ResetAffix();
        currentTimeStr = u8"正在获取系统时间...";
        isEnabled = false;
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
    bool showDate = true;
    std::string currentTimeStr;
    std::string currentDateStr;
    time_element color;
};