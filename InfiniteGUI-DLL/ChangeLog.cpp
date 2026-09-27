#include "imgui\imgui.h"
#include "ImGuiStd.h"
#include "ChangeLog.h"

void ChangeLog::Init()
{
    NewVersion("B0.9.0", "25.11.30");
    AddContent(INFO, u8"发布第一支测试版本");

    NewVersion("B0.9.1", "25.12.05");
    AddContent(OPTIMIZE, u8"优化资源文件夹位置");
    AddContent(ADD, u8"添加窗口样式自定义");
    AddContent(ADD, u8"添加窗口彩虹字");
    AddContent(ADD, u8"添加动态模糊");

    NewVersion("B0.9.2", "25.12.09");
    AddContent(OPTIMIZE, u8"优化ChangeLog界面");
    AddContent(OPTIMIZE, u8"优化/美化动态模糊");
    AddContent(FIXBUG, u8"修复字体彩色绑定问题");
    AddContent(ADD, u8"添加菜单背景模糊");
    AddContent(ADD, u8"添加多配置管理系统");

    NewVersion("B0.9.3", "25.12.18");
    AddContent(OPTIMIZE, u8"动态模糊添加FPS调制");
    AddContent(ADD, u8"全新UI大更新");
    AddContent(ADD, u8"添加点击特效");
    AddContent(CHANGE, u8"更改默认字体为平滑字体");
    AddContent(CHANGE, u8"按键显示空格改为直线");
    AddContent(CHANGE, u8"改进架构,仅文本显示可多例");
    AddContent(ADD, u8"完美实现退出(detach)");

    NewVersion("B0.9.4", "25.12.20");
    AddContent(OPTIMIZE, u8"帧率高时detach不再崩端");
    AddContent(OPTIMIZE, u8"优化帧率，降低CPU性能开销");
    AddContent(ADD, u8"添加弹窗");
    AddContent(ADD, u8"新增UI音效");
    AddContent(CHANGE, u8"改进UI配色，现在更柔和");

    NewVersion("B0.9.5", "25.12.23");
    AddContent(OPTIMIZE, u8"兼容旧版mc(lwjgl2)");
    AddContent(OPTIMIZE, u8"解决菜单鼠标被困窗口内的问题");
    AddContent(FIXBUG, u8"修复强制疾跑卡键Bug");
    AddContent(FIXBUG, u8"修复帧率调制失效Bug");
    AddContent(ADD, u8"完善弹窗显示");

    NewVersion("V1.0.0", "25.12.26");
    AddContent(CHANGE, u8"菜单打开时会返回默认状态");
    AddContent(FIXBUG, u8"修复动态模糊遗留问题");
    AddContent(FIXBUG, u8"修复按键显示拖拽问题");
    AddContent(INFO, u8"发布第一支正式版本");

    NewVersion("V1.0.1", "26.01.02");
    AddContent(CHANGE, u8"动态模糊可在游戏暂停时关闭");
    AddContent(FIXBUG, u8"解决1.8全屏游戏状态出错的问题");

    NewVersion("V1.0.2", "26.01.05");
    AddContent(OPTIMIZE, u8"优化动态模糊的模糊度曲线");
    AddContent(ADD, u8"添加打开配置文件夹的按钮");
    AddContent(ADD, u8"实现窗口磁贴");
    AddContent(FIXBUG, u8"解决初始配置错乱的Bug");

    NewVersion("V1.0.3", "26.01.10");
    AddContent(ADD, u8"实现无边框全屏");
    AddContent(FIXBUG, u8"解决部分用户激活崩端问题");
    AddContent(FIXBUG, u8"修复动态模糊花屏问题");
    AddContent(FIXBUG, u8"修复强制疾跑误激活问题");

    NewVersion("V1.0.4", "26.01.24");
    AddContent(ADD, u8"添加AutoText");
    AddContent(ADD, u8"添加音乐信息显示");
    AddContent(ADD, u8"添加弹窗进度背景条，优化弹窗");
    AddContent(OPTIMIZE, u8"优化多处细节");
    AddContent(FIXBUG, u8"修复模组右侧按钮不同步的问题");
    AddContent(REMOVE, u8"去除窗口半透明，优化帧率");

    NewVersion("V1.0.5", "26.01.27");
    AddContent(ADD, u8"鼠标悬停在窗口上时添加固定/删除按钮");
    AddContent(ADD, u8"音乐显示现在有按钮可以暂停/播放、跳过");
    AddContent(ADD, u8"添加游戏按键绑定读取系统");
    AddContent(ADD, u8"添加自动保存");
    AddContent(FIXBUG, u8"解决打开菜单时游戏窗口无法调整/关闭的问题");
    AddContent(FIXBUG, u8"解决关闭MC时游戏进程关不干净的问题");
    AddContent(CHANGE, u8"现在按住Ctrl吸附边缘，按住Shift吸附其它窗口");
    AddContent(CHANGE, u8"打开菜单时会恢复居中，解决调整窗口大小后菜单消失偏移的问题");

} //我写这个真是个天才，这样就不用手敲Imgui代码了，天才天才天才天

