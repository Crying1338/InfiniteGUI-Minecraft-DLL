#pragma once
#include <vector>
#include <memory>
#include <minwindef.h>
#include <nlohmann/json.hpp>
#include "Item.h"

class ItemManager {
public:
    ItemManager();
    void Init();

    static ItemManager& Instance() {
        static ItemManager instance;
        return instance;
    }

    void AddItem(Item* item);
    void UpdateAll() const;

    void RenderAllGui() const;
    void RenderAllBeforeGui() const;
    void RenderAllAfterGui() const;
    bool IsDirty() const;
    void ProcessKeyEvents(bool state, bool isRepeat, WPARAM key) const;

    void Clear(bool resetSingletons) const;
    void Load(const nlohmann::json& j) const;
    void Save(nlohmann::json& j) const;

    const std::vector<Item*>& GetItems() const { return Items; }

private:
    std::vector<Item*> Items;                      // 全部 item（指针，不析构）

};