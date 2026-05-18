#pragma once

class ESPlatform32 {
   private:
    static void taskWrapper(void* pvParameters);
    void run();

   public:
    void begin();
};

extern ESPlatform32 esPlatform32;
