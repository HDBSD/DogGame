/* Preview-only Auth0/Discord authorization. This translation unit and its
 * startup call are excluded entirely when PREVIEW_BUILD_LOCKING is OFF. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <stdio.h>
#include <time.h>
#include "preview_auth.h"
#include "api/preview_auth_json.h"

#include "api/preview_auth_config.h"
#include "api/preview_auth_identity.h"
static const char *s_whitelist_error = "You need to be whitelisted to run this preview build.\n\nAsk for "
                                       "access to the preview tester role in the Discord server.";

static int auth_encode_url_value(const char *src, char *dst, size_t cap)
{
    const char *hex = "0123456789ABCDEF";
    size_t n = 0;
    for (; *src; src++)
    {
        unsigned char c = (unsigned char)*src;
        if (isalnum(c) || strchr("-_.~", c))
        {
            if (n + 1 >= cap)
            {
                return 0;
            }
            dst[n++] = c;
        }
        else
        {
            if (n + 3 >= cap)
            {
                return 0;
            }
            dst[n++] = '%';
            dst[n++] = hex[c >> 4];
            dst[n++] = hex[c & 15];
        }
    }
    dst[n] = 0;
    return 1;
}

static int auth_read_query_value(const char *query, const char *name, char *value, size_t cap)
{
    int found = 0;
    size_t name_length = strlen(name);
    while (*query)
    {
        const char *end = strchr(query, '&');
        if (!end)
        {
            end = query + strlen(query);
        }
        const char *equal = memchr(query, '=', (size_t)(end - query));
        if (equal && (size_t)(equal - query) == name_length && !memcmp(query, name, name_length))
        {
            if (found)
            {
                return 0;
            }
            found = 1;
            size_t n = 0;
            for (const char *p = equal + 1; p < end; p++)
            {
                unsigned char c = (unsigned char)*p;
                if (c == '%')
                {
                    if (end - p < 3)
                    {
                        return 0;
                    }
                    int a = json_hex_digit(p[1]), b = json_hex_digit(p[2]);
                    if (a < 0 || b < 0)
                    {
                        return 0;
                    }
                    c = (unsigned char)(a * 16 + b);
                    p += 2;
                }
                else if (c == '+')
                {
                    c = ' ';
                }
                if (!c || c == '\r' || c == '\n' || n + 1 >= cap)
                {
                    return 0;
                }
                value[n++] = c;
            }
            value[n] = 0;
        }
        query = *end ? end + 1 : end;
    }
    return found;
}

/* 0 ignore unrelated/malformed requests, 1 valid code, -1 Auth0 denial. */
static int auth_parse_callback(char *request, const char *state, char code[2048])
{
    if (strncmp(request, "GET /callback?", 14))
    {
        return 0;
    }
    char *query = request + 14, *space = strchr(query, ' ');
    if (!space)
    {
        return 0;
    }
    *space = 0;
    char returned[128], error[256];
    if (!auth_read_query_value(query, "state", returned, sizeof returned) || strcmp(returned, state))
    {
        return 0;
    }
    if (auth_read_query_value(query, "error", error, sizeof error) && error[0])
    {
        return -1;
    }
    return auth_read_query_value(query, "code", code, 2048) && code[0] ? 1 : 0;
}

static void auth_reply_to_browser(SOCKET client, int success)
{
    const char *body =
        success ? "<html><body>Login received. You can close this tab and return to the game.</body></html>"
                : "<html><body>This login callback was not accepted. Return to the original login "
                  "tab.</body></html>";
    char reply[512];
    int n = snprintf(reply, sizeof reply,
                     "HTTP/1.1 %s\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: "
                     "%u\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n%s",
                     success ? "200 OK" : "400 Bad Request", (unsigned)strlen(body), body);
    if (n > 0 && n < sizeof reply)
    {
        send(client, reply, n, 0);
    }
}

