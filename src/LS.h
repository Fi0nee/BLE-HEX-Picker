#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <NimBLEDevice.h>
#include <cmath>

static uint16_t MANUFACTURER_ID = 0xFFF0;

#define MANUFACTURER_DATA_LENGTH 11
#define MANUFACTURER_DATA_PREFIX 0x6D, 0xB6, 0x43, 0xCE, 0x97, 0xFE, 0x42, 0x7C

static uint8_t _intensity_value = 0;
static uint8_t _last_intensity_value = 255;
static bool _stopping = false;

// Значения уровней вибрации (3-байтные hex)
static constexpr uint32_t VIB_STOP  = 0xE50000; // Stop
static constexpr uint32_t VIB_LOW1  = 0xF40000; // L1
static constexpr uint32_t VIB_LOW2  = 0xF70000; // L2
static constexpr uint32_t VIB_LOW3  = 0xF60000; // L3
static constexpr uint32_t VIB_LOW4  = 0xF10000; // L4
static constexpr uint32_t VIB_LOW5  = 0xF30000; // L5
static constexpr uint32_t VIB_MED   = 0xE70000; // L6
static constexpr uint32_t VIB_HIGH  = 0xE60000; // L7

#define VIB_LEVEL(v) MANUFACTURER_DATA_PREFIX, (uint8_t)(((v) >> 16) & 0xFF), (uint8_t)(((v) >> 8) & 0xFF), (uint8_t)((v) & 0xFF)

static uint8_t manufacturerDataList[][MANUFACTURER_DATA_LENGTH] = {
    { VIB_LEVEL(VIB_STOP) },
    { VIB_LEVEL(VIB_LOW1) },
    { VIB_LEVEL(VIB_LOW2) },
    { VIB_LEVEL(VIB_LOW3) },
    { VIB_LEVEL(VIB_LOW4) },
    { VIB_LEVEL(VIB_LOW5) },
    { VIB_LEVEL(VIB_MED)  },
    { VIB_LEVEL(VIB_HIGH) },
};

// ---------------- Установка manufacturer data и реклама ----------------
inline void set_manufacturer_data(uint8_t index) {
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->stop();

    uint8_t *manufacturerData = manufacturerDataList[index];
    pAdvertising->setManufacturerData(
        std::string((char *)&MANUFACTURER_ID, 2) +
        std::string((char *)manufacturerData, MANUFACTURER_DATA_LENGTH)
    );

    pAdvertising->start();
}

// ---------------- Задача BLE рекламы ----------------
inline void muse_advertising_task(void *pvParameters) {
    while (!_stopping) {
        if (_last_intensity_value != _intensity_value) {
            set_manufacturer_data(_intensity_value);
            _last_intensity_value = _intensity_value;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // один раз останавливаем все каналы
    set_manufacturer_data(0);
    vTaskDelete(NULL);
}

// ---------------- Установка интенсивности ----------------
inline void muse_set_intensity(float intensity_percent) {
    if (isnan(intensity_percent) || intensity_percent < 0.0f) intensity_percent = 0.0f;
    else if (intensity_percent > 1.0f) intensity_percent = 1.0f;

    _intensity_value = static_cast<uint8_t>(round(intensity_percent * 7.0f));
}

// ---------------- Запуск/остановка ----------------
inline void muse_start() {
    _stopping = false;
    xTaskCreatePinnedToCore(muse_advertising_task, "muse_advertising_task", 4096, nullptr, 2, nullptr, 0);
}

inline void muse_stop() {
    _stopping = true;
}

// ---------------- Инициализация ----------------
inline void muse_init() {
}
