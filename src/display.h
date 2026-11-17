#pragma once

#include <stdint.h>

void displayBegin();
void displayClear();                     // clears the buffer only
void displayShow();                      // sends the buffer to the LEDs
void displaySetLuminance(uint8_t value); // applies to pixels drawn afterwards
void displayLedTest();                   // red, green, blue chase over all LEDs (~17 s)

// The following draw in the current color (settings.color)
void displayTime(int hour, int minute);  // clears, then draws the time
void displayWifiSymbol();
void displayAutoMenu();                  // clears, draws "A" and shows it
void displayManualMenu();                // clears, draws "M" with level bars and shows it
