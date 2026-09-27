#pragma once
#include <vector>

#include "FileUtils.h"
#include "fonts.h"
#include "GlobalConfig.h"
#include "opengl_hook.h"
#include "imgui/imgui.h"

struct FontInfo {
    std::string name;
    std::wstring path;
};

class FontSelector
{
public:
    void Draw() {
        ImGuiIO& io = ImGui::GetIO();
        std::string* fontPath = &GlobalConfig::Instance().fontPath;
        // Get font files if not loaded yet
        if (fontFilesSystem.empty()) {
            fontFilesSystem = GetFontsFromDirectory(L"C:\\Windows\\Fonts");
        }

        if (fontFilesUser.empty()) {
            // 获取用户名
            wchar_t username[256]; // 确保足够空间来存储用户名
            DWORD usernameSize = sizeof(username) / sizeof(username[0]);
            GetUserNameW(username, &usernameSize);
            fontFilesUser = GetFontsFromDirectory(L"C:\\Users\\" + std::wstring(username) + L"\\AppData\\Local\\Microsoft\\Windows\\Fonts");
        }

        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;
        font_cfg.OversampleH = 1;
        font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
        
        //显示当前字体
        ImGuiStd::TextShadow(u8"当前字体:");
        ImGui::SameLine();



        ImGuiStd::TextShadow(*fontPath == "default" ? u8"阿里巴巴普惠体" : opengl_hook::gui.font->GetDebugName());

        if (ImGui::CollapsingHeader(u8"默认字体", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Selectable(u8"阿里巴巴普惠体")) {
                // Load the selected font
                opengl_hook::gui.font = io.Fonts->AddFontFromMemoryTTF(Fonts::alibaba.data, Fonts::alibaba.size, 20.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
                io.FontDefault = opengl_hook::gui.font;
                *fontPath = "default";
            }
        }

        if (ImGui::CollapsingHeader(u8"用户字体"))
        {
            for (size_t i = 0; i < fontFilesUser.size(); ++i) {
                if (ImGui::Selectable(fontFilesUser[i].name.c_str())) {
                    // Load the selected font
                    opengl_hook::gui.font = io.Fonts->AddFontFromFileTTF(StringConverter::WstringToUtf8(fontFilesUser[i].path).c_str(), 20.0f, &font_cfg, ImGui::GetIO().Fonts->GetGlyphRangesChineseFull());
                    io.FontDefault = opengl_hook::gui.font;
                    *fontPath = StringConverter::WstringToUtf8(fontFilesUser[i].path);
                }
            }
        }

        if (ImGui::CollapsingHeader(u8"系统字体"))
        {
            for (size_t i = 0; i < fontFilesSystem.size(); ++i) {
                if (ImGui::Selectable(fontFilesSystem[i].name.c_str())) {
                    // Load the selected font
                    opengl_hook::gui.font = io.Fonts->AddFontFromFileTTF(StringConverter::WstringToUtf8(fontFilesSystem[i].path).c_str(), 20.0f, &font_cfg, ImGui::GetIO().Fonts->GetGlyphRangesChineseFull());
                    io.FontDefault = opengl_hook::gui.font;
                    *fontPath = StringConverter::WstringToUtf8(fontFilesSystem[i].path);
                }
            }
        }
    }
private:
    std::vector<FontInfo> fontFilesSystem;
    std::vector<FontInfo> fontFilesUser;

    static std::vector<FontInfo> GetFontsFromDirectory(const std::wstring& directory) {
        std::vector<FontInfo> fontInfos;

        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile((directory + L"\\*").c_str(), &findFileData);

        if (hFind == INVALID_HANDLE_VALUE)
            return fontInfos;

        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring filename = findFileData.cFileName;
                if (filename.find(L".ttf") != std::string::npos || filename.find(L".otf") != std::string::npos) {
                    FontInfo fontInfo;
                    //去除后缀.ttf
                    fontInfo.name = StringConverter::WstringToUtf8(filename);
                    fontInfo.name = fontInfo.name.substr(0, fontInfo.name.find_last_of("."));
                    fontInfo.path = directory + L"\\" + filename;
                    fontInfos.push_back(fontInfo);
                }
            }
        } while (FindNextFile(hFind, &findFileData) != 0);

        FindClose(hFind);
        return fontInfos;
    }

};
