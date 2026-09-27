#pragma once
#include "ConfigManager.h"
#include "ImGuiStd.h"
#include "StringConverter.h"
#include "imgui/imgui.h"

class ConfigSelector
{
public:
    static void Draw(){
        // 当前 profile 名称（utf8）
        std::string curProfile = ConfigManager::Instance().GetCurrentProfile();
        ImVec2 btnSize = ImVec2(196.0f, 0.0f);

        // 按钮索引用来生成唯一 id
        static int uniqueId = 0;

        // 临时 UI 状态（静态以便跨帧保存输入）
        static std::string inputName;           // 用于新增/重命名的输入
        static std::string renameOldName;       // 重命名时要替换的旧名字
        static std::string deleteTargetName;    // 待删除的名字（在确认弹窗中使用）
        static std::string lastErrorMsg;        // 校验/操作失败提示

        // 弹窗标识
        const char* addPopupName = u8"创建";
        const char* renamePopupName = u8"重命名";
        const char* deletePopupName = u8"删除";

        std::vector<std::string> profiles = ConfigManager::Instance().GetProfiles();

        for (size_t i = 0; i < profiles.size(); ++i) {
            std::string profileName = profiles[i];

            // 主按钮：切换配置
            if (ImGui::Button(profileName.c_str(), btnSize)) {
                // Load the selected config (保存当前 -> 切换)
                ConfigManager::Instance().SwitchProfile(profileName, true);
            }

            ImGui::SameLine();

            // 图标按钮（占位/未来用）
            ImGui::PushFont(opengl_hook::gui.iconFont);
            // 重命名按钮（紧跟在 icon 后面；用小字体或图标也行）
            if (ImGui::Button(("*##rename" + std::to_string(i)).c_str())) {
                renameOldName = profileName;
                inputName = profileName; // 预填旧名字
                lastErrorMsg.clear();
                ImGui::OpenPopup(renamePopupName);
            }

            // 删除按钮（只有 profiles > 1 且 不是当前配置时显示）
            if (profiles.size() > 1 && profileName != curProfile)
            {
                ImGui::SameLine();
                if (ImGui::Button((u8"\uE053##del" + std::to_string(i)).c_str())) {
                    // 打开删除确认弹窗
                    deleteTargetName = profileName;
                    lastErrorMsg.clear();
                    ImGui::OpenPopup(deletePopupName);
                }
            }
            else
            {
                //保存按键
                ImGui::SameLine();
                if (ImGui::Button((":##del" + std::to_string(i)).c_str())) {
                    ConfigManager::Instance().Save();
                    ClickSound::Instance().PlaySaveSound();
                    NotificationItem::Instance().AddNotification(NotificationType_Success, u8"配置文件已保存");
                }
            }
            ImGui::PopFont();
        }

        // 新建配置按钮
        if (ImGui::Button("+", btnSize))
        {
            // 生成一个默认唯一名（profile_1, profile_2...）
            std::vector<std::string> profilesNow = ConfigManager::Instance().GetProfiles();
            std::string base = "profile";
            int idx = 1;
            std::string candidate;
            do {
                candidate = base + "_" + std::to_string(idx++);
            } while (std::find(profilesNow.begin(), profilesNow.end(), candidate) != profilesNow.end());
            inputName = candidate;
            lastErrorMsg.clear();
            ImGui::OpenPopup(addPopupName);
        }
        ImGui::SameLine();
        ImGui::PushFont(opengl_hook::gui.iconFont);
        if (ImGui::Button("{")) //打开配置文件夹
        {
            std::wstring path = StringConverter::Utf8ToWstring(FileUtils::configPath + "\\profiles");
            ShellExecute(NULL, NULL, path.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        ImGui::PopFont();

        // ----------------------
    // Create Profile Popup
    // ----------------------
        if (ImGui::BeginPopupModal(addPopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {

            ImGuiStd::TextShadow(u8"输入新配置名称：");
            ImGui::SetNextItemWidth(300.0f);
            // 使用 InputText 的 std::string 版本
            char bufCreate[256];
            std::strncpy(bufCreate, inputName.c_str(), sizeof(bufCreate));
            if (ImGui::InputText("##newProfile", bufCreate, sizeof(bufCreate))) {
                inputName = std::string(bufCreate);
            }

            if (!lastErrorMsg.empty()) {
                ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1.0f), "%s", lastErrorMsg.c_str());
            }

            if (ImGui::Button(u8"创建")) {
                // 校验
                if (inputName.empty()) {
                    lastErrorMsg = u8"名称不能为空。";
                }
                else if (!CheckNameValid(inputName)) {
                    lastErrorMsg = u8"名称只能包含英文数字下划线。";
                }
                else {
                    auto profilesNow = ConfigManager::Instance().GetProfiles();
                    if (std::find(profilesNow.begin(), profilesNow.end(), inputName) != profilesNow.end()) {
                        lastErrorMsg = u8"该名称已存在。";
                    }
                    else {
                        bool ok = ConfigManager::Instance().CreateProfile(inputName);
                        if (!ok) {
                            lastErrorMsg = u8"创建配置文件失败(可能是 IO 错误？)。";
                        }
                        else {
                            ImGui::CloseCurrentPopup();
                            lastErrorMsg.clear();
                            // 可选：自动切换到新配置
                            // ConfigManager::Instance().SwitchProfile(inputName, true);
                        }
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"取消")) {
                ImGui::CloseCurrentPopup();
                lastErrorMsg.clear();
            }

            ImGui::EndPopup();
        }

        // ----------------------
        // Rename Profile Popup
        // ----------------------
        if (ImGui::BeginPopupModal(renamePopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGuiStd::TextShadow(u8"重命名：");
            ImGui::SetNextItemWidth(300.0f);
            char bufRename[256];
            std::strncpy(bufRename, inputName.c_str(), sizeof(bufRename));
            if (ImGui::InputText("##renameProfile", bufRename, sizeof(bufRename))) {
                inputName = std::string(bufRename);
            }

            if (!lastErrorMsg.empty()) {
                ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1.0f), "%s", lastErrorMsg.c_str());
            }

            if (ImGui::Button(u8"确认")) {
                if (inputName.empty()) {
                    lastErrorMsg = u8"名称不能为空。";
                }
                else if (!CheckNameValid(inputName)) {
                    lastErrorMsg = u8"名称只能包含英文数字下划线。";
                }
                else if (inputName == renameOldName) {
                    // 没改名，直接关闭
                    ImGui::CloseCurrentPopup();
                    lastErrorMsg.clear();
                }
                else {
                    auto profilesNow = ConfigManager::Instance().GetProfiles();
                    if (std::find(profilesNow.begin(), profilesNow.end(), inputName) != profilesNow.end()) {
                        lastErrorMsg = u8"该名称已存在。";
                    }
                    else {
                        bool ok = ConfigManager::Instance().RenameProfile(renameOldName, inputName);
                        if (!ok) {
                            lastErrorMsg = u8"创建配置文件失败(可能是 IO 错误？)。";
                        }
                        else {
                            // 若重命名的是当前 profile，需要更新 currentProfile UI 文本（ConfigManager 实现一般会处理）
                            ImGui::CloseCurrentPopup();
                            lastErrorMsg.clear();
                        }
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"取消")) {
                ImGui::CloseCurrentPopup();
                lastErrorMsg.clear();
            }
            ImGui::EndPopup();
        }

        // ----------------------
        // Delete Confirmation Popup
        // ----------------------
        if (ImGui::BeginPopupModal(deletePopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::string deleteTargetNameUtf8 = StringConverter::AcpToUtf8(deleteTargetName);
            std::string deleteTargetNameUtf8Confirm = u8"确认删除\"" + deleteTargetNameUtf8 + u8"\"?";
            ImGuiStd::TextShadow(deleteTargetNameUtf8Confirm.c_str());
            if (!lastErrorMsg.empty()) {
                ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1.0f), "%s", lastErrorMsg.c_str());
            }

            if (ImGui::Button(u8"删除")) {
                // 调用删除（ConfigManager 会阻止删除当前 profile 或保留至少一个）
                bool ok = ConfigManager::Instance().DeleteProfile(deleteTargetName);
                if (!ok) {
                    lastErrorMsg = u8"删除失败，可能是因为它是当前配置或只有一个配置。";
                }
                else {
                    ImGui::CloseCurrentPopup();
                    lastErrorMsg.clear();
                    // 如果用户刚删了当前之外的 profile，通常不需额外操作
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"取消")) {
                ImGui::CloseCurrentPopup();
                lastErrorMsg.clear();
            }
            ImGui::EndPopup();
        }
    }
private:
    static bool CheckNameValid(const std::string& inputName)
    {
        bool valid = true;
        for (auto c : inputName) {
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
                valid = false;
                break;
            }
        }
        return valid;
    }
};
