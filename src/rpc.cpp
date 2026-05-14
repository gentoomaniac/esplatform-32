#include "rpc.h"

#include <ArduinoJson.h>
#include <esp_http_server.h>

#include "deviceconfig.h"

// Handler for "Config.Get"
int handleConfigGet(JsonObject params, JsonObject result, Config* config) {
    result["version"] = config->sys.device.fwVersion;
    result["device_name"] = config->sys.device.name;
    return 0;  // 0 means success
}

const RpcRoute rpcRoutes[] = {{"Config.Get", handleConfigGet}};

const size_t numRoutes = sizeof(rpcRoutes) / sizeof(rpcRoutes[0]);

esp_err_t rpc_post_handler(httpd_req_t* req, Config* config) {
    char buf[1024];  // Max payload size for our RPC calls
    int remaining = req->content_len;

    // Reject payloads that are too large
    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }

    // Read the body into our buffer safely
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
    buf[received] = '\0';  // Null-terminate the string

    // Parse JSON
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
    JsonObject params = doc["params"];

    // Prepare response JSON
    JsonDocument responseDoc;
    responseDoc["jsonrpc"] = "2.0";
    responseDoc["id"] = id;
    JsonObject resultObj = responseDoc["result"].to<JsonObject>();

    // Dispatch
    bool methodFound = false;
    for (size_t i = 0; i < numRoutes; i++) {
        if (strcmp(rpcRoutes[i].method, method) == 0) {
            methodFound = true;
            int errCode = rpcRoutes[i].handler(params, resultObj, config);

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

    // Serialize and send
    String responseStr;
    serializeJson(responseDoc, responseStr);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, responseStr.c_str(), responseStr.length());

    return ESP_OK;
}
