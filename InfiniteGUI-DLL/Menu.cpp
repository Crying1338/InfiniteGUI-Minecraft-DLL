#include "Menu.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ImGuiStd.h"
#include "App.h"
#include "ConfigManager.h"
#include "FileUtils.h"
#include "BilibiliFansItem.h"
#include "CounterItem.h"
#include "opengl_hook.h"
#include <thread>

#include "AudioManager.h"
#include "ChangeLog.h"
#include "ItemManager.h"
#include "MainMenuButton.hpp"

static ImVec4 myWindowBgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
static ImVec4 tarWindowBgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.3f);
void Menu::RenderGui()
{
    if(!isEnabled)
    {
        return;
    }
    //使窗口显示在屏幕中间
    ImGui::SetNextWindowPos(ImVec2((ImGui::GetIO().DisplaySize.x - ImGui::GetIO().DisplaySize.x / 2), (ImGui::GetIO().DisplaySize.y - ImGui::GetIO().DisplaySize.y / 2)), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2((float)opengl_hook::screen_size.x + 10, (float)opengl_hook::screen_size.y + 10), ImGuiCond_Always);


    //获取io
    ImGuiIO& io = ImGui::GetIO();
    //计算速度
    float speed = 5.0f * std::clamp(io.DeltaTime, 0.0f, 0.05f);
    myWindowBgColor = ImLerp(myWindowBgColor, tarWindowBgColor, speed);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, myWindowBgColor);
    ImGui::Begin(u8"菜单背景", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);
    ImGui::PopStyleColor();
    ImGui::PushFont(NULL, itemStyle.fontSize);
    if (itemStyle.rainbowFont)
        processRainbowFont();
    else
        ImGui::PushStyleColor(ImGuiCol_Text, itemStyle.fontColor); // 字体颜色
    switch (state)
    {
    case MENU_STATE_MAIN:
        ShowMain();
        break;
    case MENU_STATE_SETTINGS:
        ImGui::PushStyleColor(ImGuiCol_WindowBg, itemStyle.bgColor);
        //ImGui::PushStyleColor(ImGuiCol_ChildBg, itemStyle.bgColor);
        ImGui::PushStyleColor(ImGuiCol_Border, itemStyle.borderColor);
        PushRounding(itemStyle.windowRounding);
        panelAnim.blurriness = (float)blur->blurriness_value;
        ShowSettings(&opengl_hook::gui.done);
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(7);
        break;
        
    }
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::End();

    return;
}

void Menu::RenderBeforeGui()
{
    if (blur->menu_blur) blur->RenderBlur(panelAnim.blurriness);
}

void Menu::RenderAfterGui()
{
}

void Menu::OnKeyEvent(bool state, bool isRepeat, WPARAM key)
{
    if (key == NULL || isRepeat) return;
    if (state) //按键按下
    {
        if (key == VK_ESCAPE && isEnabled)
        {
            isEnabled = false;
            Toggle();
        }
    }
}

int Menu::GetKeyBind()
{
    return GetKeybindValue(u8"菜单快捷键：", VK_OEM_5);
}

MainMenuButton* myButton;
void Menu::ShowMain()
{
    if (myButton == nullptr)
    {
        myButton = new MainMenuButton(u8"设置", ImVec2(200.0f, 60.0f));
    }
    bool isBtnHovered = false;
    ////使窗口显示在屏幕中间
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    ImVec4 myColor = colors[ImGuiCol_Button];
    ImVec4 myColorHovered = colors[ImGuiCol_ButtonHovered];
    ImVec4 myColorActive = colors[ImGuiCol_ButtonActive];
    myColor.w = 0.6f;
    myColorHovered.w = 0.6f;
    myColorActive.w = 0.6f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_Button, myColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, myColorHovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, myColorActive);
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetCursorScreenPos(ImVec2((opengl_hook::screen_size.x - 200.0f) / 2, (opengl_hook::screen_size.y - 60.0f) / 2));
    bool isWindowDraging = ImGui::IsMouseDragging(0, 1.0f);
    ImGui::PushFont(NULL, style.FontSizeBase * 1.5f);
    if (myButton->Draw(isWindowDraging))
    {
        settingMenu->Enter();
        state = MENU_STATE_SETTINGS;
        tarWindowBgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.3f);
    }
    if (ImGui::IsItemHovered())
        isBtnHovered = true;
    else
        isBtnHovered = false;
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::PopFont();
    ImGui::PopStyleColor(4);
    float panel_speed = 10.0f * std::clamp(io.DeltaTime, 0.0f, 0.05f);
    if (state == MENU_STATE_MAIN)
    {
        if (isBtnHovered)
        {
            tarWindowBgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
            panelAnim.state = ImLerp(panelAnim.state, 1.0f, panel_speed);
            panelAnim.blurriness = ImLerp(panelAnim.blurriness, (float)blur->blurriness_value, panel_speed);
        }
        else
        {
            tarWindowBgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.3f);
            panelAnim.state = ImLerp(panelAnim.state, 0.0f, panel_speed);
            panelAnim.blurriness = ImLerp(panelAnim.blurriness, 0.0f, panel_speed);
        }
    }
    ShowSidePanels();
}

