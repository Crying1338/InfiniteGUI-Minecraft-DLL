#include "GameWindowTool.h"

#include "GameStateDetector.h"
#include "NotificationItem.h"

GameWindowTool::~GameWindowTool()
{
    if (windowStyleState.isBorderlessFullscreen) SetBorderlessFullscreen(opengl_hook::handle_window);
}

void GameWindowTool::Toggle()
{
}

void GameWindowTool::OnKeyEvent(bool state, bool isRepeat, WPARAM key)
{
}

void GameWindowTool::Update()
{
    if (!GameStateDetector::Instance().IsInGameWindow() || GameStateDetector::Instance().GetCurrentState() == InMenu) return;
    if (keyStateHelper.GetKeyClick(GetKeybindValue(u8"无边框全屏快捷键：", 0)))
    {
        SetBorderlessFullscreen(opengl_hook::handle_window);
        RECT area;
        GetClientRect(opengl_hook::handle_window, &area);

        opengl_hook::screen_size.x = area.right - area.left;
        opengl_hook::screen_size.y = area.bottom - area.top;
    }
}

void GameWindowTool::Load(const nlohmann::json& j)
{
    LoadItem(j);
    LoadKeybind(j);
}

void GameWindowTool::Save(nlohmann::json& j) const
{
    SaveItem(j);
    SaveKeybind(j);
}

void GameWindowTool::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
    ImGui::BeginDisabled();
    ImGuiStd::TextShadow(u8"游戏窗口工具");
    ImGui::EndDisabled();
    ImGui::PopFont();

    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

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
    if (!GameKeyBind::Instance().IsSuccess())
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

void GameWindowTool::SetBorderlessFullscreen(HWND hwnd)
{
    if (!hwnd) return;

    if (windowStyleState.isBorderlessFullscreen) {
        SetWindowLong(hwnd, GWL_STYLE, windowStyleState.style);
        SetWindowPos(hwnd, HWND_NOTOPMOST,
            windowStyleState.rect.left, windowStyleState.rect.top,
            windowStyleState.rect.right - windowStyleState.rect.left,
            windowStyleState.rect.bottom - windowStyleState.rect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        windowStyleState.isBorderlessFullscreen = false;
        NotificationItem::Instance().AddNotification(NotificationType_Info, u8"已退出无边框全屏。");
        return;
    }
    if (GameStateDetector::Instance().GetWindowState() == FullScreen)
    {
        NotificationItem::Instance().AddNotification(NotificationType_Warning, u8"无法在游戏全屏(f11)时\n开启无边框全屏。");
        return;
    }
    windowStyleState.style = GetWindowLong(hwnd, GWL_STYLE);
    GetWindowRect(hwnd, &windowStyleState.rect);

    LONG style = windowStyleState.style & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    SetWindowLong(hwnd, GWL_STYLE, style);

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    windowStyleState.isBorderlessFullscreen = true;
    NotificationItem::Instance().AddNotification(NotificationType_Info, u8"已开启无边框全屏。");
}
