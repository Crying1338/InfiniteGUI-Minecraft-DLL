#include "HttpClient.h"
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

bool HttpClient::HttpGet(const std::wstring& url, std::string& outResponse)
{
    std::wstring server, path;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;

    // 解析URL（只支持 https://domain/path）
    if (url.rfind(L"https://", 0) != 0)
        return false;

    auto noProto = url.substr(8);
    size_t slash = noProto.find(L"/");
    server = noProto.substr(0, slash);
    path = L"/" + noProto.substr(slash + 1);

    HINTERNET hSession = WinHttpOpen(L"ImInfo-Agent",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        NULL, NULL, 0);

    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, server.c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    BOOL bResults = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0,
        0, 0);

    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);

    if (bResults) {
        DWORD dwSize = 0;
        do {
            DWORD dwDownloaded = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                break;

            if (dwSize == 0)
                break;

            std::string buffer;
            buffer.resize(dwSize);

            if (!WinHttpReadData(hRequest, &buffer[0], dwSize, &dwDownloaded))
                break;

            outResponse.append(buffer, 0, dwDownloaded);

        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return bResults == TRUE;
}