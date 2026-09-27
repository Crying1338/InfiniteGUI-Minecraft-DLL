#pragma once
#include <thread>

#include "menuRule.h"
#include "App.h"
#include "ChangeLog.h"
#include "ImGuiStd.h"
#include "imgui/imgui.h"
#include "FileUtils.h"
#include "StringConverter.h"

class AboutsPanel
{
public:
	static void Draw()
	{
		bool exit = false;
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
		//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 8.0f));
		ImGui::BeginChild("About", ImVec2(-padding + ImGui::GetStyle().WindowPadding.x, -padding + ImGui::GetStyle().WindowPadding.y), true, flags);
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
		float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;
		
        ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
        ImGui::BeginDisabled();
        ImGuiStd::TextShadow(u8"关于");
        ImGui::EndDisabled();
        ImGui::PopFont();

        ImGui::SetCursorPosX(bigPadding);
        ImGuiStd::TextShadow(App::Instance().appName.c_str());
        ImGui::SameLine();
        std::string appVersion = std::to_string(App::Instance().appVersion.major) + "." + std::to_string(App::Instance().appVersion.minor) + "." + std::to_string(App::Instance().appVersion.build);
        ImGuiStd::TextShadow(("v" + appVersion).c_str());
        ImGui::SameLine();
        static std::atomic<bool> checkingUpdate = false;   // 是否在检查
        static std::atomic<bool> updateFinished = false;   // 检查是否完成
        static bool updateHasNew = false;                  // 是否发现新版本
        // 点击按钮：开始异步检查
        if (ImGui::Button(u8"检查更新") && !checkingUpdate)
        {
            // 打开“正在检查”窗口
            ImGui::OpenPopup(u8"-->检查更新...");

            checkingUpdate = true;
            updateFinished = false;

            std::thread([] {
                bool result = App::Instance().CheckUpdate();
                updateHasNew = !result;   // result=false => 有新版本
                updateFinished = true;
                checkingUpdate = false;
                }).detach();
        }
        if (ImGui::BeginPopupModal(u8"-->检查更新...", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (checkingUpdate)
            {
                ImGuiStd::TextShadow(u8"正在检查更新，请稍候...");
            }
            else if (updateFinished)
            {
                if (updateHasNew)
                {
                    ImGuiStd::TextShadow(u8"发现新版本！");
                    std::string cloudVersion =
                        std::to_string(App::Instance().cloudVersion.major) + "." +
                        std::to_string(App::Instance().cloudVersion.minor) + "." +
                        std::to_string(App::Instance().cloudVersion.build);

                    ImGuiStd::TextShadow((u8"最新版本：" + cloudVersion).c_str());
                }
                else
                    ImGuiStd::TextShadow(u8"目前已是最新版本");
                if (ImGui::Button(u8"确定"))
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(bigPadding + centerX);
        ImGuiStd::TextShadow(u8"用户协议：");
        ImGui::SameLine();
        if (ImGui::Button(u8"InfiniteGui-License"))
        {
            std::wstring path = StringConverter::Utf8ToWstring(FileUtils::modulePath) + L"\\InfiniteGui-License.txt";
            ShellExecute(NULL, NULL, path.c_str(), NULL, NULL, SW_SHOWNORMAL);

        }

        ImGui::SetCursorPosX(bigPadding);
        ImGuiStd::TextShadow(u8"作者：");
        ImGui::SameLine();
        if (ImGui::Button(App::Instance().appAuthor.c_str()))
        {
            ShellExecute(NULL, NULL, L"https://space.bilibili.com/399194206", NULL, NULL, SW_SHOWNORMAL);
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(bigPadding + centerX);
        ImGuiStd::TextShadow(u8"相关链接：");
        ImGui::SameLine();
        if (ImGui::Button(u8"爱发电"))
        {
            ShellExecute(NULL, NULL, L"https://ifdian.net/a/qc_max", NULL, NULL, SW_SHOWNORMAL);
        }
        ImGui::SameLine();
        ImGuiStd::TextShadow(u8" & ");

        ImGui::SameLine();
        if (ImGui::Button(u8"GitHub"))
        {
            ShellExecute(NULL, NULL, L"https://github.com/QCMaxcer/InfiniteGUI-Minecraft-DLL", NULL, NULL, SW_SHOWNORMAL);
        }

        float startY = ImGui::GetCursorPosY();

        ImGui::PushFont(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f);
        ImGui::BeginDisabled();
        ImGuiStd::TextShadow(u8"公告");
        ImGui::EndDisabled();
        ImGui::PopFont();


        ImGui::SetCursorPosX(bigPadding);
		ImGui::BeginChild("AnnounceChild", ImVec2(centerX - bigPadding, -basePadding), true, ImGuiWindowFlags_NoScrollbar);
		ImGuiStd::TextShadow(App::Instance().announcement.c_str());
		ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(basePadding + centerX, startY));
        ImGui::PushFont(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f);
        ImGui::BeginDisabled();
        ImGuiStd::TextShadow(u8"更新记录");
        ImGui::EndDisabled();
        ImGui::PopFont();

        ImGui::SetCursorPosX(bigPadding + centerX);
        ImGui::BeginChild("ChangelogChild", ImVec2(centerX - bigPadding, -basePadding), true, ImGuiWindowFlags_NoScrollbar);
        ChangeLog::Instance().Draw();
        ImGui::EndChild();

		ImGui::PopStyleVar();
		ImGui::EndChild();

	}
private:

};
