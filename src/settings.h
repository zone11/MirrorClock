#pragma once

// Color: 0-9 are presets, the others are animated modes.
enum ColorMode : int {
  COLOR_RAINBOW = 77,  // rainbow across the face
  COLOR_CYCLE = 88,    // whole face cycles through all colors
  COLOR_FROZEN = 99,   // cycle stopped at rainbowHue
};

enum BrightnessMode : int {
  BRIGHTNESS_AUTO = 0,    // from the light sensor
  BRIGHTNESS_MANUAL = 1,  // fixed level 1-5
};

struct Settings {
  int color = 1;
  int rainbowHue = 0;  // 0-359, used by rainbow, cycle and frozen
  int brightnessMode = BRIGHTNESS_AUTO;
  int brightnessLevel = 2;  // manual level 1-5
};

extern Settings settings;

void loadSettings();  // at boot; invalid stored values keep their defaults
void saveSettings();  // writes flash only if a value changed
