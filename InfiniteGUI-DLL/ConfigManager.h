#pragma once
#include <string>
#include <vector>

class ConfigManager
{
public:
    static ConfigManager& Instance()
    {
        static ConfigManager instance;
        return instance;
    }

    void Init();

    bool CreateProfile(const std::string& name);
    bool DeleteProfile(const std::string& name);
    bool RenameProfile(const std::string& oldName, const std::string& newName);

    bool SwitchProfile(const std::string& name, bool saveCurrent = true);

    bool Save();
    bool Load();
    bool SaveGlobal() const;
    bool LoadGlobal();
    bool SaveProfile() const;
    bool LoadProfile();

    const std::vector<std::string>& GetProfiles() const { return profiles; }
    const std::string& GetCurrentProfile() const { return currentProfile; }

private:
    ConfigManager() = default;
    static void NotifyProfileLoad();
    void RefreshProfileList();
    std::string GetProfilePath(const std::string& name) const;

private:
    std::vector<std::string> profiles;
    std::string currentProfile;
};