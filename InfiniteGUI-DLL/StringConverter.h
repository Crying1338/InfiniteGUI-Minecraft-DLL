#pragma once
#include <string>

namespace StringConverter
{
    // UTF-8 <-> UTF-16 (wstring)
    std::wstring Utf8ToWstring(const std::string& utf8);
    std::string  WstringToUtf8(const std::wstring& wstr);

    // ACP(GBK等) <-> UTF-16 (wstring)
    std::wstring AcpToWstring(const std::string& acp);
    std::string  WstringToAcp(const std::wstring& wstr);

    // 直接封装：UTF-8 <-> ACP
    std::string  Utf8ToAcp(const std::string& utf8);
    std::string  AcpToUtf8(const std::string& acp);
}