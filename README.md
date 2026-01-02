# MirrorClock - PlatformIO Project

This project has been transferred from Arduino IDE to PlatformIO.
Some minor improvements were made to the code.

## Original Project
https://github.com/bastelgarage/MirrorClock

## Hardware

- **Mirror Clock**: Discontinued product by bastelgarage.ch (Due to legal issues)
- **Board**: ESP8266 (NodeMCU v2 or compatible)
- **LED Strip**: 114 WS2812B RGB LEDs (NeoPixels)
- **Light Sensor**: BH1750 (I2C)

### Pin Configuration

- **IR Receiver**: GPIO4 (D4)
- **NeoPixel Data**: GPIO2
- **I2C SDA**: D4 (GPIO2)
- **I2C SCL**: D3 (GPIO0)

## Libraries

The following libraries are included in the `lib` folder:

- **WiFiManager** - Easy WiFi configuration via web portal
- **Time** - Time handling library
- **Timezone** - Timezone and DST handling
- **NeoPixelBus** - High-performance NeoPixel LED control
- **IRremoteESP8266** - IR remote control receiver
- **BH1750** - Light sensor for automatic brightness control

## Features

- **Word Clock Display**: Displays time using illuminated words
- **Multiple Color Modes**: 10 preset colors plus rainbow modes
- **IR Remote Control**: Change colors and settings via remote
- **Automatic Brightness**: Uses BH1750 light sensor
- **Manual Brightness**: 5 brightness levels
- **WiFi Configuration**: Uses WiFiManager for easy setup
- **NTP Time Sync**: Automatically syncs time from internet
- **Timezone Support**: Configured for Central European Time (CET/CEST)

## WiFi Setup

On first boot, the device creates a WiFi access point named "MirrorClock":

1. Connect to "MirrorClock" WiFi network
2. A configuration portal opens automatically
3. Select your WiFi network and enter password
4. Device will connect and remember settings

## IR Remote Control

The project supports standard IR remotes with number keys:

- **0-9**: Select color presets
- **\***: RGB all (cycling)
- **#**: Rainbow mode
- **Up**: Increase brightness
- **Down**: Decrease brightness
- **Left**: Auto brightness mode
- **Right**: Manual brightness mode
- **OK**: Freeze current rainbow color

## Configuration

### Change WiFi Settings

To reset WiFi settings and reconfigure:

1. Uncomment line in `main.cpp`: `//wifiManager.resetSettings();`
2. Upload and run once
3. Comment the line again and re-upload

### Change Timezone

Edit timezone rules in `main.cpp`:

```cpp
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
```

### Change NTP Server

Edit in `main.cpp`:

```cpp
static const char ntpServerName[] = "ch.pool.ntp.org";
```