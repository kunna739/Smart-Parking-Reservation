#define _POSIX_C_SOURCE 200809L
#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>
#include <ws2tcpip.h>

extern void api_dispatch_request(const HttpRequest* req, HttpResponse* res);

static SOCKET g_listen_socket = INVALID_SOCKET;
static volatile bool g_running = false;

void http_response_json(HttpResponse* res, int status_code, const char* json_str) {
    if (!res) return;
    res->status_code = status_code;
    strncpy(res->content_type, "application/json", sizeof(res->content_type) - 1);
    if (json_str) {
        strncpy(res->body, json_str, sizeof(res->body) - 1);
        res->body[sizeof(res->body) - 1] = '\0';
        res->body_length = (int)strlen(res->body);
    } else {
        res->body[0] = '\0';
        res->body_length = 0;
    }
}

void http_response_error(HttpResponse* res, int status_code, const char* error_msg) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"success\":false,\"error\":\"%s\",\"code\":%d}",
             error_msg ? error_msg : "Unknown Error", status_code);
    http_response_json(res, status_code, buf);
}

void http_response_cors_preflight(HttpResponse* res) {
    if (!res) return;
    res->status_code = 204;
    res->content_type[0] = '\0';
    res->body[0] = '\0';
    res->body_length = 0;
}

bool http_query_get_param(const char* query, const char* key, char* out_val, size_t max_len) {
    if (!query || !key || !out_val || max_len == 0) return false;

    size_t key_len = strlen(key);
    const char* p = query;

    while (*p) {
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            p += key_len + 1;
            size_t idx = 0;
            while (*p && *p != '&' && idx < max_len - 1) {
                out_val[idx++] = *p++;
            }
            out_val[idx] = '\0';
            return true;
        }
        while (*p && *p != '&') p++;
        if (*p == '&') p++;
    }

    return false;
}

