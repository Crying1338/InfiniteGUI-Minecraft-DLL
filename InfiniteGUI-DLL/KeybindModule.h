#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include "ImguiStd.h"
#include "KeyState.h"

class KeybindModule
{
public:
	virtual void OnKeyEvent(bool state, bool isRepeat, WPARAM key) = 0;

	// ---- 安全读取按键绑定 ----
	// std::map::at 在键不存在时会抛 std::out_of_range；这些回调运行在
	// WndProc / 渲染线程上，异常穿透到 JVM 会导致游戏崩溃，因此一律用安全查找。
	int GetKeybindValue(const std::string& name, int fallback = 0) const
	{
		auto it = keybinds.find(name);
		return it == keybinds.end() ? fallback : it->second;
	}
	int GetGameKeybindValue(const std::string& name, int fallback = 0) const
	{
		auto it = gameKeybinds.find(name);
		return it == gameKeybinds.end() ? fallback : it->second;
	}
	// 取或创建（用于容错：确保键位一定存在于表中）
	int& EnsureKeybind(const std::string& name, int def)
	{
		return keybinds.emplace(name, def).first->second;
	}
	int& EnsureGameKeybind(const std::string& name, int def)
	{
		return gameKeybinds.emplace(name, def).first->second;
	}

    void DrawKeybindSettings(const float& bigPadding,const float& centerX,const float& itemWidth)
    {
        ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
        ImGui::BeginDisabled();
        ImGuiStd::TextShadow(u8"按键绑定设置");
        ImGui::EndDisabled();
        ImGui::PopFont();

        ImGui::SetCursorPosX(bigPadding);
        ImGui::SetNextItemWidth(itemWidth);
        ImGui::Checkbox(u8"自定义游戏按键绑定", &customGameKeybinds);
        // 当前起始 Y
        float startY = ImGui::GetCursorPosY();
        float itemHeight = ImGui::GetFrameHeightWithSpacing();

        int index = 0;

        for (auto& [name, key] : keybinds)
        {
            bool isLeft = (index % 2 == 0);

            float x = isLeft
                ? bigPadding
                : centerX + bigPadding;

            float y = startY + (index / 2) * itemHeight;

            ImGui::SetCursorPos(ImVec2(x, y));
            ImGui::SetNextItemWidth(itemWidth);

            ImGuiStd::Keybind(name.c_str(), key);
            index++;
        }
        if (customGameKeybinds)
        {
            for (auto& [name, key] : gameKeybinds)
            {
                bool isLeft = (index % 2 == 0);

                float x = isLeft
                    ? bigPadding
                    : centerX + bigPadding;

                float y = startY + (index / 2) * itemHeight;

                ImGui::SetCursorPos(ImVec2(x, y));
                ImGui::SetNextItemWidth(itemWidth);

                ImGuiStd::Keybind(name.c_str(), key);
                index++;
            }
        }

    }
protected:
    void LoadKeybind(const nlohmann::json& j)
    {
        if(j.contains("customGameKeybinds")) customGameKeybinds = j["customGameKeybinds"];
        if(!keybinds.empty())
        {
            for (auto& [name, key] : keybinds)
            {
                if (j.contains(name))
                {
                    key = j[name];
                }
            }
        }
        if (!gameKeybinds.empty())
        {
            for (auto& [name, key] : gameKeybinds)
            {
                if (j.contains(name))
                {
                    key = j[name];
                }
            }
        }
    }
    void SaveKeybind(nlohmann::json& j) const
    {
        j["customGameKeybinds"] = customGameKeybinds;
        for (auto& [name, key] : keybinds)
        {
            j[name] = key;
        }
        for (auto& [name, key] : gameKeybinds)
        {
            j[name] = key;
        }
    }

    void ResetKeybind()
    {
        keybinds.clear();
    }

	std::map<std::string, int> keybinds; //name, key
    std::map<std::string, int> gameKeybinds; //name, key
    bool customGameKeybinds = false;
    KeyState keyStateHelper;
};