void Menu::ShowSidePanels() const
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // 动画值（0~1）
    float t = panelAnim.state;

    if (t <= 0.001f)
        return;   // 完全缩回的时候不画，提高性能

    // 面板大小
    ImVec2 panelSize = ImVec2(350, 500);
    // 按钮屏幕坐标（世界坐标）
    ImVec2 Center; 
    Center.x = io.DisplaySize.x / 2;
    Center.y = io.DisplaySize.y / 2;


    // 插值缩放
    float scale = ImLerp(0.0f, 1.0f, t);

    // 最终面板缩放尺寸
    ImVec2 finalSize = ImVec2(panelSize.x * scale, panelSize.y * scale);

    // 左右滑动距离
    float slideOffset = 400.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, scale - 0.2f * (1.0f - scale));
    ImGui::PushTextWrapPos(0.0f); // 设置自动换行
    //======================
    // 左侧 ABOUT 面板动画
    //======================
    {
        ImVec2 targetPos = ImVec2(Center.x  -slideOffset, Center.y);
        ImVec2 currentPos = ImLerp(Center, targetPos, t);

        ImGui::SetNextWindowPos(ImVec2(currentPos.x - finalSize.x * 0.5f, currentPos.y - finalSize.y * 0.5f));
        ImGui::SetNextWindowSize(finalSize);

        //ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(0.7f); // 半透明
        ImGui::Begin("##AboutPanel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
        ImGui::PushFont(opengl_hook::gui.iconFont, 28.0f);
        ImGuiStd::TextShadow("I");
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::PushFont(io.FontDefault, style.FontSizeBase * 1.5f);
        float right = ImGui::GetContentRegionAvail().x;
        float textWidth =
            ImGui::CalcTextSize(u8"公告").x +
            ImGui::GetStyle().ItemSpacing.x;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + right - textWidth);
        ImGuiStd::TextShadow(u8"公告");
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::SetNextWindowBgAlpha(0.3f); // 半透明
        ImGui::BeginChild("AnnounceChild", ImVec2(0, 0), true, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar);
        ImGuiStd::TextShadow(App::Instance().announcement.c_str());
        ImGui::EndChild();
        ImGui::End();

        ImGui::PopStyleVar();
    }

    //====================== 
    // 右侧 更新日志 面板动画
    //======================
    {
        ImVec2 targetPos = ImVec2(Center.x + slideOffset, Center.y);
        ImVec2 currentPos = ImLerp(Center, targetPos, t);

        ImGui::SetNextWindowPos(ImVec2(currentPos.x - finalSize.x * 0.5f, currentPos.y - finalSize.y * 0.5f));
        ImGui::SetNextWindowSize(finalSize);

        //ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(0.7f); // 半透明
        ImGui::Begin("##UpdateLogPanel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
        ImGui::PushFont(io.FontDefault, style.FontSizeBase * 1.5f);
        ImGuiStd::TextShadow(u8"日志");
        ImVec2 separatorPos = ImGui::GetCursorPos();
        ImGui::SameLine();
        ImGui::PopFont();
        ImGui::PushFont(opengl_hook::gui.iconFont, 28.0f);
        float right = ImGui::GetContentRegionAvail().x;
        float textWidth =
            ImGui::GetFontSize() +
            ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + right - textWidth);
        ImGuiStd::TextShadow(u8"\uE034"); //"&#xe034"
        ImGui::PopFont();
        ImGui::SetCursorPos(separatorPos); //separator的位置由字体大小决定，需要重新设置
        ImGui::Separator();
        ImGui::SetNextWindowBgAlpha(0.3f); // 半透明
        ImGui::BeginChild("UpdateLogChild", ImVec2(0, 0), true, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar);
        ChangeLog::Instance().Draw();
        ImGui::EndChild();
        ImGui::End();

        ImGui::PopStyleVar();
    }
    ImGui::PopStyleVar();
    ImGui::PopTextWrapPos();
    //======================
