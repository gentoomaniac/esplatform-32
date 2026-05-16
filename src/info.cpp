#include "info.h"

#include <ArduinoJson.h>
#include <esp_chip_info.h>

#include "system.h"

esp_err_t info_get_handler(httpd_req_t* req, Config* config) {
    JsonDocument doc;
    doc["id"] = config->sys.id;
    doc["name"] = config->sys.device.name;
    doc["fw"] = config->sys.device.fwVersion;
    doc["hw"] = getHardwareRevisionString();
    doc["auth"] = config->sys.auth.enabled ? "true" : "false";
    doc["auth_domain"] = config->sys.id;

    String jsonOutput;
    serializeJson(doc, jsonOutput);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_send(req, jsonOutput.c_str(), jsonOutput.length());
    return ESP_OK;
}
