#include "GameStateDetector.h"
#include "App.h"
#include "GameWindowTool.h"
#include "KeyState.h"
#include "Menu.h"
void GameStateDetector::Toggle()
{
}

static bool IsClipCursorSmallerThanScreen(int margin = 8)
{
	RECT clip;
	if (!GetClipCursor(&clip))
		return true;

	const int screenW = GetSystemMetrics(SM_CXSCREEN);
	const int screenH = GetSystemMetrics(SM_CYSCREEN);

	const int clipW = clip.right - clip.left;
	const int clipH = clip.bottom - clip.top;

	// 1. 尺寸明显小于屏幕
	if (clipW < screenW - margin * 2 ||
		clipH < screenH - margin * 2)
	{
		return true;
	}

	// 2. 边界有明显内缩
	if (clip.left > margin ||
		clip.top > margin ||
		clip.right < screenW - margin ||
		clip.bottom < screenH - margin)
	{
		return true;
	}

	return false;
}

static bool IsFullScreen()
{
	const int screenW = GetSystemMetrics(SM_CXSCREEN);
	const int screenH = GetSystemMetrics(SM_CYSCREEN);

	
	if (opengl_hook::screen_size.x == screenW &&
		opengl_hook::screen_size.y == screenH )
	{
		return true;
	}

	return false;
}

void GameStateDetector::Update()
{
	if(KeyState::GetKeyDown(GameWindowTool::Instance().GetFullscreenVK()))
		lastClickTime = std::chrono::steady_clock::now();

	if (Menu::Instance().isEnabled)
		currentState = InMenu;
	else if (IsMouseCursorVisible())
		currentState = InGameMenu;
	else
		currentState = InGame;
	if (IsFullScreen())
		windowState = FullScreen;
	else
		windowState = NormalWindow;

	isInGameWindow = opengl_hook::handle_window == GetForegroundWindow();

	if (currentState != lastState)
	{
		GameKeyBind::Instance().Load(FileUtils::optionsPath);
		dirtyState.contentDirty = true;
		lastState = currentState;
	}

}

void GameStateDetector::Load(const nlohmann::json& j)
{
	LoadItem(j);
	if (j.contains("hideItemInGui")) hideItemInGui = j["hideItemInGui"].get<bool>();
}

void GameStateDetector::Save(nlohmann::json& j) const
{
	SaveItem(j);
	j["hideItemInGui"] = hideItemInGui;
}

void GameStateDetector::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
	ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
	ImGui::BeginDisabled();
	ImGuiStd::TextShadow(u8"游戏状态检测设置");
	ImGui::EndDisabled();
	ImGui::PopFont();
	ImGui::SetCursorPosX(bigPadding);
	ImGui::PushItemWidth(itemWidth);
	ImGui::Checkbox(u8"仅在游戏时显示Gui", &hideItemInGui);
	ImGui::SameLine(); ImGuiStd::HelpMarker(u8"打开背包、暂停、打字等非游戏时Gui的显示。");
}

bool GameStateDetector::IsNeedHide() const 
{
	return currentState == InGameMenu && hideItemInGui;
}

GameState GameStateDetector::GetCurrentState() const
{
	return currentState;
}

WindowState GameStateDetector::GetWindowState() const
{
	return windowState;
}

bool GameStateDetector::IsInGameWindow() const
{
	return isInGameWindow;
}

bool GameStateDetector::IsInGame() const
{
	return currentState == InGame;
}

static bool IsCursorHidden(const CURSORINFO& ci)
{
	static const auto func = [&]() -> bool
		{
			static const std::array<LPCWSTR, 13> SysIds = {
			   IDC_ARROW, IDC_IBEAM, IDC_WAIT, IDC_CROSS,
			   IDC_UPARROW, IDC_SIZEALL, IDC_SIZENWSE,
			   IDC_SIZENESW, IDC_SIZENS, IDC_SIZEWE,
			   IDC_HAND, IDC_NO, IDC_APPSTARTING
			};
			// static来缓存加载后的位图，不需要重复加载，这里用到了lambda表达式
			static const std::array<HCURSOR, 13> SysCursorList = {
				[]() {
					std::array<HCURSOR, 13> cursors{};
					unsigned int index = 0;
					for (auto id : SysIds)
					{
						cursors[index] = LoadCursor(nullptr, id);
						index++;
					}
					return  cursors;
				}()
			};
			return std::any_of(
				SysCursorList.begin(),
				SysCursorList.end(),
				[&](const HCURSOR cursor)
				{
					return cursor == ci.hCursor;
				}
			);
		};
	// 0 表示光标隐藏，CURSOR_SHOWING (0x00000001) 表示光标可见
	if ((ci.flags & CURSOR_SHOWING) != 0)
		return !func();
	return false;
}

bool GameStateDetector::IsMouseCursorVisible()
{
	CURSORINFO ci;
	ci.cbSize = sizeof(ci);

	if (GetCursorInfo(&ci))
	{
		if (!(ci.flags & CURSOR_SHOWING)) return false;
	}
	return !IsCursorHidden(ci);
}

void GameStateDetector::ProcessMouseMovement(int dx, int dy)
{
	// 鼠标移动速度 = 模长
	cameraSpeed = sqrtf((float)dx * (float)dx + (float)dy * (float)dy);

	// 视角是否移动（为了给你逻辑判断）
	if (cameraSpeed > movementThreshold)
		cameraMoving = true;
	else
		cameraMoving = false;
	if(Menu::Instance().isEnabled || !IsInGame())
	{
		cameraMoving = false;
		cameraSpeed = 1.0f;
	}
}

bool GameStateDetector::IsCameraMoving() const
{
	return cameraMoving;
}

float GameStateDetector::GetCameraSpeed() const
{
	return cameraSpeed;
}
