#include "info.h"

#include <ArduinoJson.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_secure_boot.h>
#include <esp_system.h>
#include <soc/soc_caps.h>

// For newer ESP-IDF/Arduino versions, use this header for flash mapping if needed:
#include "esp_partition.h"
#include "system.h"

typedef struct {
    const char* model;
    const char* arch;  // "Xtensa" or "RISC-V"
    uint8_t cores;
    uint32_t cpu_freq_mhz;

    // Wireless Specs
    const char* bt_version;
    const char* wifi_gen;
    const char* wifi_band;
    bool has_classic_bt;
    bool has_802_15_4;  // Zigbee/Thread
    bool matter_capable;

    // Memory & Storage
    uint32_t flash_size_mb;
    uint32_t psram_size_kb;
    const char* flash_mode;
    uint32_t flash_speed_mhz;

    // Silicon & Security
    uint32_t chip_revision;
    const char* mac_addr;
    bool secure_boot_enabled;

    // Peripherals
    uint16_t adc_resolution;
    bool has_usb_otg;
    bool has_touch_sensors;
} DeviceProfile;

void serializeDeviceProfile(const DeviceProfile& dp, JsonObject obj) {
    obj["model"] = dp.model;
    obj["arch"] = dp.arch;
    obj["chip_revision"] = dp.chip_revision;
    obj["cores"] = dp.cores;
    obj["cpu_freq_mhz"] = dp.cpu_freq_mhz;

    obj["mac"] = getMac();
    obj["memory_kb"] = dp.psram_size_kb;
    obj["flash_mb"] = dp.flash_size_mb;
    obj["flash_mode"] = dp.flash_mode;
    obj["flash_speed_mhz"] = dp.flash_speed_mhz;

    obj["wifi_gen"] = dp.wifi_gen;
    obj["wifi_band"] = dp.wifi_band;

    obj["bt_version"] = dp.bt_version;
    obj["bt_classic"] = dp.has_classic_bt;

    obj["zigbee_capable"] = dp.has_802_15_4;
    obj["matter_capable"] = dp.matter_capable;

    obj["adc"] = dp.adc_resolution;
    obj["usb_otg"] = dp.has_usb_otg;
    obj["touch_gpio"] = dp.has_touch_sensors;
}

void get_detailed_info(DeviceProfile* p) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    // --- Basic Info ---
    p->model = CONFIG_IDF_TARGET;
    p->cores = chip_info.cores;
    p->cpu_freq_mhz = getCpuFrequencyMhz();
    p->chip_revision = chip_info.revision;

// --- Architecture ---
#if defined(CONFIG_IDF_TARGET_ARCH_RISCV)
    p->arch = "RISC-V";
#else
    p->arch = "Xtensa";
#endif

// --- Wireless Logic ---
#if defined(CONFIG_IDF_TARGET_ESP32E22)
    p->bt_version = "6.0";
    p->wifi_gen = "Wi-Fi 6E (ax)";
    p->wifi_band = "2.4/5/6GHz";
    p->has_classic_bt = false;
    p->has_802_15_4 = true;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
    p->bt_version = "5.3";
    p->wifi_gen = "Wi-Fi 6 (ax)";
    p->wifi_band = "2.4GHz";
    p->has_classic_bt = false;
    p->has_802_15_4 = true;
#elif defined(CONFIG_IDF_TARGET_ESP32C5)
    p->bt_version = "5.2";
    p->wifi_gen = "Wi-Fi 6 (ax)";
    p->wifi_band = "2.4/5GHz";
    p->has_classic_bt = false;
    p->has_802_15_4 = false;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    p->bt_version = "5.0";
    p->wifi_gen = "Wi-Fi 4 (n)";
    p->wifi_band = "2.4GHz";
    p->has_classic_bt = false;
    p->has_802_15_4 = false;
#elif defined(CONFIG_IDF_TARGET_ESP32)
    p->bt_version = "4.2";
    p->wifi_gen = "Wi-Fi 4 (n)";
    p->wifi_band = "2.4GHz";
    p->has_classic_bt = true;
    p->has_802_15_4 = false;
#else
    p->bt_version = "5.x";
    p->wifi_gen = "Wi-Fi 4/6";
    p->wifi_band = "2.4GHz";
    p->has_classic_bt = false;
    p->has_802_15_4 = false;
#endif

    p->matter_capable = (p->has_802_15_4 || (chip_info.features & CHIP_FEATURE_BLE));

    // --- Memory & Storage ---
    p->psram_size_kb = ESP.getPsramSize() / 1024;

    uint32_t flash_size;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        p->flash_size_mb = flash_size / (1024 * 1024);
    }

    // Safer way to get Flash Speed in Arduino/ESP-IDF
    p->flash_speed_mhz = ESP.getFlashChipSpeed() / 1000000;

// Get Flash Mode safely via SDK macros
#if defined(CONFIG_ESPTOOLPY_FLASHMODE_QIO)
    p->flash_mode = "QIO";
#elif defined(CONFIG_ESPTOOLPY_FLASHMODE_DIO)
    p->flash_mode = "DIO";
#elif defined(CONFIG_ESPTOOLPY_FLASHMODE_QOUT)
    p->flash_mode = "QOUT";
#else
    p->flash_mode = "DOUT";
#endif

// Check Secure Boot (Wrapped for compatibility)
#if CONFIG_SECURE_BOOT_ENABLED
    p->secure_boot_enabled = esp_secure_boot_enabled();
#else
    p->secure_boot_enabled = false;
#endif

    // --- Peripherals ---
    p->adc_resolution = SOC_ADC_MAX_BITWIDTH;

#if defined(SOC_USB_OTG_SUPPORTED) && SOC_USB_OTG_SUPPORTED
    p->has_usb_otg = true;
#else
    p->has_usb_otg = false;
#endif

#if defined(SOC_TOUCH_SENSOR_NUM) && (SOC_TOUCH_SENSOR_NUM > 0)
    p->has_touch_sensors = true;
#else
    p->has_touch_sensors = false;
#endif
}

esp_err_t info_get_handler(httpd_req_t* req, Config* config) {
    JsonDocument doc;
    doc["id"] = config->sys.id;
    doc["name"] = config->sys.device.name;
    doc["fw"] = config->sys.device.fwVersion;
    doc["auth"] = config->sys.auth.enabled ? "true" : "false";
    doc["auth_domain"] = config->sys.id;

    DeviceProfile dp;
    get_detailed_info(&dp);
    JsonObject deviceObj = doc["device"].to<JsonObject>();
    serializeDeviceProfile(dp, deviceObj);

    String jsonOutput;
    serializeJson(doc, jsonOutput);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_send(req, jsonOutput.c_str(), jsonOutput.length());
    return ESP_OK;
}
