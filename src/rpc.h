#pragma once

#include <ArduinoJson.h>
#include <esp_http_server.h>

#include "deviceconfig.h"

typedef int (*RpcHandler)(JsonObject, JsonObject, DeviceConfig*);

struct RpcRoute {
    const char* method;
    RpcHandler handler;
};

esp_err_t rpc_post_handler(httpd_req_t*, DeviceConfig*);
int handleConfigGet(JsonObject, JsonObject, DeviceConfig*);
