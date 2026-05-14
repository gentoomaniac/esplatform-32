#include <Arduino.h>
#include <esp_http_server.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "deviceconfig.h"
#include "metrics.h"
#include "rpc.h"

DeviceConfig* currentConfig;

esp_err_t rpc_handler(httpd_req_t *req) {
    return rpc_post_handler(req, currentConfig);
}

esp_err_t metrics_handler(httpd_req_t *req) {
    return metrics_get_handler(req, currentConfig);
}

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
        .handler   = rpc_handler,
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