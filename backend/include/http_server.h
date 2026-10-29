#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdbool.h>
#include <stddef.h>

#define HTTP_PORT 8080
#define HTTP_BUFFER_SIZE 16384

typedef struct {
    char method[16];        // "GET", "POST", "OPTIONS", "DELETE"
    char path[256];         // "/api/slots"
    char query[256];        // "category=CAR&status=AVAILABLE"
    char body[HTTP_BUFFER_SIZE];
    int body_length;
} HttpRequest;

typedef struct {
    int status_code;
    char content_type[64];
    char body[HTTP_BUFFER_SIZE];
    int body_length;
} HttpResponse;

// Handler function signature
typedef void (*RouteHandler)(const HttpRequest* req, HttpResponse* res);

int http_server_start(int port);
void http_server_stop(void);

// Response helpers
void http_response_json(HttpResponse* res, int status_code, const char* json_str);
void http_response_error(HttpResponse* res, int status_code, const char* error_msg);
void http_response_cors_preflight(HttpResponse* res);

// Query parser helper: extracts value of `key` from query string into `out_val`
bool http_query_get_param(const char* query, const char* key, char* out_val, size_t max_len);

// Simple JSON string/int extractor helpers
bool json_get_string(const char* json, const char* key, char* out_val, size_t max_len);
bool json_get_int(const char* json, const char* key, int* out_val);
bool json_get_double(const char* json, const char* key, double* out_val);

#endif // HTTP_SERVER_H
