#pragma once

#include <esp_http_server.h>

#include "deviceconfig.h"

esp_err_t metrics_get_handler(httpd_req_t*, Config*);
