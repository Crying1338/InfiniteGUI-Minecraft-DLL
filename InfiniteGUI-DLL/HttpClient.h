#pragma once
#include <string>

class HttpClient {
public:
    static bool HttpGet(const std::wstring& url, std::string& outResponse);
};