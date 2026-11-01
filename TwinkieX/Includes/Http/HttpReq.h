
#pragma once
#include "pch.h"
#include "Version.h"

DWORD WINAPI HttpThread(LPVOID lpParameter);

struct HttpDownUpInfo
{
    size_t DownloadedBytes = 0;
    size_t BytesToBeDownloaded = 0;
    size_t UploadedBytes = 0;
    size_t BytesToBeUploaded = 0;
};

bool MakeRawHttpRequest(
    const std::u8string& Url,
    const std::u8string& AgentInfo,
    const std::u8string& Verb,
    const std::vector<uint8_t>& DataIn,
    std::vector<uint8_t>& DataOut,
    std::unordered_map<std::u8string, std::u8string>& ReqHeadersInOut,
    std::unordered_map<std::u8string, std::u8string>& ResponseHeadersOut,
    std::u8string& Err,
    HttpDownUpInfo& DownUpInfo,
    const std::u8string& ContentType = u8"",
    const std::u8string& SaveToFilePath = u8"",
    bool FollowRedirects = true
);

class HttpRequest
{
    bool Done = true;
    HttpDownUpInfo DownUpInfo;
    friend DWORD WINAPI HttpThread(LPVOID lpParameter);
public:
    std::u8string Url;
    // Don't change unless you know what you're doing!
    std::u8string AgentInfo = u8"TwinkieX/" TWINKIEX_VERSION_HTTP " / jailmanguy1[at]gmail.com";
    std::u8string Verb = u8"GET";
    std::vector<uint8_t> DataIn;
    std::vector<uint8_t> DataOut;
    std::unordered_map<std::u8string, std::u8string> ReqHeadersInOut;
    std::unordered_map<std::u8string, std::u8string> ResponseHeadersOut;
    std::u8string LastErr;
    std::u8string ContentType;
    std::u8string SaveToFilePath;
    bool FollowRedirects = true;

    HttpRequest(std::u8string Url) : Url(Url) {}
    HttpRequest() {}

    ~HttpRequest() {}

    bool IsDone()
    {
        return Done;
    }

    bool Begin(std::u8string pUrl = u8"")
    {
        Done = false;
        if (!DataOut.empty()) DataOut.resize(0);
        if (!pUrl.empty()) this->Url = pUrl;
        if (!CreateThread(NULL, 0, HttpThread, this, 0, NULL))
            return false;
        return true;
    }
};