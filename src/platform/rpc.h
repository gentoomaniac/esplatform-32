#pragma once

#include <ArduinoJson.h>
#include <esp_http_server.h>

#include "deviceconfig.h"

typedef int (*RpcHandler)(JsonObject, JsonObject, Config*);

struct RpcRoute {
    const char* method;
    RpcHandler handler;
};

esp_err_t rpc_handler(httpd_req_t*);
int handleConfigGet(JsonObject, JsonObject, Config*);
