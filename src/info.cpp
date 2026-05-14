#include "info.h"

#include <ArduinoJson.h>
#include <esp_chip_info.h>

const char* getChipModelName(esp_chip_model_t model) {
    switch (model) {
        case CHIP_ESP32:
            return "ESP32";
        case CHIP_ESP32S2:
            return "ESP32-S2";
        case CHIP_ESP32S3:
            return "ESP32-S3";
        case CHIP_ESP32C3:
            return "ESP32-C3";
        case CHIP_ESP32H2:
            return "ESP32-H2";
        default:
            return "Unknown";
    }
}

static char hardware_rev_buffer[64];
static bool hardware_rev_initialized = false;

const char* getHardwareRevisionString() {
    if (!hardware_rev_initialized) {
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);

        snprintf(hardware_rev_buffer, sizeof(hardware_rev_buffer), "%s v%d.%d", getChipModelName(chip_info.model),
                 chip_info.revision / 100, chip_info.revision % 100);

        hardware_rev_initialized = true;
    }

    return hardware_rev_buffer;
}

esp_err_t info_get_handler(httpd_req_t* req, Config* config) {
    JsonDocument doc;
    doc["id"] = config->sys.id;
    doc["name"] = config->sys.device.name;
    doc["fw"] = config->sys.device.fwVersion;
    doc["hw"] = getHardwareRevisionString();

    String jsonOutput;
    serializeJson(doc, jsonOutput);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, jsonOutput.c_str(), jsonOutput.length());
    return ESP_OK;
}