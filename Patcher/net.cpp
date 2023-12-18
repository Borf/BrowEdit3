#include "net.h"
#include <sstream>

namespace net
{

std::vector<std::wstring> split(const std::wstring &source, wchar_t delimiter)
{
    std::vector<std::wstring> tokens;
    std::wstring token;
    std::wstringstream token_stream(source);
    while (std::getline(token_stream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

error::error(const std::wstring &msg) : runtime_error("Use wwhat"),
                                        msg(msg),
                                        error_code(0),
                                        error_msg(L"")
{
}

error::error(const std::wstring &msg, DWORD error_code) : runtime_error("Use wwhat"),
                                                          msg(msg),
                                                          error_code(error_code),
                                                          error_msg(format_message(error_code))
{
}

error::~error() noexcept
{
}

std::wstring error::wwhat() const
{
    if (!error_code)
        return msg;

    return msg + L" (" + std::to_wstring(error_code) + L") " + error_msg;
}

std::wstring error::format_message(DWORD err)
{
    //Get the error message, if any.
    if (err == 0)
        return std::wstring(); //No error message has been recorded

    LPTSTR message_buffer = nullptr;
    size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_FROM_SYSTEM |
                                    FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL,
                                err,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)&message_buffer,
                                0,
                                NULL);

    std::wstring message((wchar_t*)message_buffer, size);

    //Free the buffer.
    LocalFree(message_buffer);

    return message;
}

url::url(const std::wstring &url_text) : text(url_text)
{
    // The regex using RFC 3986 suggestion is
    // (^(([^:\/?#]+):)?(//([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)
    // We manage also the split of authority into host and port
    std::wregex url_regex(LR"(^(([^:\/?#]+):)?(//([^\/?#:]*)(:([^\/?#]*))?)?([^?#]*)(\?([^#]*))?(#(.*))?)",
                          std::wregex::extended);
    std::wsmatch url_match_result;

    if (std::regex_match(url_text, url_match_result, url_regex))
    {
        unsigned counter = 0;
        for (const auto &res : url_match_result)
        {
            switch (counter)
            {
            case 2:
                scheme = res;
                break;
            case 4:
                host = res;
                break;
            case 6:
                port = res;
                break;
            case 7:
                path = res;
                break;
            case 9:
                query = res;
                break;
            case 11:
                fragment = res;
                break;
            }
            counter++;
        }
    }
    else
    {
        throw error(L"Malformed url");
    }
}

url::~url()
{
}

session make_session(const std::wstring &user_agent)
{
    HINTERNET raw_handle = WinHttpOpen(user_agent.c_str(),
                                       WINHTTP_ACCESS_TYPE_NO_PROXY,
                                       WINHTTP_NO_PROXY_NAME,
                                       WINHTTP_NO_PROXY_BYPASS,
                                       0);
    if (!raw_handle)
    {
        throw error(L"Error (WinHttpOpen)", GetLastError());
    }

    return session(raw_handle);
}

bool named_proxy_policy::check_policy(const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config)
{
    return config.lpszProxy;
}

WINHTTP_PROXY_INFO named_proxy_policy::make_proxy_info(session &http_session, const url &dest_url, const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config)
{
    WINHTTP_PROXY_INFO proxy_info = {};
    proxy_info.lpszProxy = config.lpszProxy;
    proxy_info.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy_info.lpszProxyBypass = NULL;

    return proxy_info;
}

bool auto_config_url_policy::check_policy(const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config)
{
    return config.lpszAutoConfigUrl;
}

WINHTTP_PROXY_INFO auto_config_url_policy::make_proxy_info(session &http_session, const url &dest_url, const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config)
{
    WINHTTP_PROXY_INFO proxy_info = {};
    WINHTTP_PROXY_INFO proxy_info_tmp = {};
    WINHTTP_AUTOPROXY_OPTIONS opt_pac = {};

    // Script proxy pac
    opt_pac.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
    opt_pac.lpszAutoConfigUrl = config.lpszAutoConfigUrl;
    opt_pac.dwAutoDetectFlags = 0;
    opt_pac.fAutoLogonIfChallenged = TRUE;
    opt_pac.lpvReserved = 0;
    opt_pac.dwReserved = 0;

    if (WinHttpGetProxyForUrl(http_session,
                              dest_url.get_text().c_str(),
                              &opt_pac,
                              &proxy_info_tmp))
    {
        proxy_info = proxy_info_tmp;
    }
    else
    {
        throw error(L"Error (WinHttpGetProxyForUrl)", GetLastError());
    }

    return proxy_info;
}

bool auto_detect_policy::check_policy(const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config)
{
    return config.fAutoDetect;
}

WINHTTP_PROXY_INFO auto_detect_policy::make_proxy_info(session &http_session, const url &dest_url, const WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &config)
{
    WINHTTP_PROXY_INFO proxy_info = {};
    WINHTTP_PROXY_INFO proxy_info_tmp = {};
    WINHTTP_AUTOPROXY_OPTIONS opt_pac = {};

    // Autodetect proxy
    opt_pac.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
    opt_pac.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
    opt_pac.fAutoLogonIfChallenged = TRUE;
    opt_pac.lpszAutoConfigUrl = NULL;
    opt_pac.lpvReserved = 0;
    opt_pac.dwReserved = 0;

    if (WinHttpGetProxyForUrl(http_session,
                              dest_url.get_text().c_str(),
                              &opt_pac,
                              &proxy_info_tmp))
    {
        proxy_info = proxy_info_tmp;
        auto proxy_list = split(proxy_info_tmp.lpszProxy, L';');
        proxy_info.lpszProxy = const_cast<LPWSTR>(proxy_list[0].c_str());
        LOG(std::wstring(L"Proxy is: ") + proxy_info.lpszProxy);
    }
    else
    {
        throw error(L"Error (WinHttpGetProxyForUrl)", GetLastError());
    }

    return proxy_info;
}

void detect_proxy(session &http_session, const url &dest_url)
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG config = {};
    WINHTTP_PROXY_INFO proxy_info = {};
    DWORD options = SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                    SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                    SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                    SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

    if (WinHttpGetIEProxyConfigForCurrentUser(&config))
    {
        if (named_proxy_policy::check_policy(config))
        {
            LOG(L"named_proxy_policy");
            proxy_info = named_proxy_policy::make_proxy_info(http_session, dest_url, config);
        }
        if (auto_config_url_policy::check_policy(config))
        {
            LOG(L"auto_config_url_policy");
            proxy_info = auto_config_url_policy::make_proxy_info(http_session, dest_url, config);
        }
        if (auto_detect_policy::check_policy(config))
        {
            LOG(L"auto_detect_policy");
            try
            {
                proxy_info = auto_detect_policy::make_proxy_info(http_session, dest_url, config);
            }
            catch (error &e)
            {
                if (e.get_error_code() != ERROR_WINHTTP_AUTODETECTION_FAILED)
                {
                    throw;
                }
                LOG(L"ERROR_WINHTTP_AUTODETECTION_FAILED");
            }
        }
        if (proxy_info.lpszProxy)
        {
            LOG(L"set option WINHTTP_OPTION_PROXY");
            set_option(http_session, WINHTTP_OPTION_PROXY, &proxy_info);
            LOG(L"set option WINHTTP_OPTION_SECURITY_FLAGS");
            set_option(http_session, WINHTTP_OPTION_SECURITY_FLAGS, &options);
        }
    }
}

INTERNET_PORT get_port_from_url(const url &dest_url)
{
    if (!dest_url.get_port().empty())
    {
        // Retrieve port specified in the URL
        return (INTERNET_PORT)std::stoi(dest_url.get_port());
    }

    // Use default port for URL scheme
    if (dest_url.get_scheme() == L"http")
    {
        return INTERNET_DEFAULT_HTTP_PORT;
    }
    else if (dest_url.get_scheme() == L"https")
    {
        return INTERNET_DEFAULT_HTTPS_PORT;
    }

    throw error(L"Unknown scheme " + dest_url.get_text());
}

connection make_connection(session &http_session, const url &dest_url)
{
    HINTERNET raw_handle = NULL;
    if (!(raw_handle = WinHttpConnect(http_session,
                                      dest_url.get_host().c_str(),
                                      get_port_from_url(dest_url),
                                      0)))
    {
        throw error(L"Error (WinHttpConnect)", GetLastError());
    }

    return connection(raw_handle);
}

request make_request(connection &http_connection, const std::wstring &verb, const url &dest_url)
{
    HINTERNET raw_handle = NULL;
    DWORD flags = 0;

    if (dest_url.get_scheme() == L"https")
    {
        flags = WINHTTP_FLAG_SECURE;
    }
    if (!(raw_handle = WinHttpOpenRequest(http_connection,
                                          verb.c_str(),
                                          dest_url.get_path().c_str(),
                                          NULL,
                                          WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES,
                                          flags)))
    {
        throw error(L"Error (WinHttpOpenRequest)", GetLastError());
    }

    return request(raw_handle);
}

void send_request(request &http_request)
{
    std::wstring headers = L"Accept-Encoding: None\r\n";

    if (!WinHttpSendRequest(http_request,
                            headers.c_str(), headers.size(),
    //                        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0,
                            0,
                            0))
    {
        throw error(L"Error (WinHttpSendRequest)", GetLastError());
    }
}

void receive_response(request &http_request)
{
    if (!WinHttpReceiveResponse(http_request, NULL))
    {
        throw error(L"Error (WinHttpReceiveResponse)", GetLastError());
    }
}

size_t query_data_avaliable(request &http_request)
{
    DWORD size = 0;
    if (!WinHttpQueryDataAvailable(http_request, &size))
    {
        throw error(L"Error (WinHttpQueryDataAvailable)", GetLastError());
    }
    return size;
}

buffer_t read_data(request &http_request, size_t size)
{
    DWORD downloaded = 0;

    if (size == 0)
    {
        size = query_data_avaliable(http_request);
    }
    std::vector<char> buffer;
    buffer.resize(1024*8);
    if (!WinHttpReadData(http_request,
                         (LPVOID)&buffer[0],
                         1024*8,
                         &downloaded))
    {
        throw error(L"Error (WinHttpReadData)", GetLastError());
    }

    buffer.resize(downloaded);

    return buffer;
}

void set_credentials(request &http_request, DWORD target, DWORD auth_scheme, const std::wstring &username, const std::wstring &password)
{
    if (!WinHttpSetCredentials(http_request,
                               target,
                               auth_scheme,
                               username.c_str(),
                               password.c_str(),
                               NULL))
    {
        throw error(L"Error (WinHttpSetCredentials)", GetLastError());
    }
}

DWORD query_headers_status(request &http_request)
{
    DWORD status_code = 0;
    DWORD size = sizeof(DWORD);
    if (!WinHttpQueryHeaders(http_request,
                             WINHTTP_QUERY_STATUS_CODE |
                                 WINHTTP_QUERY_FLAG_NUMBER,
                             NULL,
                             &status_code,
                             &size,
                             NULL))
    {
        throw error(L"Error (WinHttpQueryHeaders)", GetLastError());
    }

    return status_code;
}

DWORD query_headers_content_length(request& http_request)
{
    DWORD status_code = 0;
    DWORD size = sizeof(DWORD);
    if (!WinHttpQueryHeaders(http_request,
        WINHTTP_QUERY_CONTENT_LENGTH |
        WINHTTP_QUERY_FLAG_NUMBER,
        NULL,
        &status_code,
        &size,
        NULL))
    {
        throw error(L"Error (WinHttpQueryHeaders)", GetLastError());
    }

    return status_code;
}

PWCHAR query_headers(request& http_request)
{
    DWORD dwSize = sizeof(DWORD);
    WinHttpQueryHeaders(http_request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
        WINHTTP_HEADER_NAME_BY_INDEX, NULL,
        &dwSize, WINHTTP_NO_HEADER_INDEX);

    // Allocate memory for the buffer.
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        PWCHAR lpOutBuffer = new WCHAR[dwSize / sizeof(WCHAR)];

        // Now, use WinHttpQueryHeaders to retrieve the header.
        bool bResults = WinHttpQueryHeaders(http_request,
            WINHTTP_QUERY_RAW_HEADERS_CRLF,
            WINHTTP_HEADER_NAME_BY_INDEX,
            lpOutBuffer, &dwSize,
            WINHTTP_NO_HEADER_INDEX);
        return lpOutBuffer;
    }
    return nullptr;
}


