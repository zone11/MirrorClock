# Development Guide

Technical notes for working on the MirrorClock firmware. For user-facing setup (WiFi portal, remote keys) see [README.md](README.md).

## Overview

A Swiss-German word clock ("ES ISCH FÜF AB DRÜ") behind a mirror. An ESP8266 gets the time via NTP, converts it to CET/CEST and lights words on a 114-LED WS2812B strip. Brightness follows a BH1750 light sensor or one of 5 manual levels. An IR remote changes colors and brightness mode.

All firmware lives in a single file: `src/main.cpp` (~1300 lines, ported from a multi-tab Arduino sketch; the `/***( xyz.ino )***/` banners mark the original tabs).

## Toolchain

| Item | Value |
|---|---|
| Build system | PlatformIO (`platformio.ini`, env `nodemcuv2`) |
| Platform / board | `espressif8266` / `nodemcuv2` (ESP-12E, 4 MB flash) |
| Framework | Arduino |
| Build flags | `-DPIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY -DVTABLES_IN_FLASH` |
| Upload | 57600 baud, `nodemcu` reset method (`upload_port` commented out, auto-detect) |
| Serial monitor | 115200 baud |
| Footprint (current) | RAM 42.4 % (34.8 KB), Flash 36.4 % (380 KB) |

```sh
pio run                 # build
pio run -t upload       # flash
pio device monitor      # serial log
pio run -t clean        # clean build
```

