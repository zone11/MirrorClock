#pragma once

#include <stdint.h>

constexpr uint16_t LED_COUNT = 114;  // 11 x 10 letter grid (0-109) + 4 minute dots (110-113)
constexpr uint8_t LED_PIN = 2;       // ignored: the ESP8266 DMA method always drives GPIO3 (RX)
constexpr uint8_t IR_PIN = 4;        // GPIO4 = D2
// BH1750 light sensor on I2C: SDA = D4 (GPIO2), SCL = D3 (GPIO0), see brightness.cpp
