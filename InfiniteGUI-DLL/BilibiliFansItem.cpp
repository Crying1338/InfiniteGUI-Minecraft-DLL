#include "BilibiliFansItem.h"
#include "HttpClient.h"
#include "ImGuiStd.h"
#include "ImGui\imgui_internal.h"
#include <nlohmann/json.hpp>
#include "AudioManager.h"
#include "HttpUpdateWorker.h"
#include "Anim.h"
#include "NotificationItem.h"

void BilibiliFansItem::Toggle()
{
}

void BilibiliFansItem::Update()
{

    // 开后台线程获取
    std::thread([this]()
        {
            std::wstring url = L"https://api.bilibili.com/x/relation/stat?vmid=" + std::to_wstring(uid);
            std::string response;
            bool ok = HttpClient::HttpGet(url, response);
            if (ok)
            {
                try {
                    auto j = nlohmann::json::parse(response);
                    pendingFans = j["data"]["follower"].get<int>();
                }
                catch (...) {
                    pendingFans = -1;
                }
            }

        }).detach();

    int newFans = pendingFans.load(); 

    if (newFans < 0)
        return;

    fansCount = newFans;

    // 内容发生变化
    dirtyState.contentDirty = false;

    if(!firstLoad)
    {
        if (fansCount > lastFansCount)
        {
            color.color = ImVec4(0.1f, 1.0f, 0.1f, 1.0f); //绿色
            if (isPlaySound)
                AudioManager::Instance().playSound("bilibilifans\\bilibilifans_up.wav", soundVolume);
            int count = fansCount - lastFansCount;
            std::string msg = u8"涨粉 " + std::to_string(count) + u8" 位。";
            NotificationItem::Instance().AddNotification(NotificationType_Info, msg);
        }
        else if (fansCount < lastFansCount)
        {
            color.color = ImVec4(1.0f, 0.1f, 0.1f, 1.0f); //红色
            if (isPlaySound) AudioManager::Instance().playSound("bilibilifans\\bilibilifans_down.wav", soundVolume);
            int count = lastFansCount - fansCount;
            std::string msg = u8"掉粉 " + std::to_string(count) + u8" 位。";
            NotificationItem::Instance().AddNotification(NotificationType_Info, msg);
        }
        else return;
    }
    else 
        firstLoad = false;
    dirtyState.contentDirty = true;
    dirtyState.animating = true;
    lastFansCount = fansCount;

}

void BilibiliFansItem::HoverSetting()
{
}

void BilibiliFansItem::DrawContent()
{
    if (closed)
    {
        isEnabled = false;
        closed = false;
    }
    ImVec4 targetTextColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    //获取io
    ImGuiIO& io = ImGui::GetIO();

    float speed = 3.0f * std::clamp(io.DeltaTime, 0.0f, 0.05f);
    color.color = ImLerp(color.color, targetTextColor, speed);

    // 判断动画是否结束
    if (Anim::AlmostEqual(color.color, targetTextColor))
    {
        color.color = targetTextColor;
        dirtyState.animating = false;
    }

    ImGuiStd::TextColoredShadow(color.color, (prefix + std::to_string(fansCount) + suffix).c_str());
}

void BilibiliFansItem::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{

    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(bigItemWidth);

    static std::string uidStr = std::to_string(uid);
    ImGuiStd::InputTextStd(u8"B站 UID", uidStr);
    ImGui::SameLine();
    if (ImGui::Button(u8"确定"))
    {
        if (uidStr.empty())
        {
            uidStr = u8"不能输入空值"; // 默认设置为你的 UID
        }
        else if (uidStr.find_first_not_of("0123456789") != std::string::npos)
        {
            uidStr = u8"只能输入数字"; // 输入非数字
        }
        else if (std::stoll(uidStr) <= 0)  //数字小于等于0
        {
            uidStr = u8"只能输入正整数"; // 输入负数
        }
        else
        {
            uid = std::stoll(uidStr);
        }
    }
    DrawAffixSettings(bigPadding, centerX, itemWidth);
    DrawSoundSettings(bigPadding, centerX, itemWidth);
    DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void BilibiliFansItem::Load(const nlohmann::json& j)
{
    LoadItem(j);
    LoadAffix(j);
    LoadWindow(j);
    LoadSound(j);
    if (j.contains("uid")) uid = j["uid"];
    if (j.contains("fansCount")) fansCount = j["fansCount"];
    lastFansCount = fansCount;
    //if(isEnabled) HttpAddTask();
}

void BilibiliFansItem::Save(nlohmann::json& j) const
{
    SaveItem(j);
    SaveAffix(j);
    SaveWindow(j);
    SaveSound(j);

    j["uid"] = uid;
    j["fansCount"] = fansCount;
}