static int auth_wait_for_callback(SOCKET listener, const char *state, char code[2048])
{
    ULONGLONG deadline = GetTickCount64() + 300000;
    while (GetTickCount64() < deadline)
    {
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET(listener, &readable);
        struct timeval timeout = {1, 0};
        int ready = select(0, &readable, NULL, NULL, &timeout);
        if (ready < 0)
        {
            return 0;
        }
        if (!ready)
        {
            continue;
        }
        SOCKET client = accept(listener, NULL, NULL);
        if (client == INVALID_SOCKET)
        {
            continue;
        }
        DWORD receive_timeout = 3000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receive_timeout, sizeof receive_timeout);
        char request[8192];
        size_t n = 0;
        int complete = 0;
        ULONGLONG request_deadline = GetTickCount64() + 3000;
        while (n + 1 < sizeof request)
        {
            int received = recv(client, request + n, (int)(sizeof request - n - 1), 0);
            if (received <= 0)
            {
                break;
            }
            n += received;
            request[n] = 0;
            if (strstr(request, "\r\n\r\n"))
            {
                complete = 1;
                break;
            }
            if (GetTickCount64() >= request_deadline)
            {
                break;
            }
        }
        int result = complete ? auth_parse_callback(request, state, code) : 0;
        auth_reply_to_browser(client, result == 1);
        closesocket(client);
        if (result)
        {
            return result;
        }
    }
    return 0;
}

static char *auth_http_request(const wchar_t *path, const char *post)
{
    HINTERNET session = NULL, connection = NULL, request = NULL;
    char *body = NULL;
    size_t n = 0;
    int ok = 0;
    session = WinHttpOpen(L"DogGame Preview Authentication/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
    {
        goto done;
    }
    WinHttpSetTimeouts(session, 15000, 15000, 15000, 15000);
    connection = WinHttpConnect(session, L"hdbsd.uk.auth0.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection)
    {
        goto done;
    }
    request = WinHttpOpenRequest(connection, post ? L"POST" : L"GET", path, NULL, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request)
    {
        goto done;
    }
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof redirects))
    {
        goto done;
    }
    if (!WinHttpSendRequest(request,
                            post ? L"Content-Type: application/x-www-form-urlencoded\r\n"
                                 : WINHTTP_NO_ADDITIONAL_HEADERS,
                            post ? (DWORD)-1 : 0, (LPVOID)post, post ? (DWORD)strlen(post) : 0,
                            post ? (DWORD)strlen(post) : 0, 0) ||
        !WinHttpReceiveResponse(request, NULL))
    {
        goto done;
    }
    DWORD status = 0, size = sizeof status;
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) ||
        status != 200)
    {
        goto done;
    }
    body = (char *)malloc(PA_LIMIT + 1);
    if (!body)
    {
        goto done;
    }
    for (;;)
    {
        DWORD available = 0, read = 0;
        if (!WinHttpQueryDataAvailable(request, &available))
        {
            goto done;
        }
        if (!available)
        {
            break;
        }
        if (available > PA_LIMIT - n)
        {
            goto done;
        }
        if (!WinHttpReadData(request, body + n, available, &read) || !read)
        {
            goto done;
        }
        n += read;
    }
    if (memchr(body, 0, n))
    {
        goto done;
    }
    body[n] = 0;
    ok = 1;
done:
    if (request)
    {
        WinHttpCloseHandle(request);
    }
    if (connection)
    {
        WinHttpCloseHandle(connection);
    }
    if (session)
    {
        WinHttpCloseHandle(session);
    }
    if (!ok)
    {
        free(body);
        body = NULL;
    }
    return body;
}

