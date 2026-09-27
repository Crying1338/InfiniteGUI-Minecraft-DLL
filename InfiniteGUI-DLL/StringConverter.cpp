#include "StringConverter.h"
#include <windows.h>

namespace StringConverter
{
    // =============================
    // UTF-8 → UTF-16 (wstring)
    // =============================
    std::wstring Utf8ToWstring(const std::string& utf8)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";

        std::wstring wstr(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);


        // Remove the trailing null character
        if (!wstr.empty() && wstr[wstr.length() - 1] == L'\0')
        {
            wstr.resize(wstr.length() - 1);
        }

        return wstr;
    }

    // =============================
    // UTF-16 → UTF-8
    // =============================
    std::string WstringToUtf8(const std::wstring& wstr)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";

        std::string utf8(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);

        // Remove the trailing null character
        if (!utf8.empty() && utf8[utf8.length() - 1] == '\0')
        {
            utf8.resize(utf8.length() - 1);
        }

        return utf8;
    }

    // =============================
    // ACP → UTF-16
    // =============================
    std::wstring AcpToWstring(const std::string& acp)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, acp.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";

        std::wstring wstr(len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, acp.c_str(), -1, &wstr[0], len);
        return wstr;
    }

    // =============================
    // UTF-16 → ACP
    // =============================
    std::string WstringToAcp(const std::wstring& wstr)
    {
        int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";

        std::string acp(len, '\0');
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &acp[0], len, nullptr, nullptr);
        return acp;
    }

    // =============================
    // UTF-8 → ACP
    // =============================
    std::string Utf8ToAcp(const std::string& utf8)
    {
        return WstringToAcp(Utf8ToWstring(utf8));
    }

    // =============================
    // ACP → UTF-8
    // =============================
    std::string AcpToUtf8(const std::string& acp)
    {
        return WstringToUtf8(AcpToWstring(acp));
    }
}