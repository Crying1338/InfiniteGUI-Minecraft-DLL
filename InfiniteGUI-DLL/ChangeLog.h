#pragma once
#include <string>
#include <vector>
#include <deque>

//一个version对应一个date
//一个version对应多个log

struct Info
{
	std::string version;
	std::string date;
};

enum ContentType
{
	FIXBUG, //修复
	ADD, //新增
	REMOVE, //移除
	OPTIMIZE, //优化
	CHANGE, //变更
	INFO //信息
};

struct Content
{
	ContentType type;
	std::string content;
};

struct Log
{
	Info info;
	std::vector<Content> contents;
};

typedef std::deque<Log> Logs;

class ChangeLog
{
	ChangeLog();
public:
	static ChangeLog& Instance()
	{
		static ChangeLog instance;
		return instance;
	}
	void Draw() const; //绘制日志
	void Init();
private:
	void AddContent(const ContentType& type, const std::string& content);
	void NewVersion(const std::string& version, const std::string& date);

	static void DrawSingleLog(const Log& log);

	static void DrawVersion(const Info& info);
	static void DrawContent(const Content& content);

	Logs logs;
};

