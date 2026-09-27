#include "GlobalConfig.h"
#include "ConfigManager.h"
void GlobalConfig::Load(const nlohmann::json& j)
{
    if (j.contains("fontPath")) fontPath = j["fontPath"];
    if (j.contains("currentProfile")) currentProfile = j["currentProfile"];
    if (j.contains("enableOptimization")) enableOptimization = j["enableOptimization"];
    if (j.contains("autoSave")) autoSave = j["autoSave"];
}

void GlobalConfig::Save(nlohmann::json& j) const
{
    j["fontPath"] = fontPath;
    j["currentProfile"] = currentProfile;
    j["enableOptimization"] = enableOptimization;
    j["autoSave"] = autoSave;
}