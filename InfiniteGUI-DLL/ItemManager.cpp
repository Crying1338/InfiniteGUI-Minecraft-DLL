#include <Windows.h>
#include "ItemManager.h"
#include "TimeItem.h"
#include "FpsItem.h"
#include "DanmakuItem.h"
#include "KeystrokesItem.h"
#include "CPSItem.h"
#include "BilibiliFansItem.h"
#include "FileCountItem.h"
#include "CounterItem.h"
#include "TextItem.h"

#include "Sprint.h"

#include "Motionblur.h"
#include "ClickEffect.h"

#include "CPSDetector.h"
#include "GameStateDetector.h"
#include "GlobalWindowStyle.h"
#include "GameWindowTool.h"

#include "Menu.h"
#include "NotificationItem.h"
#include <algorithm>
#include <chrono>

#include "AutoText.h"
#include "MusicInfoItem.h"

#include "ArrayListModule.h"
#include "TargetHudItem.h"
#include "Log.hpp"

// ------------------------------------------------
ItemManager::ItemManager()
{
    Init();
}

void ItemManager::Init()
{
    // 注册默认 Singleton
    // 每个模块的构造函数单独隔离：任一模块构造抛异常（例如 WinRT / 资源初始化失败）
    // 只跳过该模块，不能让整个 ItemManager 单例构造失败（否则后续每次访问都会重新抛出异常）
    auto addSafe = [this](const char* label, auto factory)
    {
        try
        {
            AddItem(factory());
        }
        catch (const std::exception& ex)
        {
            InfGuiLogLimited("ItemCtor", (std::string(label) + " -> " + ex.what()).c_str());
        }
        catch (...)
        {
            InfGuiLogLimited("ItemCtor", (std::string(label) + " -> 未知异常").c_str());
        }
    };

    addSafe("菜单", []() -> Item* { return &Menu::Instance(); });
    addSafe("强制疾跑", []() -> Item* { return &Sprint::Instance(); });
    addSafe("自动消息", []() -> Item* { return &AutoText::Instance(); });
    addSafe("动态模糊", []() -> Item* { return &Motionblur::Instance(); });
    addSafe("点击特效", []() -> Item* { return &ClickEffect::Instance(); });
    addSafe("时间显示", []() -> Item* { return &TimeItem::Instance(); });
    addSafe("FPS显示", []() -> Item* { return &FpsItem::Instance(); });
    addSafe("B站弹幕显示", []() -> Item* { return &DanmakuItem::Instance(); });
    addSafe("按键显示", []() -> Item* { return &KeystrokesItem::Instance(); });
    addSafe("CPS显示", []() -> Item* { return &CPSItem::Instance(); });
    addSafe("粉丝数显示", []() -> Item* { return &BilibiliFansItem::Instance(); });
    addSafe("文本显示", []() -> Item* { return &TextItem::Instance(); });
    addSafe("文件数量显示", []() -> Item* { return &FileCountItem::Instance(); });
    addSafe("计数器", []() -> Item* { return &CounterItem::Instance(); });
    addSafe("音乐信息显示", []() -> Item* { return &MusicInfoItem::Instance(); });
    addSafe("ArrayList", []() -> Item* { return &ArrayListModule::Instance(); });
    addSafe("TargetHUD", []() -> Item* { return &TargetHudItem::Instance(); });
    addSafe("提示弹窗", []() -> Item* { return &NotificationItem::Instance(); });
    addSafe("全局窗口样式", []() -> Item* { return &GlobalWindowStyle::Instance(); });
    addSafe("游戏状态检测", []() -> Item* { return &GameStateDetector::Instance(); });
    addSafe("窗口工具", []() -> Item* { return &GameWindowTool::Instance(); });
    addSafe("CPS检测", []() -> Item* { return &CPSDetector::Instance(); });
}

// ------------------------------------------------
void ItemManager::AddItem(Item* item)
{
    Items.push_back(item);
}

// ------------------------------------------------
void ItemManager::UpdateAll() const
{
    for (auto item : Items)
    {
        if (!item->isEnabled) continue;
        // 单个模块抛异常不应该连带其它模块或整局游戏（异常穿透 JVM = 崩溃）
        try
        {
            if (auto upd = dynamic_cast<UpdateModule*>(item))
            {
                if (upd->ShouldUpdate())
                {
                    upd->Update();
                    upd->MarkUpdated();
                }
            }
        }
        catch (const std::exception& ex)
        {
            InfGuiLogLimited("Update", (item->name + " -> " + ex.what()).c_str());
        }
        catch (...)
        {
            InfGuiLogLimited("Update", (item->name + " -> 未知异常").c_str());
        }
    }
}

