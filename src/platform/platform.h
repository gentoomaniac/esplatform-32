#pragma once

#include "deviceconfig.h"

class ESPlatform32 {
   private:
    int ledPin;
    int buttonPin;
    Config config;

    static void taskWrapper(void* pvParameters);
    void run();

   public:
    void begin(uint8_t, uint8_t);

    void setConfigValue(std::string, ConfigValue);

    template <typename T>
    T* registerConfig(std::string key, T defaultValue) {
        if (config.customConfig.find(key) == config.customConfig.end()) {
            config.customConfig[key] = defaultValue;
        }

        // Return the pointer to the value inside the variant
        return std::get_if<T>(&config.customConfig[key]);
    }

    template <typename T>
    std::optional<T> getConfigValue(const std::string& key) {
        auto it = config.customConfig.find(key);
        if (it != config.customConfig.end()) {
            if (auto* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        return std::nullopt;
    }
};

extern ESPlatform32 esPlatform32;
