#pragma once
#include "ConfigSelector.h"
#include "FontSelector.h"
#include "GameKeyBind.h"
#include "menuRule.h"
#include "ItemManager.h"
class GlobalSettings
{
public:
	GlobalSettings()
	{
		m_configSelector = new ConfigSelector();
		m_fontSelector = new FontSelector();
	}
	~GlobalSettings()
	{
		delete m_configSelector;
		delete m_fontSelector;
	}
	void Draw()
	{
		bool exit = false;
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
		//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 8.0f));
		ImGui::BeginChild("GlobalSettings", ImVec2(-padding + ImGui::GetStyle().WindowPadding.x, -padding + ImGui::GetStyle().WindowPadding.y), true, flags);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));

		// ===== 布局参数 =====
		ImGuiStyle& style = ImGui::GetStyle();
		float basePadding = style.WindowPadding.x;
		float bigPadding = basePadding * 3.0f;

		// 获取窗口可用宽度
		float contentWidth = ImGui::GetContentRegionAvail().x;
		float centerX = contentWidth * 0.5f;

		// 每个输入框宽度（留点余量，避免顶到边）
		float itemWidth = centerX - bigPadding * 4.0f;


		ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
		ImGui::BeginDisabled();
		ImGuiStd::TextShadow(u8"全局基础设置");
		ImGui::EndDisabled();
		ImGui::PopFont();

		float startY = ImGui::GetCursorPosY();

		ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
		ImGui::BeginDisabled();
		ImGuiStd::TextShadow(u8"配置选择：");
		ImGui::EndDisabled();
		ImGui::PopFont();

		ImGui::SetCursorPosX(bigPadding);
		ImGui::BeginChild("ConfigSelector", ImVec2(centerX - bigPadding, 192.0f), true, ImGuiWindowFlags_NoScrollbar);
		m_configSelector->Draw();
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(centerX + basePadding, startY));

		ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
		ImGui::BeginDisabled();
		ImGuiStd::TextShadow(u8"字体选择：");
		ImGui::EndDisabled();
		ImGui::PopFont();

		ImGui::SetCursorPosX(bigPadding + centerX);
		ImGui::BeginChild("FontSelector", ImVec2(centerX - bigPadding, 192.0f), true, ImGuiWindowFlags_NoScrollbar);
		m_fontSelector->Draw();
		ImGui::EndChild();

		ImGui::SetCursorPosX(bigPadding);
		ImGui::PushItemWidth(itemWidth);
		ImGui::Checkbox(u8"开启限帧优化", &GlobalConfig::Instance().enableOptimization);
		ImGui::SameLine();
		ImGuiStd::HelpMarker(u8"主动降低自身的渲染计算频率以优化性能。\nGUI是否刷新取决于窗口内容是否发生变化以及是否处于动画状态。\n    在界面静止时，无限GUI 会复用已有渲染结果；\n    在动画过程中，刷新频率最高限制为显示器的刷新率，以保证视觉流畅并避免无意义的高频计算。\n这种按需刷新机制在性能与体验之间取得了最佳平衡。");
		ImGui::SameLine();
		ImGui::SetCursorPosX(bigPadding + centerX);
		ImGui::PushItemWidth(itemWidth);

		ImGui::Checkbox(u8"自动保存", &GlobalConfig::Instance().autoSave);


		ImGui::SetCursorPosX(bigPadding);
		ImGui::PushItemWidth(itemWidth);
		ImGui::Checkbox(u8"显示游戏键位绑定界面", &m_showKeyBindUI);
		ImGui::SameLine();
		ImGuiStd::HelpMarker(u8"无限Gui会在激活时读取游戏配置文件，从而获取按键绑定。\n这些绑定按键只作用于无限Gui，不会因为修改而影响游戏的正常操作。");

		for(auto& item : ItemManager::Instance().GetItems())
		{
			if(item->type == Hidden)
				item->DrawSettings(bigPadding, centerX, itemWidth);
		}
		if(m_showKeyBindUI)
			GameKeyBind::Instance().DrawKeyBindUI();

		ImGui::PopStyleVar();
		ImGui::EndChild();
	}
private:
	ConfigSelector *m_configSelector;
	FontSelector *m_fontSelector;
	bool m_showKeyBindUI;
};
