#include "TextItem.h"

#include "GameStateDetector.h"

void TextItem::Toggle()
{
}

void TextItem::RenderGui()
{
    if (GameStateDetector::Instance().IsNeedHide()) return;
    for(auto& text : texts)
        if(text.GetEnabled()) text.RenderGui();
}

void TextItem::RenderBeforeGui()
{
}

void TextItem::RenderAfterGui()
{
}

void TextItem::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    //DrawItemSettings();
    for (int i = 0; i < texts.size(); i++)
    {
        ImGui::PushID(i);
        Text& text = texts[i];
        if (ImGui::CollapsingHeader(text.GetText().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            text.DrawSettings(bigPadding, centerX, itemWidth);
            //居中显示
            ImGui::SetCursorPosX(bigPadding);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.1f, 0.1f, 1.0f)); //红色警示
            if (ImGui::Button(u8"删除",ImVec2(centerX * 2 - bigPadding, 0)))
            {
                texts.erase(texts.begin() + i);
            }
            ImGui::PopStyleColor();
            ImGui::Separator();
        }
        ImGui::PopID();
    }
    ImGui::SetCursorPosX(bigPadding);
    if (ImGui::Button("+", ImVec2(centerX * 2 - bigPadding, 0)))
    {
        texts.emplace_back();
    }
}

void TextItem::Load(const nlohmann::json& j)
{
    LoadItem(j);
    texts.clear();
    if (!j.contains("texts") || !j["texts"].is_array())
        return;

    for (const auto& tj : j["texts"])
    {
        Text text;
        text.Load(tj);
        texts.push_back(std::move(text));
    }
}

void TextItem::Save(nlohmann::json& j) const
{
    SaveItem(j);
    j["texts"] = nlohmann::json::array();
    for (const auto& text : texts)
    {
        nlohmann::json tj;
        text.Save(tj);
        j["texts"].push_back(tj);
    }
}