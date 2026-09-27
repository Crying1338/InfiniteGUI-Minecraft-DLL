#pragma once
#include "Item.h"
#include "WindowModule.h"
#include "nlohmann/json.hpp"
#include "Text.h"

class TextItem : public Item, public RenderModule
{
public:
    TextItem() {
        type = Hud;
        name = u8"文本显示";
        description = u8"显示一段文本";
        icon = "(";
        TextItem::Reset();
    }

    static TextItem& Instance() {
        static TextItem instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        isEnabled = false;
        texts.clear();
        texts.emplace_back();
    }
    void RenderGui() override;
    void RenderBeforeGui() override;
    void RenderAfterGui() override;
    void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override;
    void Load(const nlohmann::json& j) override;
    void Save(nlohmann::json& j) const override;
private:
    std::vector<Text> texts;
};