#include <Arduino.h>
#include <esp_http_server.h>
#include <WiFi.h>

#include "deviceconfig.h"

esp_err_t metrics_get_handler(httpd_req_t *req, DeviceConfig *currentConfig) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
        "# HELP esp32_temperature_c Current temperature\n"
        "# TYPE esp32_temperature_c gauge\n"
        "esp32_temperature_c{device=\"%s\"} %.2f\n"
        "# HELP esp32_wifi_rssi_dbm WiFi signal strength\n"
        "# TYPE esp32_wifi_rssi_dbm gauge\n"
        "esp32_wifi_rssi_dbm{device=\"%s\"} %d\n",
        currentConfig->DeviceName, temperatureRead(),
        currentConfig->DeviceName, (int)WiFi.RSSI()
    );
    
    Serial.println((int)WiFi.RSSI());

    httpd_resp_set_type(req, "text/plain; version=0.0.4");
    httpd_resp_send(req, buffer, strlen(buffer));
    return ESP_OK;
}