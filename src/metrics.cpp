#include <Arduino.h>
#include <esp_http_server.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "deviceconfig.h"

void esp32_temperature_c(httpd_req_t *req, DeviceConfig *config) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "# HELP esp32_temperature_c Current temperature\n"
        "# TYPE esp32_temperature_c gauge\n"
        "esp32_temperature_c{id=\"%s\",name=\"%s\"} %.2f\n",
        config->Sys.Id, config->Sys.DeviceName, temperatureRead()
    );
    httpd_resp_send_chunk(req, buffer, HTTPD_RESP_USE_STRLEN);
}

void esp32_clockspeed(httpd_req_t *req, DeviceConfig *config) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "# HELP esp32_clockspeed CPU clockspeed\n"
        "# TYPE esp32_clockspeed gauge\n"
        "esp32_clockspeed{id=\"%s\",name=\"%s\"} %u\n",
        config->Sys.Id, config->Sys.DeviceName, ESP.getCpuFreqMHz()
    );
    httpd_resp_send_chunk(req, buffer, HTTPD_RESP_USE_STRLEN);
}

void esp32_memory_bytes(httpd_req_t *req, DeviceConfig *config) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "# HELP esp32_memory_bytes Memory usage\n"
        "# TYPE esp32_memory_bytes gauge\n"
        "esp32_memory_bytes{id=\"%s\",name=\"%s\",type=\"total\"} %u\n"
        "esp32_memory_bytes{id=\"%s\",name=\"%s\",type=\"free\"} %u\n"
        "esp32_memory_bytes{id=\"%s\",name=\"%s\",type=\"minfree\"} %u\n",
        config->Sys.Id, config->Sys.DeviceName, ESP.getHeapSize(),
        config->Sys.Id, config->Sys.DeviceName, ESP.getFreeHeap(),
        config->Sys.Id, config->Sys.DeviceName, ESP.getMinFreeHeap()
    );
    httpd_resp_send_chunk(req, buffer, HTTPD_RESP_USE_STRLEN);
}

void esp32_fs_bytes(httpd_req_t *req, DeviceConfig *config) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "# HELP esp32_fs_bytes Filesystem stats\n"
        "# TYPE esp32_fs_bytes gauge\n"
        "esp32_fs_bytes{id=\"%s\",name=\"%s\",type=\"total\"} %zu\n"
        "esp32_fs_bytes{id=\"%s\",name=\"%s\",type=\"used\"} %zu\n",
        config->Sys.Id, config->Sys.DeviceName, LittleFS.totalBytes(),
        config->Sys.Id, config->Sys.DeviceName, LittleFS.usedBytes()
    );
    httpd_resp_send_chunk(req, buffer, HTTPD_RESP_USE_STRLEN);
}

void esp32_wifi_rssi_dbm(httpd_req_t *req, DeviceConfig *config) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
        "# HELP esp32_wifi_rssi_dbm WiFi signal strength\n"
        "# TYPE esp32_wifi_rssi_dbm gauge\n"
        "esp32_wifi_rssi_dbm{id=\"%s\",name=\"%s\"} %d\n",
        config->Sys.Id, config->Sys.DeviceName, (int)WiFi.RSSI()
    );
    httpd_resp_send_chunk(req, buffer, HTTPD_RESP_USE_STRLEN);
}

void esp32_uptime_seconds(httpd_req_t *req, DeviceConfig *config) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "# HELP esp32_uptime_seconds System uptime\n"
        "# TYPE esp32_uptime_seconds counter\n"
        "esp32_uptime_seconds{id=\"%s\",name=\"%s\"} %lu\n",
        config->Sys.Id, config->Sys.DeviceName, (unsigned long) millis() / 1000
    );
    httpd_resp_send_chunk(req, buffer, HTTPD_RESP_USE_STRLEN);
}

esp_err_t metrics_get_handler(httpd_req_t *req, DeviceConfig *config) {
    httpd_resp_set_type(req, "text/plain; version=0.0.4");
    esp32_temperature_c(req, config);
    esp32_clockspeed(req, config);
    esp32_memory_bytes(req, config);
    esp32_fs_bytes(req, config);
    esp32_wifi_rssi_dbm(req, config);
    esp32_uptime_seconds(req, config);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
