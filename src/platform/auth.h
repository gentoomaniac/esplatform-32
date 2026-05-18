#pragma once

#include <esp_http_server.h>

#include "deviceconfig.h"

typedef esp_err_t (*AuthedHandler)(httpd_req_t*, Config*);

esp_err_t authed(httpd_req_t*, Config*, AuthedHandler);
