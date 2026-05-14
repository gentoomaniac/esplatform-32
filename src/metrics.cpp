#include <Arduino.h>
#include <esp_http_server.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "deviceconfig.h"

esp_err_t metrics_get_handler(httpd_req_t *req, DeviceConfig *currentConfig) {
    uint8_t cpuMHz = ESP.getCpuFreqMHz();

    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();

    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), 
        "# HELP esp32_temperature_c Current temperature\n"
        "# TYPE esp32_temperature_c gauge\n"
        "esp32_temperature_c{device=\"%s\"} %.2f\n"
        "# HELP esp32_wifi_rssi_dbm WiFi signal strength\n"
        "# TYPE esp32_wifi_rssi_dbm gauge\n"
        "esp32_wifi_rssi_dbm{device=\"%s\"} %d\n"
        "# HELP esp32_uptime_seconds System uptime\n"
        "# TYPE esp32_uptime_seconds counter\n"
        "esp32_uptime_seconds{device=\"%s\"} %lu\n"
        "# HELP esp32_fs_bytes Filesystem stats\n"
        "# TYPE esp32_fs_bytes gauge\n"
        "esp32_fs_bytes{device=\"%s\",type=\"total\"} %zu\n"
        "esp32_fs_bytes{device=\"%s\",type=\"used\"} %zu\n"
        "# HELP esp32_memory_bytes Memory usage\n"
        "# TYPE esp32_memory_bytes gauge\n"
        "esp32_memory_bytes{device=\"%s\",type=\"total\"} %u\n"
        "esp32_memory_bytes{device=\"%s\",type=\"free\"} %u\n"
        "esp32_memory_bytes{device=\"%s\",type=\"minfree\"} %u\n"
        "# HELP esp32_clockspeed CPU clockspeed\n"
        "# TYPE esp32_clockspeed gauge\n"
        "esp32_clockspeed{device=\"%s\"} %u\n",
        currentConfig->DeviceName, temperatureRead(),
        currentConfig->DeviceName, (int)WiFi.RSSI(),
        currentConfig->DeviceName, (unsigned long) millis() / 1000,
        currentConfig->DeviceName, totalBytes,
        currentConfig->DeviceName, usedBytes,
        currentConfig->DeviceName, totalHeap,
        currentConfig->DeviceName, freeHeap,
        currentConfig->DeviceName, minFreeHeap,
        currentConfig->DeviceName, cpuMHz
    );
    
    httpd_resp_set_type(req, "text/plain; version=0.0.4");
    httpd_resp_send(req, buffer, strlen(buffer));
    return ESP_OK;
}