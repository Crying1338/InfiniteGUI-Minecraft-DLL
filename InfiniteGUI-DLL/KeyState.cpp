#include "KeyState.h"

#include "opengl_hook.h"


bool KeyState::GetKeyClick(int key)
{
	if (key <= 0)
		return false;
	//static bool PrevKeyState[512] = { false };

	bool CurrKeyState = GetKeyDown(key);
	bool IsClick = false;
	if (PrevKeyState[key] != CurrKeyState)
	{
		IsClick = CurrKeyState;
		PrevKeyState[key] = CurrKeyState;
	}
	return IsClick;
}

bool KeyState::GetKeyDown(int key)
{
	return GetAsyncKeyState(key) & 0x8000;
}

bool KeyState::GetKeyUp(int key)
{
	if (key <= 0)
		return false;
	//static bool PrevKeyState[512] = { false };
	bool CurrKeyState = GetKeyDown(key);
	bool IsReleased = PrevKeyState[key] && !CurrKeyState;
	PrevKeyState[key] = CurrKeyState;
	return IsReleased;
}

void KeyState::SetKeyDown(int key, int mode)
{
	switch (mode)
	{
	case SC_MOUSE_EVENT:
	{
		if (key == Click::Left || key == Click::Right)
			mouse_event((key == Click::Left) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
		else
			keybd_event(key, 0, 0, 0);
		break;
	}
	case SC_SEND_INPUT:
	{
		INPUT input{};
		if (key == Click::Left || key == Click::Right)
		{
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			SendInput(1, &input, sizeof(INPUT));
		}
		else
		{
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;
			input.ki.dwFlags = 0;
			SendInput(1, &input, sizeof(INPUT));
		}
		break;
	}
	default:
	case SC_POST_MESSAGE:
	{
		const HWND hwnd = opengl_hook::handle_window;
		if (key == Click::Left || key == Click::Right)
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(hwnd, &point);
			PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
		}
		else
			PostMessage(hwnd, WM_KEYDOWN, key, 0);
		break;
	}
	}
}

void KeyState::SetKeyUp(int key, int mode)
{
	switch (mode)
	{
	case SC_MOUSE_EVENT:
	{
		if (key == Click::Left || key == Click::Right)
			mouse_event((key == Click::Left) ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
		else
			keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
		break;
	}
	case SC_SEND_INPUT:
	{
		INPUT input{};
		if (key == Click::Left || key == Click::Right)
		{
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			SendInput(1, &input, sizeof(INPUT));
		}
		else
		{
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));
		}
		break;
	}
	default:
	case SC_POST_MESSAGE:
	{
		const HWND hwnd = opengl_hook::handle_window;
		if (key == Click::Left || key == Click::Right)
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(hwnd, &point);
			PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(point.x, point.y));
		}
		else
			PostMessage(hwnd, WM_KEYUP, key, 0);
		break;
	}
	}
}

void KeyState::SetKeyClick(int key, int sleep, int mode)
{
	switch (mode)
	{
	case SC_MOUSE_EVENT:
	{
		if (key == Click::Left || key == Click::Right)
		{
			mouse_event((key == Click::Left) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			if (sleep > 0) Sleep(sleep);
			mouse_event((key == Click::Left) ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
		}
		else
		{
			keybd_event(key, 0, 0, 0);
			if (sleep > 0) Sleep(sleep);
			keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
		}
		break;
	}
	case SC_SEND_INPUT:
	{
		INPUT input{};
		if (key == Click::Left || key == Click::Right)
		{
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			SendInput(1, &input, sizeof(INPUT));
			if (sleep > 0) Sleep(sleep);
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			SendInput(1, &input, sizeof(INPUT));
		}
		else
		{
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));
			input.ki.dwFlags = 0;
			SendInput(1, &input, sizeof(INPUT));
		}
		break;
	}
	default:
	case SC_POST_MESSAGE:
	{
		const HWND hwnd = opengl_hook::handle_window;
		if (key == Click::Left || key == Click::Right)
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(hwnd, &point);
			PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
			if (sleep > 0) Sleep(sleep);
			PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(point.x, point.y));
		}
		else
		{
			PostMessage(hwnd, WM_KEYDOWN, key, 0);
			if (sleep > 0) Sleep(sleep);
			PostMessage(hwnd, WM_KEYUP, key, 0);
		}
		break;
	}
	}
}