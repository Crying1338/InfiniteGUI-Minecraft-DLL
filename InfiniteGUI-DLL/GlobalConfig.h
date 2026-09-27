#pragma once
#include <nlohmann/json.hpp>
#include <Windows.h>

class GlobalConfig {
public:
    std::string fontPath = "default";  // 全局语言

    std::string currentProfile = "profile";  // 全局语言

    // 在这里加更多全局参数…
    bool enableOptimization = true;

    bool autoSave = true;

    static GlobalConfig& Instance() {
        static GlobalConfig instance;
        return instance;
    }

    void Load(const nlohmann::json& j);
    void Save(nlohmann::json& j) const;
};