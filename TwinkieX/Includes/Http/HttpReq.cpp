#include "pch.h"
#include "HttpReq.h"
#pragma warning(disable : 4505)

static std::u8string FromStr(const std::string& that)
{
    return (char8_t*)that.c_str();
}

static std::string ToStr(const std::u8string& that)
{
    return (char*)that.c_str();
}

static std::wstring FromStr(const std::u8string& that)
{
    return std::filesystem::path(that).wstring();
}

static std::u8string ToStr(const std::wstring& that)
{
    return std::filesystem::path(that).u8string();
}

// Make an HTTP/HTTPS request and store the return data.
// This function is blocking. For a non-blocking method, use HttpRequest.
bool MakeRawHttpRequest(const std::u8string& Url, const std::u8string& AgentInfo, const std::u8string& Verb, const std::vector<uint8_t>& DataIn, std::vector<uint8_t>& DataOut, std::u8string& Err, const std::u8string& ContentType)
{
    std::wstring WUrl = FromStr(Url);

    wchar_t ExtraInfo[8192]{ 0 };
    wchar_t Path[1024]{ 0 };
    wchar_t Host[512]{ 0 };
    wchar_t Password[512]{ 0 };
    wchar_t Username[128]{ 0 };

#define DefComp(c, v) Components.lpsz##c = v; Components.dw##c##Length = sizeof(v)
    URL_COMPONENTS Components{};
    Components.dwStructSize = sizeof(URL_COMPONENTS);
    DefComp(HostName, Host);
    DefComp(UrlPath, Path);
    DefComp(Password, Password);
    DefComp(UserName, Username);
    DefComp(ExtraInfo, ExtraInfo);

    if (!WinHttpCrackUrl(WUrl.c_str(), (DWORD)WUrl.size(), 0, &Components))
    {
        Err = u8"Could not parse URL.";
        return false;
    }

    auto Session = WinHttpOpen(
        FromStr(AgentInfo).c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!Session || Session == INVALID_HANDLE_VALUE)
    {
        WinHttpCloseHandle(Session);
        Err = u8"WinHTTP failure while setting up session.";
        return false;
    }

    auto Connection = WinHttpConnect(Session, Host, Components.nPort, 0);
    if (!Connection || Connection == INVALID_HANDLE_VALUE)
    {
        WinHttpCloseHandle(Connection); WinHttpCloseHandle(Session);
        Err = u8"WinHTTP failure while setting up connection.";
        return false;
    }

    DWORD ReqFlags = (Components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;

    wchar_t AcceptTypes[128];
    if (!ContentType.empty())
        memcpy_s(
            AcceptTypes,
            sizeof(AcceptTypes),
            FromStr(ContentType).c_str(),
            FromStr(ContentType).size() * sizeof(wchar_t)
        );
    wchar_t* pAcceptTypes = AcceptTypes;


    auto Request = WinHttpOpenRequest(
        Connection,
        FromStr(Verb).c_str(),
        Path[0] ? Path : L"/",
        nullptr,
        WINHTTP_NO_REFERER,
        ContentType.empty() ? WINHTTP_DEFAULT_ACCEPT_TYPES : (LPCWSTR*)&pAcceptTypes,
        ReqFlags
    );
    size_t DataWrittenToRequest = 0;
    if (!DataIn.empty())
    {
        WinHttpWriteData(Request, DataIn.data(), (DWORD)(DataIn.size() * sizeof(uint8_t)), (LPDWORD)&DataWrittenToRequest);
        if (DataWrittenToRequest != DataIn.size() * sizeof(uint8_t))
        {
            WinHttpCloseHandle(Request); WinHttpCloseHandle(Connection); WinHttpCloseHandle(Session);
            Err = u8"Not all data was written into request.";
            return false;
        }
    }

    if (!WinHttpSendRequest(
        Request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0, 0, 0) ||
        !WinHttpReceiveResponse(Request, nullptr))
    {
        WinHttpCloseHandle(Request); WinHttpCloseHandle(Connection); WinHttpCloseHandle(Session);
        Err = u8"Sending HTTP request failed.";
        return false;
    }

    size_t DataAvailable = 0;
    size_t DataRead = 0;
    while (WinHttpQueryDataAvailable(Request, (LPDWORD)&DataAvailable) && DataAvailable)
    {
        size_t AlreadyWritten = DataOut.size();
        DataOut.resize(AlreadyWritten + DataAvailable);

        WinHttpReadData(
            Request,
            DataOut.data() + AlreadyWritten,
            (DWORD)DataAvailable,
            (LPDWORD)&DataRead
        );
        DataOut.resize(AlreadyWritten + DataRead);

    }
    WinHttpCloseHandle(Request); WinHttpCloseHandle(Connection); WinHttpCloseHandle(Session);

    return true;
#undef DefComp
}

DWORD WINAPI HttpThread(LPVOID lpParameter)
{
    HttpRequest* Req = (HttpRequest*)lpParameter;
    if (!Req || Req->Url.empty()) return FALSE;

    bool Result = MakeRawHttpRequest(
        Req->Url,
        Req->AgentInfo,
        Req->Verb,
        Req->DataIn,
        Req->DataOut,
        Req->LastErr,
        Req->ContentType
    );

    *(bool*)Req = true;

    return Result;
}