std::tuple<DWORD, DWORD, DWORD> query_auth_scheme(request &http_request)
{
    DWORD supported_schemes;
    DWORD first_scheme;
    DWORD target;
    if (!WinHttpQueryAuthSchemes(http_request,
                                 &supported_schemes,
                                 &first_scheme,
                                 &target))
    {
        throw error(L"Error (WinHttpQueryAuthSchemes)", GetLastError());
    }

    return std::make_tuple(supported_schemes, first_scheme, target);
}

DWORD select_auth_scheme(DWORD supported_schemes)
{
    if (supported_schemes & WINHTTP_AUTH_SCHEME_NEGOTIATE)
    {
        LOG(L"Using WINHTTP_AUTH_SCHEME_NEGOTIATE")
        return WINHTTP_AUTH_SCHEME_NEGOTIATE;
    }
    else if (supported_schemes & WINHTTP_AUTH_SCHEME_NTLM)
    {
        LOG(L"Using WINHTTP_AUTH_SCHEME_NTLM")
        return WINHTTP_AUTH_SCHEME_NTLM;
    }
    else if (supported_schemes & WINHTTP_AUTH_SCHEME_PASSPORT)
        return WINHTTP_AUTH_SCHEME_PASSPORT;
    else if (supported_schemes & WINHTTP_AUTH_SCHEME_DIGEST)
        return WINHTTP_AUTH_SCHEME_DIGEST;
    else
        return 0;
}

