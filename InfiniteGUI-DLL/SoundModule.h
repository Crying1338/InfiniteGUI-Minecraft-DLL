#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "ImGuiStd.h"
class SoundModule
{
public:

    void DrawSoundSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
    {
        ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
        ImGui::BeginDisabled();
        ImGuiStd::TextShadow(u8"声音设置");
        ImGui::EndDisabled();
        ImGui::PopFont();

        float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;
        // ===== 左：提示音 =====
        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(itemWidth);

        ImGui::Checkbox(u8"提示音", &isPlaySound);

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(bigItemWidth);
        ImGui::SliderFloat(u8"音量", &soundVolume, 0.0f, 1.0f, "%.2f");
    }
    bool IsPlaySound() const { return isPlaySound; }
    void SetPlaySound(bool playSound) { isPlaySound = playSound; }
protected:
    void LoadSound(const nlohmann::json& j)
    {
        if (j.contains("isPlaySound")) isPlaySound = j["isPlaySound"];
        if (j.contains("soundVolume")) soundVolume = j["soundVolume"];
    }
    void SaveSound(nlohmann::json& j) const
    {
        j["isPlaySound"] = isPlaySound;
        j["soundVolume"] = soundVolume;
    }
    void ResetSound()
    {
        isPlaySound = true;
        soundVolume = 0.5f;
    }
    bool isPlaySound;
    float soundVolume;    // 声音音量（0.0~1.0）
};