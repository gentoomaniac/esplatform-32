#include "auth.h"

#include <Arduino.h>
#include <esp_http_server.h>

#include "deviceconfig.h"

esp_err_t authed(httpd_req_t* req, Config* config, AuthedHandler handler) {
    // auth code
    if (config->sys.auth.enabled) {
        Serial.println("would need auth, to be implemented");
    }

    return handler(req, config);
}