// Can not use wstring because the encoding is defined by the the remote server
buffer_t fetch_request(const url &dest_url, const std::wstring &username, const std::wstring &password, const std::wstring &user_agent, const std::function<void(float)>& callback)
{
    LOG(L"fetch_request " + dest_url.get_text());
    auto http_session = make_session(user_agent);
    detect_proxy(http_session, dest_url);
    auto http_connection = make_connection(http_session, dest_url);

    auto http_request = make_request(http_connection, L"GET", dest_url);

    // When no proxy authentication is required we can ony send and receive, like:
    // send_request(http_request);
    // receive_response(http_request);
    // When proxy authentication is required begin the proxy authentication challenge
    DWORD supported_schemes = 0;
    DWORD first_scheme = 0;
    DWORD target = 0;
    DWORD proxy_auth_scheme = 0;
    DWORD last_status = 0;
    bool done = false;
    while (!done)
    {
        if (proxy_auth_scheme != 0)
        {
            LOG(L"Sending proxy credentials");
            set_credentials(http_request, WINHTTP_AUTH_TARGET_PROXY, proxy_auth_scheme, username, password);
        }
        LOG(L"Sending request");
        send_request(http_request);
        receive_response(http_request);

        DWORD status_code = query_headers_status(http_request);
        switch (status_code)
        {
        case 200:
            LOG(L"200 The resource was successfully retrieved");
            done = true;
            break;
        case 401:
            LOG(L"401 The server requires authentication");
            std::tie(supported_schemes, first_scheme, target) = query_auth_scheme(http_request);
            if (auto selected_scheme = select_auth_scheme(supported_schemes) != 0)
            {
                LOG(L"Sending server credentials");
                set_credentials(http_request, target, selected_scheme, username, password);
            }
            else
            {
                done = true;
            }
            // TODO: handle multiple 401 response
            break;
        case 407:
            LOG(L"407 The proxy requires authentication");
            std::tie(supported_schemes, first_scheme, target) = query_auth_scheme(http_request);
            proxy_auth_scheme = select_auth_scheme(supported_schemes);
            // TODO: handle multiple 407 response
            if (last_status == 407)
            {
                LOG(L"Break for multiple 407 response");
                done = true;
            }
            break;
        default:
            LOG(L"Error: unexpected status code");
        }
        last_status = status_code;
    }

    PWCHAR headers = query_headers(http_request);

    int length = -1;
    try
    {
        length = query_headers_content_length(http_request);
    }
    catch (net::error e)
    {
        length = 100000000000;
    }

    buffer_t buffer;
    while (size_t size = query_data_avaliable(http_request) != 0)
    {
        auto chunk = read_data(http_request, size);
        buffer.insert(std::end(buffer), std::begin(chunk), std::end(chunk));
        if(callback)
            callback(buffer.size() / (float)length);
    }
    return buffer;
}

} // namespace net