// Lightweight JSON parsing helpers
bool json_get_string(const char* json, const char* key, char* out_val, size_t max_len) {
    if (!json || !key || !out_val || max_len == 0) return false;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* pos = strstr(json, pattern);
    if (!pos) return false;

    pos += strlen(pattern);
    while (*pos && (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n' || *pos == ':')) {
        pos++;
    }

    if (*pos != '\"') return false;
    pos++; // skip open quote

    size_t idx = 0;
    while (*pos && *pos != '\"' && idx < max_len - 1) {
        if (*pos == '\\' && *(pos + 1)) pos++; // skip escape
        out_val[idx++] = *pos++;
    }
    out_val[idx] = '\0';
    return true;
}

bool json_get_int(const char* json, const char* key, int* out_val) {
    if (!json || !key || !out_val) return false;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* pos = strstr(json, pattern);
    if (!pos) return false;

    pos += strlen(pattern);
    while (*pos && (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n' || *pos == ':')) {
        pos++;
    }

    char num_buf[32];
    size_t idx = 0;
    if (*pos == '-') num_buf[idx++] = *pos++;
    while (isdigit((unsigned char)*pos) && idx < sizeof(num_buf) - 1) {
        num_buf[idx++] = *pos++;
    }
    num_buf[idx] = '\0';

    if (idx > 0) {
        *out_val = atoi(num_buf);
        return true;
    }
    return false;
}

bool json_get_double(const char* json, const char* key, double* out_val) {
    if (!json || !key || !out_val) return false;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* pos = strstr(json, pattern);
    if (!pos) return false;

    pos += strlen(pattern);
    while (*pos && (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n' || *pos == ':')) {
        pos++;
    }

    char num_buf[32];
    size_t idx = 0;
    if (*pos == '-') num_buf[idx++] = *pos++;
    while ((isdigit((unsigned char)*pos) || *pos == '.') && idx < sizeof(num_buf) - 1) {
        num_buf[idx++] = *pos++;
    }
    num_buf[idx] = '\0';

    if (idx > 0) {
        *out_val = atof(num_buf);
        return true;
    }
    return false;
}

static void parse_http_request(const char* raw, HttpRequest* req) {
    memset(req, 0, sizeof(HttpRequest));

    // 1. Method
    const char* p = raw;
    size_t m_idx = 0;
    while (*p && !isspace((unsigned char)*p) && m_idx < sizeof(req->method) - 1) {
        req->method[m_idx++] = *p++;
    }
    req->method[m_idx] = '\0';

    while (*p && isspace((unsigned char)*p)) p++;

    // 2. URL (Path + Query)
    char full_url[256] = {0};
    size_t u_idx = 0;
    while (*p && !isspace((unsigned char)*p) && u_idx < sizeof(full_url) - 1) {
        full_url[u_idx++] = *p++;
    }
    full_url[u_idx] = '\0';

    char* q_mark = strchr(full_url, '?');
    if (q_mark) {
        *q_mark = '\0';
        snprintf(req->path, sizeof(req->path), "%s", full_url);
        snprintf(req->query, sizeof(req->query), "%s", q_mark + 1);
    } else {
        snprintf(req->path, sizeof(req->path), "%s", full_url);
        req->query[0] = '\0';
    }

    // 3. Body (find \r\n\r\n)
    const char* body_start = strstr(raw, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        strncpy(req->body, body_start, sizeof(req->body) - 1);
        req->body_length = (int)strlen(req->body);
    } else {
        req->body[0] = '\0';
        req->body_length = 0;
    }
}

static void send_http_response(SOCKET client, const HttpResponse* res) {
    char header_buf[1024];
    const char* status_text = "OK";
    if (res->status_code == 201) status_text = "Created";
    else if (res->status_code == 204) status_text = "No Content";
    else if (res->status_code == 400) status_text = "Bad Request";
    else if (res->status_code == 404) status_text = "Not Found";
    else if (res->status_code == 500) status_text = "Internal Server Error";

    int header_len = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Connection: close\r\n\r\n",
        res->status_code, status_text,
        res->content_type[0] ? res->content_type : "application/json",
        res->body_length);

    send(client, header_buf, header_len, 0);
    if (res->body_length > 0) {
        send(client, res->body, res->body_length, 0);
    }
}

int http_server_start(int port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[HTTP Error] WSAStartup failed.\n");
        return -1;
    }

    g_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listen_socket == INVALID_SOCKET) {
        fprintf(stderr, "[HTTP Error] Socket creation failed.\n");
        WSACleanup();
        return -1;
    }

    // Reuse address
    BOOL opt = TRUE;
    setsockopt(g_listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((u_short)port);

    if (bind(g_listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[HTTP Error] Bind to port %d failed: %d\n", port, WSAGetLastError());
        closesocket(g_listen_socket);
        WSACleanup();
        return -1;
    }

    if (listen(g_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "[HTTP Error] Listen failed: %d\n", WSAGetLastError());
        closesocket(g_listen_socket);
        WSACleanup();
        return -1;
    }

    g_running = true;
    printf(">>> Smart Parking C11 HTTP Backend running at http://localhost:%d <<<\n", port);

    char recv_buf[HTTP_BUFFER_SIZE];

    while (g_running) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client = accept(g_listen_socket, (struct sockaddr*)&client_addr, &client_len);

        if (client == INVALID_SOCKET) {
            if (!g_running) break;
            continue;
        }

        int bytes_recv = recv(client, recv_buf, sizeof(recv_buf) - 1, 0);
        if (bytes_recv > 0) {
            recv_buf[bytes_recv] = '\0';

            // Handle HTTP 100-Continue if requested
            if (strstr(recv_buf, "100-continue") != NULL || strstr(recv_buf, "100-Continue") != NULL) {
                const char* cont_resp = "HTTP/1.1 100 Continue\r\n\r\n";
                send(client, cont_resp, (int)strlen(cont_resp), 0);
                int more = recv(client, recv_buf + bytes_recv, (int)sizeof(recv_buf) - 1 - bytes_recv, 0);
                if (more > 0) {
                    bytes_recv += more;
                    recv_buf[bytes_recv] = '\0';
                }
            }

            // Ensure complete body is received based on Content-Length
            char* cl_hdr = strstr(recv_buf, "Content-Length:");
            if (!cl_hdr) cl_hdr = strstr(recv_buf, "content-length:");
            if (cl_hdr) {
                int content_len = atoi(cl_hdr + 15);
                char* body_pos = strstr(recv_buf, "\r\n\r\n");
                if (body_pos) {
                    body_pos += 4;
                    int cur_body_len = bytes_recv - (int)(body_pos - recv_buf);
                    while (cur_body_len < content_len && bytes_recv < (int)sizeof(recv_buf) - 1) {
                        int more = recv(client, recv_buf + bytes_recv, (int)sizeof(recv_buf) - 1 - bytes_recv, 0);
                        if (more <= 0) break;
                        bytes_recv += more;
                        recv_buf[bytes_recv] = '\0';
                        cur_body_len += more;
                    }
                }
            }

            HttpRequest req;
            HttpResponse res;
            memset(&res, 0, sizeof(HttpResponse));

            parse_http_request(recv_buf, &req);

            if (strcmp(req.method, "OPTIONS") == 0) {
                http_response_cors_preflight(&res);
            } else {
                api_dispatch_request(&req, &res);
            }

            send_http_response(client, &res);
        }

        closesocket(client);
    }

    closesocket(g_listen_socket);
    WSACleanup();
    return 0;
}

void http_server_stop(void) {
    g_running = false;
    if (g_listen_socket != INVALID_SOCKET) {
        closesocket(g_listen_socket);
        g_listen_socket = INVALID_SOCKET;
    }
}
