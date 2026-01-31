# M5Stack Stamp Pico Hardware Reference

## Board Overview

**Model**: M5Stack Stamp Pico
**MCU**: ESP32-PICO-D4 (WiFi only, no Thread/BLE mesh)
**Form Factor**: Ultra-compact stamp module with castellated edges

## Key Specifications

- **WiFi**: 2.4GHz 802.11 b/g/n (no Thread/Bluetooth mesh)
- **Flash**: 4MB
- **PSRAM**: None in base PICO-D4
- **Operating Voltage**: 3.3V
- **Dimensions**: 25.4mm × 20.5mm

## GPIO Assignments

### Built-in Components

| Pin Name | GPIO | Type | Notes |
|----------|------|------|-------|
| PIN_LED_CH0 | GPIO27 | SK6812 RGB (1 pixel) | Status indicator - Channel 0 (special single-pixel RGB channel) |
| PIN_STATUS_LED | GPIO22 | Single-color | Single-color status LEDs |
| Button | GPIO39 | Input | User button (input only pin) |

### External LED Strip Channels (Future Use)

| Pin Name | GPIO | Purpose | Max LEDs |
|----------|------|---------|----------|
| PIN_LED_CH1 | GPIO26 | WS2811 Strip | 200 |
| PIN_LED_CH2 | GPIO18 | WS2811 Strip | 200 |
| PIN_LED_CH3 | GPIO25 | WS2811 Strip | 200 |
| PIN_LED_CH4 | GPIO19 | WS2811 Strip | 200 |

## PIN_LED_CH0 - Status RGB LED (GPIO27)

### Hardware Details
- **Pin Name**: `PIN_LED_CH0` (Channel 0 - special single-pixel RGB channel)
- **LED Model**: SK6812 (WS2812-compatible timing)
- **Quantity**: 1 LED
- **Protocol**: One-wire addressable RGB
- **Voltage**: 3.3V logic level
- **Color Order**: GRB (Green-Red-Blue)

### Driver Implementation
Use ESP-IDF's `led_strip` component with RMT backend:
- **Component**: `led_strip.h`
- **LED Model**: `LED_MODEL_WS2812` (SK6812 compatible)
- **Pixel Format**: `LED_PIXEL_FORMAT_GRB`
- **RMT Clock**: 10MHz resolution

### Status Color Mapping

| Color | RGB | Matter State |
|-------|-----|--------------|
| Red | (255, 0, 0) | Not commissioned / Error |
| Blue | (0, 0, 255) | BLE pairing mode |
| Green | (0, 255, 0) | Connected / Operating normally |
| Yellow | (255, 255, 0) | Updating / Busy |
| Off | (0, 0, 0) | Deep sleep / Idle |

## Matter Compatibility

### RMT Usage
- **Status LED**: Uses RMT channel for SK6812 control (minimal overhead)
- **Matter Stack**: WiFi-only configuration, no Thread radio conflicts
- **No Conflicts**: ESP32-PICO-D4 WiFi implementation doesn't conflict with RMT peripheral

### Memory Considerations
- Status LED: Minimal memory (1 LED = 3 bytes RGB buffer)
- Matter stack requires ~200KB RAM
- 4-channel LED strips (future): ~2.4KB RAM (4 × 200 LEDs × 3 bytes)

## Power Requirements

- **Status LED**: ~60mA max (all colors at full brightness)
- **Per WS2811 Strip**: ~12A max at full white (200 LEDs × 60mA)
- **Recommended**: External 5V power supply for LED strips
- **MCU**: Can be powered via USB-C or 3.3V pin

## Pin Constraints

### Input-Only Pins
- GPIO39: Button (no output capability)

### Strapping Pins (use with caution)
- GPIO0: Boot mode (pulled up internally)
- GPIO2: Boot mode (pulled down internally)

### Reserved Pins
- GPIO1, GPIO3: UART0 (serial console)
- GPIO6-11: Flash SPI (DO NOT USE)

## References

- [M5Stack Stamp Pico Product Page](https://shop.m5stack.com/products/m5stamp-pico-mate-with-pin-headers)
- [M5Stack Documentation](https://docs.m5stack.com/en/core/stamp_pico)
- [ESP32-PICO-D4 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf)
- [SK6812 LED Datasheet](https://cdn-shop.adafruit.com/product-files/1138/SK6812+LED+datasheet+.pdf)
