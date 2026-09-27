#pragma once
#include <Windows.h>
#include <WinUser.h>

enum SendInputMode
{
	SC_MOUSE_EVENT = 0,
	SC_SEND_INPUT = 1,
	SC_POST_MESSAGE = 2
};

class KeyState
{
public:
	static bool GetKeyDown(int key);
	bool GetKeyUp(int key);
	bool GetKeyClick(int key);
	static void SetKeyDown(int key, int mode = SC_POST_MESSAGE);
	static void SetKeyUp(int key, int mode = SC_POST_MESSAGE);
	static void SetKeyClick(int key, int sleep = 20, int mode = SC_POST_MESSAGE);

	private:
		bool PrevKeyState[512] = { false };
};


enum Click
{
	Right = VK_RBUTTON,
	Left = VK_LBUTTON
};

