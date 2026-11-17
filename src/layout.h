#pragma once

#include <stdint.h>

// Which LEDs form the time and the symbols on the clock face.
// Pure logic without hardware access, so it can be unit-tested on the host.

typedef void (*LightFn)(uint8_t led);  // called once for every LED to switch on

void layoutTime(int hour, int minute, LightFn light);  // hour 0-23, minute 0-59
void layoutWifiSymbol(LightFn light);
void layoutAutoSymbol(LightFn light);                  // "A" for automatic brightness
void layoutManualSymbol(LightFn light);                // "M" for manual brightness
void layoutLevelBars(int level, LightFn light);        // bars for brightness level 1-5
