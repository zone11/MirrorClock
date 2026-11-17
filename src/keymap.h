#pragma once

#include <stdint.h>

// Key numbers shared by the IR remote, the web interface and the tests.
// Digits 0-9 are their own key number (color presets).
enum Key : int {
  KEY_NONE = -1,
  KEY_CYCLE = 11,    // *  : whole face cycles through all colors
  KEY_RAINBOW = 12,  // #  : rainbow across the face
  KEY_UP = 13,       // brighter (only while the brightness menu is open)
  KEY_LEFT = 14,     // automatic brightness
  KEY_OK = 15,       // freeze the cycling color
  KEY_RIGHT = 16,    // manual brightness
  KEY_DOWN = 17,     // darker (only while the brightness menu is open)
};

bool isValidKey(int key);       // 0-9 or 11-17
int keyForCode(uint64_t code);  // NEC code -> key number, KEY_NONE if unknown
