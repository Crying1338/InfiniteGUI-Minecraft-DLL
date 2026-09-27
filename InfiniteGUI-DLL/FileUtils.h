#pragma once

#include <string>
#include <shlobj.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
inline bool DirectoryExists(const std::string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) &&
        (attr & FILE_ATTRIBUTE_DIRECTORY);
}

namespace FileUtils {

    inline std::string appDataPath;

    inline std::string configPath;

    inline std::string modulePath;

    inline std::string gamePath;

    inline std::string optionsPath;

    inline std::string soundPath;

    inline std::string GetAppDataPath()
    {
        PWSTR path = nullptr;
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path);

        char buffer[MAX_PATH];
        wcstombs(buffer, path, MAX_PATH);
        CoTaskMemFree(path);


        std::string p(buffer);

        if (!DirectoryExists(p))
        {
            return "";
        } //说明这个AppDataPath有问题

        if (p == "C:\\Users\\")
            return ""; //C:Users文件夹需要管理员权限才能动，不能作为AppDataPath

        if (!p.empty() && p.back() == '\\')
            p.pop_back(); //如果p中最后一个字符是\，则去掉，否则加上\InfiniteGUI

        p += "\\InfiniteGUI";

        if (!DirectoryExists(p))
        {
            CreateDirectoryA(p.c_str(), NULL);
        }

        return p;
    }

    inline std::string GetConfigPath()
    {
        return appDataPath + "\\Configs";
    }

    inline std::string GetGameRunDir()
    {
        char buffer[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buffer);
        return std::string(buffer);
    }

    inline std::string GetSoundPath()
    {
        return modulePath + "\\Assets\\Sounds";
    }

    inline std::string GetSoundPath(std::string soundName)
    {
        return soundPath + "\\" + soundName;
    }

    inline std::string GetModulePath(HMODULE hMod)
    {
        char path[MAX_PATH];
        GetModuleFileNameA(hMod, path, MAX_PATH);

        // 将 DLL 名字去掉，只保留路径
        std::string fullPath(path);
        size_t pos = fullPath.find_last_of("\\/");
        if (pos != std::string::npos)
            fullPath = fullPath.substr(0, pos);

        return fullPath;
    }

    inline void InitBasePath(std::string basePath)
    {
        PathRemoveFileSpecA(basePath.data());
    }

    inline void InitPaths(HMODULE hMod)
    {
        modulePath = GetModulePath(hMod);
        gamePath = GetGameRunDir();
        optionsPath = gamePath + "\\options.txt";
        soundPath = GetSoundPath();
        appDataPath = GetAppDataPath();
        if(appDataPath.empty()) // 如果获取失败，则使用默认路径（可能是多用户操作系统）
            appDataPath = "C:\\InfiniteGUI";

        configPath = GetConfigPath();
        InitBasePath(modulePath);
    }

};