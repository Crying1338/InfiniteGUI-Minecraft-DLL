#pragma once
#include <string>

struct version
{
	int major;
	int minor;
	int build;
};

class App
{
public:
	std::string appName = u8"无限GUI";
	version appVersion = { 1, 0, 5 };
	std::string appAuthor = u8"路过的Max";
	std::string appDescription = "InfiniteGUI is a library that allows you to create overlays for in-game information.";
	std::string announcement = u8"这里没有公告捏...";
	version cloudVersion = { 0, 0, 0 };
	std::wstring versionUrl = L"https://gitee.com/qc_max/InfiniteGUI/raw/master/version.json";
	std::wstring announcementUrl = L"https://gitee.com/qc_max/InfiniteGUI/raw/master/announcement.txt";
	static App& Instance()
	{
		static App instance;
		return instance;
	}

	void GetAnnouncement();

	bool CheckUpdate();
};