The build currently produces one warning: `NeoPixelBrightnessBus` is deprecated (see [Known issues](#known-issues)).

## Libraries

Libraries are pulled by PlatformIO from `lib_deps` into `.pio/libdeps/nodemcuv2/`. Resolved versions:

| Library | Version | Used for |
|---|---|---|
| tzapu/WiFiManager | 0.16.0 | Captive portal "MirrorClock" on first boot |
| paulstoffregen/Time | 1.6.1 | System clock, `setSyncProvider` |
| jchristensen/Timezone | 1.2.6 | UTC → CET/CEST |
| makuna/NeoPixelBus | 2.8.4 | LED output (I2S DMA) |
| crankyoldgit/IRremoteESP8266 | 2.8.6 | IR receiver (NEC codes) |
| claws/BH1750 | 1.3.0 | Ambient light (I2C) |

**The `lib/` folder is not compiled.** It contains older vendored copies (NeoPixelBus 2.2.9, IRremote 2.4.0, WiFiManager 0.12, …) from the original Arduino project. The PlatformIO dependency graph and include paths only reference `.pio/libdeps`. Don't edit anything in `lib/` expecting an effect.

## Hardware and pins

| Function | GPIO | NodeMCU label | Notes |
|---|---|---|---|
| LED data (WS2812B) | **GPIO3** | RX | Forced by the I2S DMA method, see below |
| IR receiver | GPIO4 | D2 | `RECV_PIN = 4` |
| I2C SDA (BH1750) | GPIO2 | D4 | Boot strap pin, must be high at boot (I2C pull-up does this) |
| I2C SCL (BH1750) | GPIO0 | D3 | Boot strap pin, same as above |

`PixelPin = 2` in the code is **ignored**. On ESP8266, `Neo800KbpsMethod` resolves to `NeoEsp8266DmaWs2812xMethod`, which always drives GPIO3 (RX) through the I2S peripheral. Consequences:

- Serial RX cannot be used (TX/logging works).
- `Show()` runs via DMA and does not block interrupts, so IR reception isn't disturbed by LED updates.
- If you change the method (e.g. to `NeoEsp8266Uart1Ws2812xMethod` for GPIO2), the strip moves to a different pin, which collides with I2C SDA on this board.

## LED layout

114 LEDs: an 11 × 10 letter grid (indices 0–109) plus 4 minute dots (110–113).

The grid is wired column by column in a serpentine:

```
index(col, row) = col even ? col*10 + row : col*10 + (9 - row)     // col 0..10, row 0..9 (top = 0)
```

Example: "ES ISCH" = row 0, columns 0,1,3,4,5,6 → LEDs 0, 19, 39, 40, 59, 60.

Minute dots: 112 (+1), 111–112 (+2), 110–112 (+3), 110–113 (+4).

Word → LED mapping is hard-coded in the `woerter.ino` section (`itis`, `m5`, `m10`, `m15`, `m20`, `before`, `after`, `half`, `houres`). Menu glyphs (`sym_aa`, `sym_mm`, `sym_stufe`, `sym_showwifi`) are in the `symbole.ino` section.

## Time-to-words logic

Swiss-German style, 5-minute resolution plus dots:

| Minutes | Words |
|---|---|
| 0 | `<hour>` |
| 5 / 10 / 15 / 20 | FÜF / ZÄH / VIERTU / ZWÄNZG **AB** `<hour>` |
| 25 | FÜF **VOR** HALBI `<hour+1>` |
| 30 | HALBI `<hour+1>` |
| 35 | FÜF **AB** HALBI `<hour+1>` |
| 40 / 45 / 50 / 55 | ZWÄNZG / VIERTU / ZÄH / FÜF **VOR** `<hour+1>` |

`m % 5` lights the minute dots. `houres()` converts to 12 h and adds 1 when `m > 24`; hour 0 and 12 both show ZWÖLFI.

## Runtime flow

**`setup()`**

1. `strip.Begin()`, `pixeltest()`: red, green, blue chase over all 114 LEDs (~17 s).
2. Show the WiFi glyph (LEDs 44, 45, 53, 54).
3. `WiFiManager.autoConnect("MirrorClock")`: blocks until connected. With no saved credentials it opens the AP + config portal.
4. UDP on port 8888, `setSyncProvider(getNtpTime)`. `getNtpTime()` sets the resync interval itself: 86400 s after success, 60 s after a failure.
5. Start IR receiver, `Wire.begin(D4, D3)`, `lightMeter.begin()`.

**`loop()`** (about every 20 ms)

1. If time is set: `lokaleZeit()` (UTC → local into globals `h`, `m`, …), then `wort()` redraws all words and calls `Show()`.
2. `helligkeit()`: reads the BH1750 once per second (auto mode only) and sets the brightness.
3. Leaves the menu after 6 s without a key press (`menuinterval`).
4. Decodes IR. `0xFFFFFFFF` (NEC repeat) replays the last key.

**NTP**: `getNtpTime()` resolves `ch.pool.ntp.org`, sends a 48-byte request, waits up to 1.5 s. Returns UTC (the `timeZone` constant must stay 0 because Timezone does the conversion).

## State and modes

All state is in globals at the top of `main.cpp` (German names):

| Variable | Meaning |
|---|---|
| `aktivcolor` | 0–9 preset hue, 77 = rainbow across pixels, 88 = whole-face color cycle, 99 = frozen cycle color |
| `regenbogenfarbe` | Current hue 0–359 for modes 77/88/99, +1 every `regenbogeninterval` (100 ms) |
| `hellmodus` | 0 = auto (sensor), 1 = manual |
| `luxstufe` / `luxkorr` | Manual level 1–5 → brightness 30/60/100/150/255 |
| `aktivmenu` | 1 while a brightness menu glyph is shown (suppresses the clock face) |

Auto brightness: the reading is clamped to 0–10 lux, then `map(…, 0, 10, 40, 255)`. Sensor errors (negative values) are ignored and the last good reading is kept. Anything above 10 lux is full brightness.

Colors are HSL with saturation 1.0 and lightness 0.25; preset 5 is RGB(200,200,200).

## IR remote

`translateIR()` accepts two NEC remotes (codes `0xFD....` and `0xFF....`) and maps them to a key number `taste`; `tastecheck()` acts on it.

| Key | `taste` | Action |
|---|---|---|
| 0–9 | 0–9 | Color preset |
| * | 11 | Color cycle (88) |
| # | 12 | Rainbow (77) |
| Up / Down | 13 / 17 | Brighter / darker (only while manual menu is open) |
| Left | 14 | Auto brightness, shows "A" |
| Right | 16 | Manual brightness, shows "M" + level bars |
| OK | 15 | Freeze color cycle (88 → 99) |

To add a remote: log `results.value` over serial and add `case` lines in `translateIR()`.

## Configuration points (compile time)

| What | Where |
|---|---|
| NTP server | `ntpServerName` |
| Time zone / DST | `CEST`, `CET` rules |
| Reset WiFi credentials | uncomment `wifiManager.resetSettings();` in `setup()`, flash, run once, revert |
| IR pin | `RECV_PIN` |
| Manual brightness levels | `setlux()` |
| Rainbow speed | `regenbogeninterval` |

Nothing is persisted: color and brightness mode reset to defaults (color 1, auto) on every reboot.

## Known issues

- **Deprecation warning** for `NeoPixelBrightnessBus`. The replacement `NeoPixelBusLg` exists in the used 2.8.4 but uses `SetLuminance()` instead of `SetBrightness()` and applies gamma, so it's not a drop-in rename.
- `README.md` lists wrong pins (IR is D2, LED data is RX/GPIO3) and says the `lib/` folder is used.

## Repo layout

```
src/main.cpp        firmware (only source file)
platformio.ini      build config and dependencies
lib/                unused vendored libraries (legacy)
workfiles/          personal notes/zips, git-ignored
.pio/               build output and downloaded libs, git-ignored
```

No unit tests, no CI.