int preview_authenticate(void)
{
    WSADATA wsa;
    SOCKET listener = INVALID_SOCKET;
    int sockets = 0, allowed = 0;
    char *tokens = NULL, *jwks = NULL;
    const char *error = "Preview authentication could not be completed. Please try again.";
    char state[44], verifier[44], nonce[44], challenge[44];
    unsigned char digest[32];
    if (!auth_random_value(state) || !auth_random_value(verifier) || !auth_random_value(nonce) ||
        !auth_sha256(verifier, strlen(verifier), digest) ||
        !base64url_encode(digest, sizeof digest, challenge, sizeof challenge))
    {
        goto done;
    }
    if (WSAStartup(MAKEWORD(2, 2), &wsa))
    {
        goto done;
    }
    sockets = 1;
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
    {
        goto done;
    }
    BOOL exclusive = TRUE;
    if (setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&exclusive, sizeof exclusive))
    {
        goto done;
    }
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(8765);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listener, (struct sockaddr *)&address, sizeof address) || listen(listener, 4))
    {
        error = "Preview login could not open localhost port 8765. Close another login attempt or "
                "application using that port, then try again.";
        goto done;
    }
    char redirect[256], authorize[2048], code[2048], encoded_code[6144], post[8192];
    if (!auth_encode_url_value(PA_REDIRECT, redirect, sizeof redirect))
    {
        goto done;
    }
    snprintf(authorize, sizeof authorize,
             "https://" PA_DOMAIN "/authorize?response_type=code&client_id=" PA_CLIENT
             "&redirect_uri=%s&scope=openid%%20profile%%20email&connection=discord&state=%s&nonce=%s&code_"
             "challenge=%s&code_challenge_method=S256&prompt=login",
             redirect, state, nonce, challenge);
    if ((INT_PTR)ShellExecuteA(NULL, "open", authorize, NULL, NULL, SW_SHOWNORMAL) <= 32)
    {
        error = "Could not open your browser for Discord login.";
        goto done;
    }
    int callback = auth_wait_for_callback(listener, state, code);
    closesocket(listener);
    listener = INVALID_SOCKET;
    if (callback != 1)
    {
        error =
            callback < 0
                ? "Discord login was denied or cancelled. This preview cannot start without authorization."
                : "Preview login timed out or did not return a valid callback. Please launch the game and "
                  "try again.";
        goto done;
    }
    if (!auth_encode_url_value(code, encoded_code, sizeof encoded_code))
    {
        goto done;
    }
    snprintf(post, sizeof post,
             "grant_type=authorization_code&client_id=" PA_CLIENT "&code=%s&redirect_uri=%s&code_verifier=%s",
             encoded_code, redirect, verifier);
    tokens = auth_http_request(L"/oauth/token", post);
    SecureZeroMemory(post, sizeof post);
    SecureZeroMemory(verifier, sizeof verifier);
    if (!tokens)
    {
        goto done;
    }
    JsonParser response;
    char jwt[32768];
    if (!json_parse(&response, tokens) || response.tokens[0].type != JSON_OBJECT ||
        !json_read_string(&response, json_find_member(&response, 0, "id_token"), jwt, sizeof jwt))
    {
        goto done;
    }
    jwks = auth_http_request(L"/.well-known/jwks.json", NULL);
    if (!jwks)
    {
        error =
            "Could not verify the preview login signing key. Check your internet connection and try again.";
        goto done;
    }
    int verified = auth_verify_identity_token(jwt, nonce, jwks, (int64_t)time(NULL));
    SecureZeroMemory(jwt, sizeof jwt);
    if (verified < 0)
    {
        error = s_whitelist_error;
        goto done;
    }
    if (!verified)
    {
        error = "Preview login returned an invalid or expired identity token. Please try signing in again.";
        goto done;
    }
    allowed = 1;
done:
    if (listener != INVALID_SOCKET)
    {
        closesocket(listener);
    }
    if (sockets)
    {
        WSACleanup();
    }
    if (tokens)
    {
        SecureZeroMemory(tokens, strlen(tokens));
        free(tokens);
    }
    free(jwks);
    SecureZeroMemory(verifier, sizeof verifier);
    if (!allowed)
    {
        fprintf(stderr, "[PREVIEW] %s\n", error);
        MessageBoxA(NULL, error, "DogGame preview access", MB_OK | MB_ICONERROR);
    }
    return allowed;
}
