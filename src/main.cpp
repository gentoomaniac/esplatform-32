#include <Arduino.h>
#include <esp_http_server.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "deviceconfig.h"
#include "info.h"
#include "metrics.h"
#include "rpc.h"
#include "system.h"

Config* config;

esp_err_t rpc_handler(httpd_req_t *req) {
    return rpc_post_handler(req, config);
}

esp_err_t metrics_handler(httpd_req_t *req) {
    return metrics_get_handler(req, config);
}

esp_err_t info_handler(httpd_req_t *req) {
    return info_get_handler(req, config);
}

httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Increase URI match length if you have long endpoints later
    config.max_uri_handlers = 8; 

    
    httpd_uri_t info_uri = {
        .uri       = "/info",
        .method    = HTTP_GET,
        .handler   = info_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t metrics_uri = {
        .uri       = "/metrics",
        .method    = HTTP_GET,
        .handler   = metrics_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t rpc_uri = {
        .uri       = "/rpc",
        .method    = HTTP_POST,
        .handler   = rpc_handler,
        .user_ctx  = NULL
    };

    Serial.println("Starting Native ESP-IDF HTTP Server...");
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &info_uri);
        httpd_register_uri_handler(server, &metrics_uri);
        httpd_register_uri_handler(server, &rpc_uri);
        return server;
    }

    Serial.println("Error starting server!");
    return NULL;
}

void setup() {
    Serial.begin(115200);
    config = GetDefaultConfig();
    Serial.println("default config loaded");

    if (config->sys.debug.serialEnabled) {
        Serial.begin(config->sys.debug.baud);
        Serial.println("Serial console enabled");
    }

    if (!LittleFS.begin(true)) {
        Serial.println("Critical Error: LittleFS mount failed!");
        return; 
    }

    if (config->wifi.ap.enabled && config->wifi.sta.enabled) {
        WiFi.mode(WIFI_AP_STA);
    }

    //start WiFi AP
    if (config->wifi.ap.enabled) {
        Serial.print("Enabling access point with SSID `"); Serial.print(config->wifi.ap.ssid); Serial.println("` ...");
        WiFi.softAP(config->wifi.ap.ssid, config->wifi.ap.password);
        Serial.println("Gateway IP: " + WiFi.softAPIP().toString());
    }

    // Connect to existing WiFi
    if (config->wifi.sta.enabled) {
        Serial.print("Connecting to Wi-Fi "); Serial.print(config->wifi.sta.ssid); Serial.print(" ");
        WiFi.begin(config->wifi.sta.ssid, config->wifi.sta.password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.print("\nConnected!\nIP Address: "); Serial.println(WiFi.localIP());
    }

    pinMode(LED_BUILTIN, OUTPUT);

    // Start the server
    start_webserver();
}

void loop() {
    onboardLed();
}