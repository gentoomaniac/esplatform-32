#include "platform.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_http_server.h>

#include "deviceconfig.h"
#include "info.h"
#include "metrics.h"
#include "rpc.h"
#include "system.h"

static const int RESET_BUTTON_PRESS_SECONDS = 10;

ESPlatform32 esPlatform32;

httpd_handle_t start_webserver(Config* config) {
    httpd_handle_t server = NULL;
    httpd_config_t httpDefaultConfig = HTTPD_DEFAULT_CONFIG();
    httpDefaultConfig.stack_size = 8192;
    httpDefaultConfig.lru_purge_enable = true;
    httpDefaultConfig.max_uri_handlers = 4;

    httpd_uri_t info_uri = {.uri = "/info", .method = HTTP_GET, .handler = info_handler, .user_ctx = config};

    httpd_uri_t metrics_uri = {.uri = "/metrics", .method = HTTP_GET, .handler = metrics_handler, .user_ctx = config};

    httpd_uri_t rpc_uri = {.uri = "/rpc", .method = HTTP_POST, .handler = rpc_handler, .user_ctx = config};

    Serial.println("Starting Native ESP-IDF HTTP Server...");
    if (httpd_start(&server, &httpDefaultConfig) == ESP_OK) {
        httpd_register_uri_handler(server, &info_uri);
        httpd_register_uri_handler(server, &metrics_uri);
        httpd_register_uri_handler(server, &rpc_uri);
        return server;
    }

    Serial.println("Error starting server!");
    return NULL;
}

void resetSystem() {
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
}

void waitForSystemReset(void* pvParameters) {
    uint8_t button_pin = (uint8_t)(uintptr_t)pvParameters;
    while (true) {
        if (!digitalRead(button_pin)) {
            unsigned long start = millis();
            Serial.println("button pressed ...");

            while (millis() - start < RESET_BUTTON_PRESS_SECONDS * 1000) {
                if (digitalRead(button_pin)) {
                    Serial.println("reset button released");
                    return;
                }
                Serial.print(".");
                delay(100);
            }
            Serial.printf("\nresetting ...\n");
            resetSystem();
        }

        delay(100);
    }
}

void ESPlatform32::setConfigValue(std::string key, ConfigValue value) {
    config.customConfig[key] = value;
}

void ESPlatform32::taskWrapper(void* pvParameters) {
    ESPlatform32* app = (ESPlatform32*)pvParameters;
    app->run();
}

void ESPlatform32::run() {
    while (true) {
        if (config.sys.led) {
            onboardLed(this->ledPin);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ESPlatform32::begin(uint8_t led_pin, uint8_t button_pin) {
    this->ledPin = led_pin;
    this->buttonPin = button_pin;

    Serial.begin(115200);
    xTaskCreatePinnedToCore(waitForSystemReset, "ESPlatform32_reset_watchdog", 4096, (void*)(uintptr_t)this->buttonPin,
                            1, NULL,
                            0  // Run on Core 0 (Main loop usually runs on Core 1)
    );

    if (!LittleFS.begin(true)) {
        Serial.println("Critical Error: LittleFS mount failed!");
        return;
    }

    getDefaultConfig(&config);
    // loadConfig(&config);

    if (config.sys.debug.serialEnabled) {
        Serial.begin(config.sys.debug.baud);
        Serial.println("Serial console enabled");
    }

    if (config.wifi.ap.enabled && config.wifi.sta.enabled) {
        WiFi.mode(WIFI_AP_STA);
    }

    // start WiFi AP
    if (config.wifi.ap.enabled) {
        Serial.print("Enabling access point with SSID `");
        Serial.print(config.wifi.ap.ssid);
        Serial.println("` ...");
        WiFi.softAP(config.wifi.ap.ssid, config.wifi.ap.password);
        Serial.println("Gateway IP: " + WiFi.softAPIP().toString());
    }

    // Connect to existing WiFi
    if (config.wifi.sta.enabled) {
        Serial.print("Connecting to Wi-Fi ");
        Serial.print(config.wifi.sta.ssid);
        Serial.print(" ");
        WiFi.begin(config.wifi.sta.ssid, config.wifi.sta.password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.print("\nConnected!\nIP Address: ");
        Serial.println(WiFi.localIP());
    }

    pinMode(led_pin, OUTPUT);

    if (WiFi.localIP() || WiFi.softAPIP()) {
        delay(100);
        start_webserver(&config);
    } else {
        while (true) {
            digitalWrite(led_pin, !digitalRead(led_pin));
            delay(100);
        }
    }

    // start main bg task
    xTaskCreatePinnedToCore(taskWrapper, "ESPlatform32", 4096, this, 1, NULL,
                            0  // Run on Core 0 (Main loop usually runs on Core 1)
    );
}
