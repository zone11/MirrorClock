#include "settings.h"

#include <EEPROM.h>

Settings settings;

// Same layout as the original firmware (four ints), so saved settings survive the update.
// Replaced by a versioned struct with checksum in a later phase.
constexpr int ADDR_COLOR = 0;
constexpr int ADDR_RAINBOW_HUE = 4;
constexpr int ADDR_BRIGHTNESS_MODE = 8;
constexpr int ADDR_BRIGHTNESS_LEVEL = 12;
constexpr int STORAGE_SIZE = 16;

void loadSettings() {
  EEPROM.begin(STORAGE_SIZE);
  int color, hue, mode, level;
  EEPROM.get(ADDR_COLOR, color);
  EEPROM.get(ADDR_RAINBOW_HUE, hue);
  EEPROM.get(ADDR_BRIGHTNESS_MODE, mode);
  EEPROM.get(ADDR_BRIGHTNESS_LEVEL, level);

  // empty or foreign flash: ignore invalid values
  if ((color >= 0 && color <= 9) || color == COLOR_RAINBOW || color == COLOR_CYCLE || color == COLOR_FROZEN) {
    settings.color = color;
  }
  if (hue >= 0 && hue < 360) {
    settings.rainbowHue = hue;
  }
  if (mode == BRIGHTNESS_AUTO || mode == BRIGHTNESS_MANUAL) {
    settings.brightnessMode = mode;
  }
  if (level >= 1 && level <= 5) {
    settings.brightnessLevel = level;
  }
}

void saveSettings() {
  EEPROM.put(ADDR_COLOR, settings.color);
  EEPROM.put(ADDR_RAINBOW_HUE, settings.rainbowHue);
  EEPROM.put(ADDR_BRIGHTNESS_MODE, settings.brightnessMode);
  EEPROM.put(ADDR_BRIGHTNESS_LEVEL, settings.brightnessLevel);
  EEPROM.commit();  // no flash write if nothing changed
}