// ------------------------------------------------
void ItemManager::RenderAllGui() const
{
    bool isWindowNeedHide = false;
    if (GameStateDetector::Instance().IsNeedHide())
        isWindowNeedHide = true; // 隐藏所有窗口
    for (auto item : Items)
    {
        if (!item->isEnabled) continue;
        try
        {
            if (auto ren = dynamic_cast<RenderModule*>(item))
            {
                if(!ren->IsRenderGui()) continue;
                if (dynamic_cast<WindowModule*>(ren) && isWindowNeedHide)
                    continue;
                ren->RenderGui();
            }
        }
        catch (const std::exception& ex)
        {
            InfGuiLogLimited("RenderGui", (item->name + " -> " + ex.what()).c_str());
        }
        catch (...)
        {
            InfGuiLogLimited("RenderGui", (item->name + " -> 未知异常").c_str());
        }
    }
}

// ------------------------------------------------
void ItemManager::RenderAllBeforeGui() const
{
    for (auto item : Items)
    {
        if (!item->isEnabled) continue;
        try
        {
            if (auto ren = dynamic_cast<RenderModule*>(item))
            {
                if (!ren->IsRenderBeforeGui()) continue;
                ren->RenderBeforeGui();
            }
        }
        catch (const std::exception& ex)
        {
            InfGuiLogLimited("RenderBeforeGui", (item->name + " -> " + ex.what()).c_str());
        }
        catch (...) {}
    }
}

// ------------------------------------------------
void ItemManager::RenderAllAfterGui() const
{
    for (auto item : Items)
    {
        if (!item->isEnabled) continue;
        try
        {
            if (auto ren = dynamic_cast<RenderModule*>(item))
            {
                if (!ren->IsRenderAfterGui()) continue;
                ren->RenderAfterGui();
            }
        }
        catch (const std::exception& ex)
        {
            InfGuiLogLimited("RenderAfterGui", (item->name + " -> " + ex.what()).c_str());
        }
        catch (...) {}
    }
}

bool ItemManager::IsDirty() const
{
    bool isDirty = false;
    if (GameStateDetector::Instance().IsInGame())
        for (auto item : Items)
        {
            if (!item->isEnabled) continue;
            if (auto ren = dynamic_cast<RenderModule*>(item))
            {
                if (ren->IsAnimating()) //动画中
                {
                    isDirty = true;
                    break;
                }
                if (ren->IsContentDirty()) //内容变化
                {
                    ren->SetContentDirty(false);
                    isDirty = true;
                    break;
                }
            }
        }
    else isDirty = true;
    return isDirty;
}

// ------------------------------------------------
void ItemManager::ProcessKeyEvents(bool state, bool isRepeat, WPARAM key) const
{
    for (auto item : Items)
    {
        // 逐模块隔离：某个模块的按键回调抛异常（例如 std::map::at 抛 out_of_range），
        // 异常一旦穿透 WndProc -> GLFW -> JVM 就会导致游戏崩溃（hs_err 0xe06d7363）
        try
        {
            if (auto kbd = dynamic_cast<KeybindModule*>(item))
            {
                if (auto menu = dynamic_cast<Menu*>(kbd))
                    menu->OnKeyEvent(state, isRepeat, key);
                if (!item->isEnabled) continue;
                kbd->OnKeyEvent(state, isRepeat, key);
            }
        }
        catch (const std::exception& ex)
        {
            InfGuiLogLimited("ProcessKeyEvents", (item->name + " -> " + ex.what()).c_str());
        }
        catch (...)
        {
            InfGuiLogLimited("ProcessKeyEvents", (item->name + " -> 未知异常").c_str());
        }
    }
}

// ------------------------------------------------
// JSON Load / Save
// ------------------------------------------------
void ItemManager::Load(const nlohmann::json& j) const
{
    // ---- 加载Item ----
    if (j.contains("Items"))
    {
        for (auto& node : j["Items"])
        {
            try
            {
                std::string type = node["type"];
                for (auto item : Items)
                {
                    if (item->name == type)
                    {
                        item->Load(node);
                        break;
                    }
                }
            }
            catch (const std::exception& ex)
            {
                InfGuiLogLimited("ItemLoad", ex.what());
            }
            catch (...) {}
        }
    }
}

// ------------------------------------------------
void ItemManager::Save(nlohmann::json& j) const
{
    j["Items"] = nlohmann::json::array();
    for (auto item : Items)
    {
        // 单个模块序列化失败不影响整体保存（nlohmann::json 可能抛 type_error）
        try
        {
            nlohmann::json node;
            item->Save(node);
            j["Items"].push_back(node);
        }
        catch (const std::exception& ex)
        {
            InfGuiLogLimited("ItemSave", (item->name + " -> " + ex.what()).c_str());
        }
        catch (...) {}
    }

}

void ItemManager::Clear(bool resetSingletons) const
{
    // ---- 重置所有 Items ----
    if (resetSingletons)
    {
        for (auto* item : Items)
        {
            item->Reset();   //  要求 Item 提供 Reset() 或默认状态
        }
    }
}
