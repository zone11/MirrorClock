#include "layout.h"

#include <stddef.h>

// LED index of a letter: columns are wired as a serpentine,
// index = col even ? col * 10 + row : col * 10 + (9 - row)

namespace {

// Words on the clock face (Swiss German)
const uint8_t ES_ISCH[] = {0, 19, 39, 40, 59, 60};
const uint8_t FUEF[]    = {80, 99, 100};
const uint8_t ZAEH[]    = {81, 98, 101};
const uint8_t VIERTU[]  = {1, 18, 21, 38, 41, 58};
const uint8_t ZWAENZG[] = {2, 17, 22, 37, 42, 57};
const uint8_t VOR[]     = {82, 97, 102};
const uint8_t AB[]      = {3, 16};
const uint8_t HALBI[]   = {36, 43, 56, 63, 76};

// Hours, index 0 = ZWÖLFI
const uint8_t HOUR_12[] = {9, 10, 29, 30, 49, 50};
const uint8_t HOUR_1[]  = {4, 15, 24};
const uint8_t HOUR_2[]  = {35, 44, 55, 64};
const uint8_t HOUR_3[]  = {84, 95, 104};
const uint8_t HOUR_4[]  = {5, 14, 25, 34, 45};
const uint8_t HOUR_5[]  = {54, 65, 74, 85};
const uint8_t HOUR_6[]  = {6, 13, 26, 33, 46, 53};
const uint8_t HOUR_7[]  = {66, 73, 86, 93, 106};
const uint8_t HOUR_8[]  = {7, 12, 27, 32, 47};
const uint8_t HOUR_9[]  = {52, 67, 72, 87};
const uint8_t HOUR_10[] = {8, 11, 28, 31};
const uint8_t HOUR_11[] = {71, 88, 91, 108};

// Minute dots for +1 to +4 minutes
const uint8_t DOTS_1[] = {112};
const uint8_t DOTS_2[] = {111, 112};
const uint8_t DOTS_3[] = {110, 111, 112};
const uint8_t DOTS_4[] = {110, 111, 112, 113};

// Symbols
const uint8_t WIFI[]   = {44, 45, 53, 54};
const uint8_t AUTO[]   = {1, 2, 3, 4, 17, 19, 20, 22, 37, 39, 41, 42, 43, 44};
const uint8_t MANUAL[] = {0, 1, 2, 3, 4, 18, 22, 38, 40, 41, 42, 43, 44};
const uint8_t BARS_1[] = {69};
const uint8_t BARS_2[] = {70, 71};
const uint8_t BARS_3[] = {87, 88, 89};
const uint8_t BARS_4[] = {90, 91, 92, 93};
const uint8_t BARS_5[] = {105, 106, 107, 108, 109};

template <size_t N>
void light(LightFn fn, const uint8_t (&leds)[N]) {
  for (uint8_t led : leds) {
    fn(led);
  }
}

void lightHour(int hour, LightFn fn) {  // hour 0-12
  switch (hour) {
    case 0:
    case 12: light(fn, HOUR_12); break;
    case 1:  light(fn, HOUR_1);  break;
    case 2:  light(fn, HOUR_2);  break;
    case 3:  light(fn, HOUR_3);  break;
    case 4:  light(fn, HOUR_4);  break;
    case 5:  light(fn, HOUR_5);  break;
    case 6:  light(fn, HOUR_6);  break;
    case 7:  light(fn, HOUR_7);  break;
    case 8:  light(fn, HOUR_8);  break;
    case 9:  light(fn, HOUR_9);  break;
    case 10: light(fn, HOUR_10); break;
    case 11: light(fn, HOUR_11); break;
  }
}

}  // namespace

void layoutTime(int hour, int minute, LightFn fn) {
  light(fn, ES_ISCH);

  int shownHour = hour % 12;
  if (minute > 24) {  // from "füf vor halbi" on, the next hour is shown
    shownHour++;
  }
  lightHour(shownHour, fn);

  switch (minute / 5) {  // words in 5-minute steps
    case 1:  light(fn, FUEF);    light(fn, AB);  break;
    case 2:  light(fn, ZAEH);    light(fn, AB);  break;
    case 3:  light(fn, VIERTU);  light(fn, AB);  break;
    case 4:  light(fn, ZWAENZG); light(fn, AB);  break;
    case 5:  light(fn, FUEF);    light(fn, VOR); light(fn, HALBI); break;
    case 6:  light(fn, HALBI);   break;
    case 7:  light(fn, HALBI);   light(fn, FUEF); light(fn, AB); break;
    case 8:  light(fn, ZWAENZG); light(fn, VOR); break;
    case 9:  light(fn, VIERTU);  light(fn, VOR); break;
    case 10: light(fn, ZAEH);    light(fn, VOR); break;
    case 11: light(fn, FUEF);    light(fn, VOR); break;
  }

  switch (minute % 5) {  // minute dots
    case 1: light(fn, DOTS_1); break;
    case 2: light(fn, DOTS_2); break;
    case 3: light(fn, DOTS_3); break;
    case 4: light(fn, DOTS_4); break;
  }
}

void layoutWifiSymbol(LightFn fn) {
  light(fn, WIFI);
}

void layoutAutoSymbol(LightFn fn) {
  light(fn, AUTO);
}

void layoutManualSymbol(LightFn fn) {
  light(fn, MANUAL);
}

void layoutLevelBars(int level, LightFn fn) {  // each level adds to the bars of the lower ones
  switch (level) {
    case 5: light(fn, BARS_5); [[fallthrough]];
    case 4: light(fn, BARS_4); [[fallthrough]];
    case 3: light(fn, BARS_3); [[fallthrough]];
    case 2: light(fn, BARS_2); [[fallthrough]];
    case 1: light(fn, BARS_1);
  }
}
