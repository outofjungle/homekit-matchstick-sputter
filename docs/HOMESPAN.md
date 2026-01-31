# HomeSpan Setup and Configuration Guide

## Overview

This project uses [HomeSpan](https://github.com/HomeSpan/HomeSpan) to create a native ESP32 HomeKit bridge with 4 independent light accessories. Each LED channel (GPIO 26, 18, 25, 19) appears as a separate LightBulb in the Apple Home app with full HSV color control.

## Architecture

```
HomeSpan Bridge: "Sputter Lights"
├── Bridge Accessory (Device Info)
├── Channel 1 (GPIO 26, 200 LEDs)
├── Channel 2 (GPIO 18, 200 LEDs)
├── Channel 3 (GPIO 25, 200 LEDs)
└── Channel 4 (GPIO 19, 200 LEDs)
```

Each channel supports:
- **Power**: On/Off
- **Hue**: 0-360 degrees
- **Saturation**: 0-100%
- **Brightness**: 0-100%

## Initial Setup

### 1. Build and Flash Firmware

```bash
make clean
make build
make flash
make monitor
```

### 2. WiFi Configuration

On first boot, the device has no WiFi credentials configured.

**Serial CLI Method:**

1. Open serial monitor (`make monitor`)
2. Press **`W`** to enter WiFi setup mode
3. Enter your SSID when prompted
4. Enter your WiFi password
5. Device saves credentials to NVS and reboots
6. After reboot, device connects to WiFi automatically

**Expected serial output:**
```
========================================
homekit-matchstick-sputter - Phase 2
HomeKit Integration - 4 Light Channels
========================================
FastLED initialized.
HomeSpan initialized.
Creating HomeKit accessories...
========================================
Setup complete!
Press 'W' in serial monitor to configure WiFi
After WiFi is connected, pair with HomeKit
========================================

*** HomeSpan Message: WiFi Connected
*** HomeSpan Message: IP Address: 192.168.1.XXX
*** HomeSpan Message: Setup Code: XXX-XX-XXX
*** HomeSpan Message: Scan QR Code or enter Setup Code in Home app
```

### 3. HomeKit Pairing

Once WiFi is connected:

1. **Open Apple Home app** on iPhone/iPad
2. Tap **"+"** → **Add Accessory**
3. Choose one of:
   - **Scan QR Code** (displayed in serial output)
   - **Enter Setup Code manually** (displayed in serial output)
4. If prompted about "Uncertified Accessory", tap **Add Anyway**
5. Device appears as **"Sputter Lights"**
6. Assign to a room and tap **Done**

### 4. Verify Channels

After pairing, you should see 4 light accessories:
- Channel 1
- Channel 2
- Channel 3
- Channel 4

Test each channel:
1. Toggle power on/off
2. Adjust brightness (0-100%)
3. Change color (hue/saturation)
4. Verify LED strips respond correctly

## HomeSpan Serial Commands

Press these keys in the serial monitor:

| Key | Command | Description |
|-----|---------|-------------|
| `W` | WiFi Setup | Configure WiFi credentials |
| `S` | Status | Show device status and pairing info |
| `A` | About | Show HomeSpan version and device info |
| `H` | Help | Display all available commands |
| `E` | Erase | Erase all HomeKit pairings (reset) |
| `R` | Reboot | Restart the device |

## Troubleshooting

### WiFi Issues

**Problem**: Device won't connect to WiFi
- Check SSID and password are correct (case-sensitive)
- Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Try moving device closer to router
- Press `W` to reconfigure WiFi

**Problem**: Device keeps disconnecting from WiFi
- Check WiFi signal strength
- Verify router DHCP settings
- Try assigning a static IP in router settings

### Pairing Issues

**Problem**: Home app can't find device
- Verify device is connected to WiFi (check serial output)
- Ensure iPhone/iPad is on the same network
- Try rebooting the device (press `R` in serial monitor)
- Check setup code matches serial output

**Problem**: Pairing fails or times out
- Press `E` in serial monitor to erase pairings
- Reboot device (press `R`)
- Try pairing again with fresh setup code

**Problem**: "Unable to Add Accessory" error
- Erase pairings: Press `E` in serial monitor
- Restart Home app completely
- Reboot iPhone/iPad
- Try pairing again

### LED Issues

**Problem**: Wrong colors displayed
- Check color order in main.cpp:24-27 (currently `GRB`)
- Try changing to `RGB` if colors are swapped
- Verify LED type matches code (`WS2811`)

**Problem**: LEDs not responding to HomeKit commands
- Check serial output for "Channel updated" messages
- Verify FastLED.show() is being called (should be in loop)
- Try toggling power off/on in Home app
- Check physical LED connections

**Problem**: Dim or flickering LEDs
- Check power supply can handle 4x 200 LED strips
- Verify FastLED brightness is set correctly (currently 25% = 64/255)
- Ensure good electrical connections

## Advanced Configuration

### Changing Device Name

Edit `src/config.h`:
```cpp
constexpr const char* DEVICE_NAME = "Your Custom Name";
```

### Changing Channel Names

Edit `src/main.cpp` (lines 61, 68, 75, 82):
```cpp
new Characteristic::Name("Your Channel Name");
```

### Adjusting Brightness Limit

Edit `src/main.cpp:30`:
```cpp
FastLED.setBrightness(64);  // 0-255 (64 = 25%)
```

### Resetting to Factory Defaults

1. Press `E` in serial monitor (erases HomeKit pairings)
2. Press `W` to reconfigure WiFi
3. Pair with Home app again

## Security Notes

- HomeKit communication is encrypted (HAP protocol)
- Setup code is randomly generated on first boot
- Only paired iOS devices can control the lights
- Pairing data is stored in ESP32 NVS (persists across reboots)
- To fully reset: erase pairings (`E`) and reflash firmware

## Performance

- **Response time**: ~100-300ms from Home app command to LED update
- **WiFi range**: Depends on ESP32 antenna and router
- **Max concurrent connections**: 8 HomeKit controllers (iOS limitation)
- **LED update rate**: ~60-100 FPS (FastLED + HomeSpan polling)

## Further Reading

- [HomeSpan Documentation](https://github.com/HomeSpan/HomeSpan/blob/master/docs/README.md)
- [HomeSpan API Reference](https://github.com/HomeSpan/HomeSpan/blob/master/docs/Reference.md)
- [Apple HomeKit Accessory Protocol](https://developer.apple.com/homekit/)
- [FastLED Documentation](https://fastled.io/)

## Support

For issues specific to this project:
- Check serial output for error messages
- Review this troubleshooting guide
- Open an issue on the project repository

For HomeSpan-specific issues:
- [HomeSpan GitHub Issues](https://github.com/HomeSpan/HomeSpan/issues)
- [HomeSpan Community Forum](https://github.com/HomeSpan/HomeSpan/discussions)
