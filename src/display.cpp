#include "display.h"

#include <Arduino.h>
#include <NeoPixelBusLg.h>

#include "config.h"
#include "layout.h"
#include "settings.h"

namespace {

// No gamma: brightness scales colors linearly, like the original NeoPixelBrightnessBus
NeoPixelBusLg<NeoGrbFeature, Neo800KbpsMethod, NeoGammaNullMethod> strip(LED_COUNT, LED_PIN);

const RgbColor BLACK(0);
const RgbColor WHITE(200, 200, 200);
const int PRESET_HUES[10] = {327, 150, 130, 30, 0, -1, 100, 190, 300, 50};  // -1 = white
const unsigned long RAINBOW_STEP_MS = 100;

unsigned long rainbowTimer = 0;

HslColor hsl(float hue) {
  return HslColor(hue / 360.0f, 1.0f, 0.25f);
}

void stepRainbow() {
  if (millis() - rainbowTimer > RAINBOW_STEP_MS) {
    rainbowTimer = millis();
    settings.rainbowHue = (settings.rainbowHue + 1) % 360;
  }
}

// Sets one LED in the current color
void paint(uint8_t led) {
  int color = settings.color;
  if (color >= 0 && color <= 9) {
    if (PRESET_HUES[color] < 0) {
      strip.SetPixelColor(led, WHITE);
    } else {
      strip.SetPixelColor(led, hsl(PRESET_HUES[color]));
    }
  } else if (color == COLOR_RAINBOW) {
    stepRainbow();
    strip.SetPixelColor(led, hsl(settings.rainbowHue - led));
  } else if (color == COLOR_CYCLE) {
    stepRainbow();
    strip.SetPixelColor(led, hsl(settings.rainbowHue));
  } else if (color == COLOR_FROZEN) {
    strip.SetPixelColor(led, hsl(settings.rainbowHue));
  }
}

}  // namespace

void displayBegin() {
  strip.Begin();
}

void displayClear() {
  strip.ClearTo(BLACK);
}

void displayShow() {
  strip.Show();
}

void displaySetLuminance(uint8_t value) {
  strip.SetLuminance(value);
}

void displayLedTest() {
  for (int hue : {0, 120, 240}) {
    for (uint16_t led = 0; led < LED_COUNT; led++) {
      strip.SetPixelColor(led, hsl(hue));
      strip.Show();
      delay(50);
      displayClear();
    }
  }
}

void displayTime(int hour, int minute) {
  displayClear();
  layoutTime(hour, minute, paint);
}

void displayWifiSymbol() {
  layoutWifiSymbol(paint);
}

void displayAutoMenu() {
  displayClear();
  layoutAutoSymbol(paint);
  displayShow();
}

void displayManualMenu() {
  displayClear();
  layoutManualSymbol(paint);
  layoutLevelBars(settings.brightnessLevel, paint);
  displayShow();
}
