#ifdef WEB_KEYS

#include "web.h"

#include <ESP8266WebServer.h>

#include "controls.h"
#include "keymap.h"
#include "settings.h"

namespace {

ESP8266WebServer server(80);

void handleKey() {
  if (server.hasArg("nr")) {
    String nr = server.arg("nr");
    int key = nr.toInt();
    if (nr != String(key) || !isValidKey(key)) {
      server.send(400, "text/plain", "nr must be 0-9 or 11-17\n");
      return;
    }
    controlsHandleKey(key);
  }
  server.send(200, "text/plain",
              "color=" + String(settings.color) + " rainbowHue=" + String(settings.rainbowHue) +
                  " brightnessMode=" + String(settings.brightnessMode) +
                  " brightnessLevel=" + String(settings.brightnessLevel) +
                  " menu=" + String(controlsMenuActive()) + "\n");
}

}  // namespace

void webBegin() {
  server.on("/key", handleKey);
  server.begin();
}

void webHandle() {
  server.handleClient();
}

#endif
