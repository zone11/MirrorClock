#include "controls.h"

#include <Arduino.h>

#include "brightness.h"
#include "display.h"
#include "keymap.h"
#include "settings.h"

namespace {

const unsigned long MENU_TIMEOUT_MS = 6000;

bool menuActive = false;
unsigned long menuTimer = 0;

void openMenu(int mode) {
  menuActive = true;
  menuTimer = millis();
  settings.brightnessMode = mode;
  if (mode == BRIGHTNESS_AUTO) {
    displayAutoMenu();
  } else {
    displayManualMenu();
  }
}

// Up/down also switch to manual brightness, like in the original firmware
void changeLevel(int delta) {
  menuTimer = millis();
  settings.brightnessMode = BRIGHTNESS_MANUAL;
  settings.brightnessLevel = constrain(settings.brightnessLevel + delta, 1, 5);
  displayManualMenu();
}

}  // namespace

void controlsHandleKey(int key) {
  if (!isValidKey(key)) {
    return;
  }
  if (key <= 9) {
    settings.color = key;
  }
  switch (key) {
    case KEY_CYCLE:
      settings.color = COLOR_CYCLE;
      break;
    case KEY_RAINBOW:
      settings.color = COLOR_RAINBOW;
      break;
    case KEY_UP:
      if (menuActive) {
        changeLevel(+1);
      }
      break;
    case KEY_DOWN:
      if (menuActive) {
        changeLevel(-1);
      }
      break;
    case KEY_LEFT:
      openMenu(BRIGHTNESS_AUTO);
      break;
    case KEY_RIGHT:
      openMenu(BRIGHTNESS_MANUAL);
      break;
    case KEY_OK:
      if (settings.color == COLOR_CYCLE) {
        settings.color = COLOR_FROZEN;
      }
      break;
  }
  saveSettings();
}

void controlsApplyBrightness(unsigned long now) {
  bool autoMode = settings.brightnessMode == BRIGHTNESS_AUTO;
  brightnessReadSensor(now, autoMode);
  if (autoMode) {
    displaySetLuminance(brightnessAuto());
  } else {
    if (menuActive) {
      displayManualMenu();
    }
    displaySetLuminance(brightnessManual(settings.brightnessLevel));
  }
}

void controlsCheckMenuTimeout() {
  // millis() here, not the loop start time: a key handled earlier in this pass may have set menuTimer later
  if (millis() - menuTimer > MENU_TIMEOUT_MS) {
    menuActive = false;
  }
}

bool controlsMenuActive() {
  return menuActive;
}
