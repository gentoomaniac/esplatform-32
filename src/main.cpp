#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_http_server.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "deviceconfig.h"
#include "metrics.h"

// ==========================================
// 1. STATE & CONFIGURATION
// ==========================================

// Global config instance
DeviceConfig* currentConfig;

// ==========================================
// 2. C-STYLE JSON-RPC DISPATCHER
// ==========================================
typedef int (*RpcHandler)(JsonObject params, JsonObject result);

// Handler for "Config.Get"
int handleConfigGet(JsonObject params, JsonObject result) {
    result["version"] = currentConfig->Version;
    result["device_name"] = currentConfig->DeviceName;
    return 0; // 0 means success
}

struct RpcRoute {
    const char* method;
    RpcHandler handler;
};

// C-style routing table
const RpcRoute rpcRoutes[] = {
    {"Config.Get", handleConfigGet}
};
const size_t numRoutes = sizeof(rpcRoutes) / sizeof(rpcRoutes[0]);


// ==========================================
// 3. ESP-IDF HTTP HANDLERS (Go-Style)
// ==========================================

// --- POST /rpc ---
esp_err_t rpc_post_handler(httpd_req_t *req) {
    char buf[1024]; // Max payload size for our RPC calls
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
        if (ret <= 0) { // Timeout or error
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        received += ret;
        remaining -= ret;
    }
    buf[received] = '\0'; // Null-terminate the string

    // Parse JSON
    JsonDocument doc; 
    DeserializationError error = deserializeJson(doc, buf);

    if (error) {
        const char* errResp = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Parse error\"},\"id\":null}";
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
            int errCode = rpcRoutes[i].handler(params, resultObj);
            
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

esp_err_t metrics_handler(httpd_req_t *req) {
    return metrics_get_handler(req, currentConfig);
}

// ==========================================
// 4. SERVER INITIALIZATION 
// ==========================================

httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Increase URI match length if you have long endpoints later
    config.max_uri_handlers = 8; 

    // Route Definitions
    httpd_uri_t metrics_uri = {
        .uri       = "/metrics",
        .method    = HTTP_GET,
        .handler   = metrics_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t rpc_uri = {
        .uri       = "/rpc",
        .method    = HTTP_POST,
        .handler   = rpc_post_handler,
        .user_ctx  = NULL
    };

    Serial.println("Starting Native ESP-IDF HTTP Server...");
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &metrics_uri);
        httpd_register_uri_handler(server, &rpc_uri);
        return server;
    }

    Serial.println("Error starting server!");
    return NULL;
}


// ==========================================
// 5. MAIN LIFECYCLE
// ==========================================
void setup() {
    currentConfig = GetDefaultConfig();
    Serial.begin(115200);

    if (!LittleFS.begin(true)) {
        Serial.println("Critical Error: LittleFS mount failed!");
        return; 
    }

    // Wi-Fi Connection
    Serial.print("Connecting to Wi-Fi ...");
    WiFi.begin(currentConfig->Wifi.SSID, currentConfig->Wifi.Password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected! IP Address: ");
    Serial.println(WiFi.localIP());

    // Start the server
    start_webserver();
}

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // 2 is the standard pin for most ESP32 dev boards
#endif

unsigned long previousLedMillis = 0;
const long ledInterval = 1000; // Blink interval in milliseconds (1 second)
bool ledState = false;

void loop() {
    // Get the current uptime in milliseconds
    unsigned long currentMillis = millis();

    // Check if it's time to toggle the LED
    if (currentMillis - previousLedMillis >= ledInterval) {
        // Save the last time you blinked the LED
        previousLedMillis = currentMillis;

        // Toggle the state
        ledState = !ledState;

        // Apply the new state to the physical pin
        digitalWrite(LED_BUILTIN, ledState);
    }
    
    // Future non-blocking sensor reads will go here!
    // e.g., if (currentMillis - previousSensorMillis >= 5000) { readDHT(); }
}