// LOGO 顶部动画面板
//======================
    {
        // 按钮中心
        ImVec2 startPos = Center;

        // 目标位置：向上偏移 100 像素
        ImVec2 targetPos = ImVec2(Center.x, Center.y - 150);

        // 插值当前坐标
        ImVec2 currentPos = targetPos;
            //ImLerp(currentPos, targetPos, t);


        // logo 原始大小
        ImVec2 logoSize = ImVec2(
            (float)opengl_hook::gui.logoTexture.width,
            (float)opengl_hook::gui.logoTexture.height
        );

        // 动画缩放，0~1
        float scale = t;

        // 最终缩放大小
        ImVec2 finalSize = ImVec2(
            logoSize.x * scale,
            logoSize.y * scale
        );

        // logo 窗口透明度（跟着scale淡入）
        float logoAlpha = scale;

        if (finalSize.x > 1 && finalSize.y > 1)
        {
            ImGui::SetNextWindowPos(
                ImVec2(
                    currentPos.x - finalSize.x * 0.5f,
                    currentPos.y - finalSize.y * 0.5f
                )
            );

            ImGui::SetNextWindowSize(finalSize);
            ImGui::SetNextWindowBgAlpha(0.0f); // 完全无背景（相当于纯 Image）

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, logoAlpha);

            ImGui::Begin("##LogoPanel", nullptr,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoBackground
            );

            // 居中绘制图片
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::Image(opengl_hook::gui.logoTexture.id,
                finalSize
            );

            ImGui::End();

            ImGui::PopStyleVar(2);
        }
    }
}

ImVec2 menuInnerSize = ImVec2(900, 556);
ImVec2 menuSize = ImVec2(menuInnerSize.x + 6.0f, menuInnerSize.y + 6.0f);
void Menu::ShowSettings(bool* done)
{
    //使窗口显示在屏幕中间
    if (needRepos)
    {
        ImGui::SetNextWindowPos(
            ImVec2(ImGui::GetIO().DisplaySize.x - ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y - ImGui::GetIO().DisplaySize.y / 2), 
        NULL, 
       ImVec2(0.5f, 0.5f));
        needRepos = false;
    }
    ImGui::SetNextWindowSize(menuSize, ImGuiCond_Once);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::Begin(u8"主控制面板", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor();
    ImGui::SetCursorPos(ImVec2(3, 3));
    //设置控件与左边的间隔
    if (!initialized)
    {
        settingMenu->Init();
        initialized = true;
    }
    if (settingMenu->Draw(opengl_hook::gui.done))
    {
        isEnabled = false;
        Toggle();
    }
    ImGui::End();
}

void Menu::Toggle()
{
    static RECT gameWindowRect;
    if (!isEnabled)
    {
        if (GlobalConfig::Instance().autoSave) {
            ConfigManager::Instance().Save();
            NotificationItem::Instance().AddNotification(NotificationType_Success, u8"自动保存：配置文件已保存。");
        }
        RECT rect;
        // 获取窗口的矩形位置和大小
        if (GetWindowRect(opengl_hook::handle_window, &rect)) {
            // 计算窗口中心的位置
            int centerX = (rect.left + rect.right) / 2;
            int centerY = (rect.top + rect.bottom) / 2;

            // 设置鼠标位置
            SetCursorPos(centerX, centerY);
        }
        ClipCursor(&gameWindowRect);
        myWindowBgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        panelAnim.state = 0.0f;
        panelAnim.blurriness = 0.0f;
        dirtyState.animating = false;
        dirtyState.contentDirty = true; //下一次更新
    }
    else
    {
        needRepos = true;
        GetClipCursor(&gameWindowRect); //保存当前裁剪矩形
        ClipCursor(NULL); // 移除当前的鼠标裁剪矩形，让鼠标恢复为全屏自由移动
        dirtyState.animating = true;
        state = MENU_STATE_MAIN;
    }
}

void Menu::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

    ImGui::SetCursorPosX(bigPadding);
    ImGui::PushItemWidth(itemWidth);
    ImGui::Checkbox(u8"背景模糊", &blur->menu_blur);
    ImGui::SetCursorPosX(bigPadding);
    ImGui::PushItemWidth(bigItemWidth);
    ImGui::SliderInt(u8"模糊强度", &blur->blurriness_value, 0, 10);
    DrawKeybindSettings(bigPadding, centerX, itemWidth);
    DrawSoundSettings(bigPadding, centerX, itemWidth);
    DrawStyleSettings(bigPadding, centerX, itemWidth);
}

void Menu::Load(const nlohmann::json& j)
{
    LoadKeybind(j);
    if(j.contains("menu_blur")) blur->menu_blur = j["menu_blur"].get<bool>();
    if(j.contains("blurriness_value")) blur->blurriness_value = j["blurriness_value"].get<int>();
    LoadSound(j);
    LoadStyle(j);
    //LoadItem(j);
}

void Menu::Save(nlohmann::json& j) const
{
    SaveKeybind(j);
    j["menu_blur"] = blur->menu_blur;
    j["blurriness_value"] = blur->blurriness_value;
    SaveSound(j);
    SaveStyle(j);
    j["type"] = name;
}