ChangeLog::ChangeLog()
{
	Init();
}

void ChangeLog::Draw() const
{
    for (auto& log : logs)
    {
        DrawSingleLog(log);
    }
}

void ChangeLog::AddContent(const ContentType& type, const std::string& content)
{
    Content newContent;
    newContent.type = type;
    newContent.content = content;
    logs.front().contents.push_back(newContent);
}

void ChangeLog::NewVersion(const std::string& version, const std::string& date)
{
	Log newLog;
	newLog.info.version = version;
	newLog.info.date = date;
	logs.push_front(newLog);
}

void ChangeLog::DrawSingleLog(const Log& log)
{
    DrawVersion(log.info);
    for (auto& content : log.contents)
    {
        DrawContent(content);
    }
    ImGui::Separator();
}

void ChangeLog::DrawVersion(const Info& info)
{
    std::string text = "   -" + info.date + "-    |    -" + info.version + "-";
    ImGuiStd::TextShadowWrapped(text.c_str());
}

void ChangeLog::DrawContent(const Content& content)
{
    std::string prefix;
    std::string contentStr = content.content;
    //std::string finalStr;
    ImVec4 * prefixColor = nullptr;
    static ImVec4 COLOR_INFO = ImVec4(0.88f, 0.76f, 0.42f, 1.0f); // 金色（信息）
    static ImVec4 COLOR_OPTIMIZE = ImVec4(0.70f, 0.58f, 0.80f, 1.0f); // 紫色（优化）
    static ImVec4 COLOR_ADD = ImVec4(0.55f, 0.78f, 0.60f, 1.0f); // 绿色（新增） 
    static ImVec4 COLOR_FIXBUG = ImVec4(0.55f, 0.70f, 0.85f, 1.0f); // 蓝色（修复）
    static ImVec4 COLOR_REMOVE = ImVec4(0.88f, 0.52f, 0.52f, 1.0f); // 红色（移除）
    static ImVec4 COLOR_CHANGE = ImVec4(0.65f, 0.65f, 0.65f, 1.0f); // 灰色（变更）
    switch (content.type)
    {
    case INFO:
        prefix = u8"[信息]";
        prefixColor = &COLOR_INFO;
        break;
    case OPTIMIZE:
        prefix = u8"[优化]";
        prefixColor = &COLOR_OPTIMIZE;
        break;
    case ADD:
        prefix = u8"[新增]";
        prefixColor = &COLOR_ADD;
        break;
    case FIXBUG:
        prefix = u8"[修复]";
        prefixColor = &COLOR_FIXBUG;
        break;
    case REMOVE:
        prefix = u8"[移除]";
        prefixColor = &COLOR_REMOVE;
        break;
    case CHANGE:
        prefix = u8"[变更]";
        prefixColor = &COLOR_CHANGE;
        break;
    }

    //finalStr = prefix + " " + contentStr;
    ImGui::Bullet();
    ImGui::SameLine();
    ImGuiStd::TextColoredShadow(*prefixColor, prefix.c_str());
    ImGui::SameLine();
    ImGuiStd::TextShadowWrapped(contentStr.c_str());
}
