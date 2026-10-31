
#pragma once
#include "pch.h"
#include "Version.h"

DWORD WINAPI HttpThread(LPVOID lpParameter);
bool MakeRawHttpRequest(const std::u8string& Url, const std::u8string& AgentInfo, const std::u8string& Verb, const std::vector<uint8_t>& DataIn, std::vector<uint8_t>& DataOut, std::u8string& Err, const std::u8string& ContentType = u8"");

class HttpRequest
{
    // DO NOT CHANGE LOCATION
    // If you do, then update HttpThread accordingly.
    bool Done = true;
    uint8_t PaddingA[3];
    uint16_t PaddingB = 0;
    uint16_t PaddingC = 0;
public:
    std::u8string Url;
    std::vector<uint8_t> DataOut;
    std::vector<uint8_t> DataIn;
    std::u8string LastErr;
    // Don't change unless you know what you're doing!
    // 
    std::u8string AgentInfo = u8"TwinkieX/" TWINKIEX_VERSION_HTTP " / jailmanguy1[at]gmail.com";
    std::u8string Verb = u8"GET";
    std::u8string ContentType;

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