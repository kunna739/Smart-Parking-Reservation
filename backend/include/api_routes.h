#ifndef API_ROUTES_H
#define API_ROUTES_H

#include "http_server.h"

void api_dispatch_request(const HttpRequest* req, HttpResponse* res);

#endif // API_ROUTES_H
