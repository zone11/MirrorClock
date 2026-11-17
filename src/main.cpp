// MirrorClock firmware
// Swiss German word clock on an ESP8266 with 114 WS2812B LEDs.
// Based on the original firmware by Alf Müller.

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#include "brightness.h"
#include "controls.h"
#include "display.h"
#include "remote.h"
#include "settings.h"
#include "timesync.h"
#include "web.h"

void setup() {
  Serial.begin(115200);
  loadSettings();

  displayBegin();
#ifdef LED_TEST
  displayLedTest();
#endif
  displayWifiSymbol();
  displayShow();

  WiFiManager wifiManager;
  wifiManager.autoConnect("MirrorClock");  // opens a setup portal if no WiFi is saved
  Serial.println("WiFi connected");

  timesyncBegin();
  remoteBegin();
  brightnessBegin();
#ifdef WEB_KEYS
  webBegin();
#endif
}

void loop() {
  unsigned long now = millis();
#ifdef WEB_KEYS
  webHandle();
#endif
  controlsApplyBrightness(now);  // before drawing: luminance only applies to pixels drawn afterwards

  int hour, minute;
  if (timesyncLocalTime(hour, minute) && !controlsMenuActive()) {
    displayTime(hour, minute);
  }

  controlsCheckMenuTimeout();
  controlsHandleKey(remotePoll());
  displayShow();  // the only regular Show() per pass
  delay(20);
}
