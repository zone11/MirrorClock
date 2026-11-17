#pragma once

#include <stdint.h>

void brightnessBegin();                                   // starts I2C and the BH1750
void brightnessReadSensor(unsigned long now, bool autoMode);  // reads once per second, only in auto mode
uint8_t brightnessAuto();                                 // luminance 40-255 from the last reading
uint8_t brightnessManual(int level);                      // luminance for manual level 1-5
