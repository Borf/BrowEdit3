#ifndef NET_H
#define NET_H

#include <windows.h>
#include <winhttp.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <regex>

#include <iostream>
#define LOG(msg) std::wcout << (msg) << std::endl;

namespace net
{

std::vector<std::wstring> split(const std::wstring &source, wchar_t delimiter);

class error : public std::runtime_error
{
  public:
    error(const std::wstring &msg);
    error(const std::wstring &msg, DWORD error_code);
    virtual ~error() noexcept;

    DWORD get_error_code() const { return error_code; }

    std::wstring wwhat() const;

    static std::wstring format_message(DWORD err);

  protected:
    std::wstring msg;
    DWORD error_code;
    std::wstring error_msg;
};

class url
{
  public:
    url(const std::wstring &url_text);
    ~url();

    std::wstring get_text() const { return text; }
    std::wstring get_scheme() const { return scheme; }
    std::wstring get_host() const { return host; }
    std::wstring get_port() const { return port; }
    std::wstring get_path() const { return path; }
    std::wstring get_query() const { return query; }
    std::wstring get_fragment() const { return fragment; }

  protected:
    std::wstring text;
    std::wstring scheme;
    std::wstring host;
    std::wstring port;
    std::wstring path;
    std::wstring query;
    std::wstring fragment;
};

template <typename Tag>
class internet_handle
{
  public:
    internet_handle(HINTERNET raw_handle) : handle(raw_handle) {}
    ~internet_handle()
    {
        if (handle)
        {
            WinHttpCloseHandle(handle);
        }
    }

    internet_handle(internet_handle &&other) noexcept // move constructor
        : handle(std::exchange(other.handle, NULL))
    {
    }

    internet_handle &operator=(internet_handle &&other) noexcept // move assignment
    {
        std::swap(handle, other.handle);
        return *this;
    }

    operator HINTERNET() { return handle; }

  protected:
    HINTERNET handle;

    internet_handle(const internet_handle &other) {}            // copy constructor
    internet_handle &operator=(const internet_handle &other) {} // copy assignment
};

using session = internet_handle<struct SessionTag>;
using connection = internet_handle<struct ConnectionTag>;
using request = internet_handle<struct RequestTag>;
using buffer_t = std::vector<char>;

session make_session(const std::wstring &user_agent);

struct named_proxy_policy
{
    static bool check_policy(const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config);
    static WINHTTP_PROXY_INFO make_proxy_info(session &http_session, const url &dest_url, const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config);
};

struct auto_config_url_policy
{
    static bool check_policy(const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config);
    static WINHTTP_PROXY_INFO make_proxy_info(session &http_session, const url &dest_url, const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config);
};

struct auto_detect_policy
{
    static bool check_policy(const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config);
    static WINHTTP_PROXY_INFO make_proxy_info(session &http_session, const url &dest_url, const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config);
};

template <typename TObj, typename TBuff>
void set_option(TObj &http_obj, DWORD option, TBuff *buffer)
{
    if (!WinHttpSetOption(http_obj, option, buffer, sizeof(TBuff)))
    {
        throw error(L"Error (WinHttpSetOption)", GetLastError());
    }
}

void detect_proxy(session &http_session, const url &dest_url);

INTERNET_PORT get_port_from_url(const url &dest_url);

connection make_connection(session &http_session, const url &dest_url);

request make_request(connection &http_connection, const std::wstring &verb, const url &dest_url);

void send_request(request &http_request);

void receive_response(request &http_request);

size_t query_data_avaliable(request &http_request);

buffer_t read_data(request &http_request, size_t size = 0);

void set_credentials(request &http_request, DWORD target, DWORD auth_scheme, const std::wstring &username, const std::wstring &password);

DWORD query_headers_status(request &http_request);

std::tuple<DWORD, DWORD, DWORD> query_auth_scheme(request &http_request);

DWORD select_auth_scheme(DWORD supported_schemes);

// Can not use wstring because the encoding is defined by the the remote server
buffer_t fetch_request(const url &dest_url, const std::wstring &username = L"", const std::wstring &password = L"", const std::wstring &user_agent = L"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko)", const std::function<void(float)> &callback = nullptr);

} // namespace net

#endif