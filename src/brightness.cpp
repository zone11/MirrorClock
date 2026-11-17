#include "brightness.h"

#include <Arduino.h>
#include <BH1750.h>
#include <Wire.h>

namespace {

const unsigned long READ_INTERVAL_MS = 1000;
const uint8_t MANUAL_LEVELS[5] = {30, 60, 100, 150, 255};

BH1750 lightMeter;
int lux = 0;
unsigned long lastRead = 0;

}  // namespace

void brightnessBegin() {
  Wire.begin(D4, D3);  // SDA = D4 (GPIO2), SCL = D3 (GPIO0)
  lightMeter.begin();
}

void brightnessReadSensor(unsigned long now, bool autoMode) {
  if (now - lastRead > READ_INTERVAL_MS) {
    lastRead = millis();
    if (autoMode) {
      float reading = lightMeter.readLightLevel();
      if (reading >= 0) {  // -1 / -2 = sensor error, keep the last reading
        lux = reading;
      }
    }
  }
}

uint8_t brightnessAuto() {
  return map(constrain(lux, 0, 10), 0, 10, 40, 255);  // full brightness from 10 lux
}

uint8_t brightnessManual(int level) {
  return MANUAL_LEVELS[constrain(level, 1, 5) - 1];
}
