#pragma once
#include "imgui\imgui.h"
#include "Item.h"
#include "WindowModule.h"
#include "AffixModule.h"
#include "UpdateModule.h"

struct cps_element {
    ImVec4 color;

};
class CPSItem : public Item, public WindowModule, public AffixModule, public UpdateModule
{
public:
    CPSItem() {
        type = Hud; // 信息项类型
        name = u8"CPS显示";
        description = u8"显示左右键CPS";
        icon = "!";
        CPSItem::Reset();
    }

    static CPSItem& Instance() {
        static CPSItem instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        ResetAffix();
        ResetWindow();
        isEnabled = false;
        prefix = "[CPS: ";
        suffix = "]";
        showLeft = true;
        showRight = true;
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

    bool showLeft = true;
    bool showRight = true;

    int cpsLeft = 0;
    int cpsRight = 0;

    cps_element color;

};