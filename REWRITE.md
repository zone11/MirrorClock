# Rewrite plan

Branch `rewrite` (from `dev`). Goal: a cleanly structured firmware with a web UI, while the clock looks and behaves like the original.

## Decisions

| Topic | Decision |
|---|---|
| Language | Everything in English (names, comments, logs, web UI). Only the words lit on the clock stay Swiss German. |
| Web UI | `http://mirrorclock.local`, no password. Enable/disable IR, NTP server, time zone, and every setting the remote can change. |
| Time zone | Selectable from a few presets (stored as an index, POSIX TZ strings from the core). |
| OTA | ArduinoOTA, hostname `mirrorclock`, no password. Separate `ota` env in `platformio.ini`. |
| Storage | One settings struct with magic, version and checksum in the EEPROM emulation. No filesystem, no JSON library. |
| Time | Core `configTime()` (SNTP in the background, DST via TZ string) replaces Time, Timezone and the hand-written NTP code. |
| Behaviour | Same words, colors, brightness curve, remote codes and menus as the original. |

## Architecture

```
src/main.cpp        setup() / loop() only
src/config.h        pins, LED count
src/layout.*        time and symbols -> LED indices (pure C++, unit-tested on the host)
src/keymap.*        IR code -> key number (pure C++, unit-tested on the host)
src/settings.*      persistent settings: load, save, defaults
src/display.*       LED strip, colors, rainbow, drawing time and symbols
src/brightness.*    BH1750 light sensor, auto/manual luminance
src/controls.*      key handling and the brightness menu (shared by IR and web)
src/remote.*        IR receiver, can be disabled
src/timesync.*      NTP and local time
src/web.*           web server, JSON API, HTML page
src/ota.*           ArduinoOTA
test/               host tests (`pio test -e native`)
```

Key numbers are the same everywhere (IR, web, tests): `0-9` color presets, `11` `*` color cycle, `12` `#` rainbow, `13` up, `14` left (auto brightness), `15` OK (freeze cycle color), `16` right (manual brightness), `17` down.

Time zone presets:

| Index | Zone |
|---|---|
| 0 (default) | Europe/Zurich |
| 1 | Europe/London |
| 2 | Europe/Helsinki |
| 3 | UTC |
| 4 | America/New_York |

## Phases

Each phase is one commit, built, flashed and checked on the clock.

- [x] **1. Same behaviour, new structure.** Split into the modules above, everything in English. Host tests compare all 1440 minutes, all symbols and all IR codes against the original firmware's output (golden data). Settings stay in the old EEPROM layout so nothing is lost. `WEB_TASTE` becomes `WEB_KEYS` (`/key?nr=`) until phase 5 replaces it.
- [ ] **2. OTA.** ArduinoOTA plus an `ota` env, so later phases can be flashed over WiFi. LEDs show progress; IR and web pause during the update.
- [ ] **3. Time via `configTime()`.** Remove Time, Timezone and the NTP code. Time zone presets (still a compile-time default here).
- [ ] **4. Settings struct.** Magic, version, checksum; add `irEnabled`, `ntpServer`, `timezone`. One-time migration from the old layout (offsets 0/4/8/12). Central setters used by IR and web.
- [ ] **5. Web server and JSON API.** `GET /api/status`, `POST /api/settings`, `POST /api/key`. Tested with `curl`. Replaces `WEB_KEYS`.
- [ ] **6. Web UI page.** Single HTML page in `PROGMEM`: color presets, rainbow/cycle/freeze, auto/manual with level, IR switch, NTP server, time zone, status. Polls `/api/status` so remote changes show up.
- [ ] **7. Docs.** Rewrite `DEVELOPMENT.md` and `README.md` for the new structure.

## Risks

- RAM: about 34 KB of 80 KB used before the rewrite; web server and page are expected to add 5-10 KB. Measure after phase 5.
- The HTML page must live in flash (`PROGMEM`), not RAM.
- WiFiManager 0.16 uses port 80 only during the first-time setup portal, before our server starts.
- Flash wear: settings are written only when a value changes.
