# Hardware Configuration

## Board Specifications

- **Board:** M5Stack Stamp Pico
- **MCU:** ESP32 PICO32
- **Flash:** 4MB
- **WiFi:** 2.4 GHz 802.11 b/g/n
- **Bluetooth:** BLE 4.2 (for Matter commissioning)

## LED Strip Configuration

### Onboard Status LEDs

The M5Stack Stamp Pico includes built-in status LEDs for Matter state indication:
- **SK6812 RGB LED (GPIO 27):** Primary status indicator showing Matter connection state
- **Single-color LED (GPIO 22):** Secondary status indicator

These are separate from the external LED strip channels and are used for debugging and status display.

### WS2811 LED Strips

- **Type:** WS2811 (compatible with WS2812)
- **Color Order:** GRB
- **LEDs per Channel:** 200
- **Total LEDs:** 800 (4 channels × 200 LEDs)
- **Control Protocol:** Single-wire serial (via RMT peripheral)

### GPIO Pin Mapping

#### Status LEDs (Matter State Indication)

| Pin Name | GPIO Pin | Description |
|----------|----------|-------------|
| PIN_LED_CH0 | GPIO 27 | SK6812 RGB (1 pixel) - Channel 0 status indicator |
| PIN_STATUS_LED | GPIO 22 | Single-color LED for status indication |

#### External LED Strip Channels

| Pin Name | GPIO Pin | Description |
|----------|----------|-------------|
| PIN_LED_CH1 | GPIO 26 | LED Strip Channel 1 Data (200 LEDs) |
| PIN_LED_CH2 | GPIO 18 | LED Strip Channel 2 Data (200 LEDs) |
| PIN_LED_CH3 | GPIO 25 | LED Strip Channel 3 Data (200 LEDs) |
| PIN_LED_CH4 | GPIO 19 | LED Strip Channel 4 Data (200 LEDs) |

## Power Considerations

### Power Calculation

Each WS2811 LED at full white (255, 255, 255):
- Current draw: ~60mA per LED
- 200 LEDs per channel: 200 × 60mA = 12A
- 4 channels at full brightness: 4 × 12A = **48A total**

### Power Supply Requirements

**WARNING:** External power supply required!

- **Minimum Rating:** 5V @ 50A
- **Recommended:** 5V @ 60A (for safety margin)
- **DO NOT** power from USB or ESP32 regulators

### Power Management

Current firmware (Phase 1) does not implement:
- Current limiting
- Soft start
- Power monitoring

**Future Phases:**
- Add configurable brightness limiting
- Implement current monitoring
- Add safety shutoffs

## Wiring Diagram

```
External 5V PSU
    |
    +---[GND]-----------> ESP32 GND
    |
    +---[5V]------------> LED Strip Common VCC
                          (All 4 channels)

M5Stack Stamp Pico (ESP32 PICO32)
    GPIO27 -----------> SK6812 RGB Status LED (1 pixel)
    GPIO22 -----------> Single-color Status LED
    GPIO26 -----------> LED Strip 1 Data
    GPIO18 -----------> LED Strip 2 Data
    GPIO25 -----------> LED Strip 3 Data
    GPIO19 -----------> LED Strip 4 Data
    GND    -----------> LED Strip Common GND
```

## Safety Notes

1. **Never run all channels at full white simultaneously** without adequate power supply
2. **Use thick gauge wires** (16-18 AWG) for power distribution
3. **Connect ground first** before power when wiring
4. **Add inline fuses** (15A per channel recommended)
5. **Ensure proper heatsinking** for power supply

## Testing Procedure

### Initial Power-On Test

1. Connect ESP32 to computer via USB (for programming only)
2. Connect external 5V power supply to LED strips
3. Ensure all grounds are common
4. Power on external supply first, then USB
5. Test one channel at a time initially

### Commissioning Test

1. Flash firmware to ESP32
2. Monitor serial output for QR code
3. Commission device via Apple Home or chip-tool
4. Test each channel independently
5. Verify proper on/off control

## Reference Design

This hardware configuration is based on the `matchstick2-4ch-holiday` project:
- Source: `/Users/ven/Workspace/matchstick2-4ch-holiday/src/config.h`
- Proven design with 4 channels × 200 LEDs
- Same GPIO pin assignments
