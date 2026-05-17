#include "rpc.h"

#include <ArduinoJson.h>
#include <esp_http_server.h>

#include "deviceconfig.h"

#define DEBUG

const size_t MAX_PAYLOAD_SIZE = 1024;
const char* JSON_RPC_VERSION = "2.0";

int handleConfigGet(JsonObject params, JsonObject result, Config* config) {
    serializeConfig(*config, result);
    return 0;
}

int handleConfigSet(JsonObject params, JsonObject result, Config* config) {
    if (params["config"].is<JsonObjectConst>()) {
        Serial.println("updating live config ...");
        deserializeConfig(*config, params["config"]);

        Serial.println("persisting config change ...");
        saveConfig(config);
        Serial.println("config written");
        return 0;
    }

    return 400;
}

int handleSystemReboot(JsonObject params, JsonObject result, Config* config) {
    xTaskCreate(
        [](void* pd) {
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
            vTaskDelete(NULL);
        },
        "reboot_task", 2048, NULL, 1, NULL);
    return 0;
}

int handleSystemReset(JsonObject params, JsonObject result, Config* config) {
    xTaskCreate(
        [](void* pd) {
            vTaskDelay(pdMS_TO_TICKS(500));
            Serial.println("Resetting config...");
            resetConfig();

            Serial.println("Rebooting...");
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();

            vTaskDelete(NULL);
        },
        "reset_task", 4096, NULL, 1, NULL);
    return 0;
}

const RpcRoute rpcRoutes[] = {
    {"Config.Get", handleConfigGet},
    {"Config.Set", handleConfigSet},
    {"System.Reboot", handleSystemReboot},
    {"System.Reset", handleSystemReset},
};

const size_t numRoutes = sizeof(rpcRoutes) / sizeof(rpcRoutes[0]);

esp_err_t rpc_post_handler(httpd_req_t* req, Config* config) {
    char buf[MAX_PAYLOAD_SIZE];
    int remaining = req->content_len;

    // Reject payloads that are too large
    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }

    int received = 0;
    while (remaining > 0) {
        int ret = httpd_req_recv(req, buf + received, remaining);
        if (ret <= 0) {  // Timeout or error
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        received += ret;
        remaining -= ret;
    }
    buf[received] = '\0';

#ifdef DEBUG
    Serial.println(buf);
#endif

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buf);

    if (error) {
        const char* errResp =
            "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Parse error\"},\"id\":null}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, errResp, strlen(errResp));
        return ESP_OK;
    }

    const char* method = doc["method"];
    int id = doc["id"] | 0;

    JsonDocument responseDoc;
    responseDoc["jsonrpc"] = JSON_RPC_VERSION;
    responseDoc["id"] = id;

    JsonDocument params;
    if (doc["params"].is<const char*>()) {
        error = deserializeJson(params, doc["params"].as<const char*>());
    } else {
        params.set(doc["params"]);
    }
    if (error) {
        const char* errResp =
            "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Parse error of params field\"},\"id\":null}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, errResp, strlen(errResp));
        return ESP_OK;
    }
    JsonObject resultObj = responseDoc["result"].to<JsonObject>();

    // Dispatch
    bool methodFound = false;
    for (size_t i = 0; i < numRoutes; i++) {
        if (strcmp(rpcRoutes[i].method, method) == 0) {
            methodFound = true;
            int errCode = rpcRoutes[i].handler(params.as<JsonObject>(), resultObj, config);

            if (errCode != 0) {
                responseDoc.remove("result");
                JsonObject errorObj = responseDoc["error"].to<JsonObject>();
                errorObj["code"] = errCode;
                errorObj["message"] = "RPC execution error";
            }
            break;
        }
    }

    if (!methodFound) {
        responseDoc.remove("result");
        JsonObject errorObj = responseDoc["error"].to<JsonObject>();
        errorObj["code"] = -32601;
        errorObj["message"] = "Method not found";
    }

    String responseStr;
    serializeJson(responseDoc, responseStr);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_send(req, responseStr.c_str(), responseStr.length());

    return ESP_OK;
}
