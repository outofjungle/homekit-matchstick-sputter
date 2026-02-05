#pragma once

// GPIO Pin Definitions - M5Stack Stamp Pico
// Based on docs/HARDWARE.md

// Status LEDs (Matter State Indication)
constexpr uint8_t PIN_STATUS_SK6812 = 27;  // SK6812 RGB LED (1 pixel) - Channel 0 status indicator
constexpr uint8_t PIN_STATUS_LED = 22;     // Single-color LED for status indication

// External LED Strip Channels (WS2811, 200 LEDs each)
constexpr uint8_t PIN_LED_CH1 = 26;        // LED Strip Channel 1 Data
constexpr uint8_t PIN_LED_CH2 = 18;        // LED Strip Channel 2 Data
constexpr uint8_t PIN_LED_CH3 = 19;        // LED Strip Channel 3 Data
constexpr uint8_t PIN_LED_CH4 = 25;        // LED Strip Channel 4 Data

// Button Input
constexpr uint8_t PIN_BUTTON = 39;         // Button input (GPIO39 - input only) - Factory reset
constexpr uint8_t PIN_BUTTON_ANIM = 0;     // Button input (GPIO0) - Animation cycling

// LED Configuration
constexpr uint8_t NUM_LEDS_PER_CHANNEL = 200;  // 200 LEDs per channel

// Blink Configuration
constexpr unsigned long BLINK_INTERVAL_MS = 500;  // 500ms on, 500ms off = 1Hz blink
constexpr unsigned long DEBOUNCE_MS = 50;        // Button debounce time

// Factory Reset Configuration
constexpr unsigned long FACTORY_RESET_WARNING_MS = 5000;   // 5 seconds - trigger warning animation (3x cycles)
constexpr unsigned long FACTORY_RESET_CONFIRM_MS = 3000;   // 3 seconds - green confirmation display (cancel)
// Note: Animation duration is 3 cycles × 8 LEDs × 300ms = ~7.2 seconds
// Total time if held: 5s (hold) + 7.2s (animation) = ~12.2s to trigger reset

// Animation Button Configuration
constexpr unsigned long ANIM_BUTTON_LONG_PRESS_MS = 2000;  // 2 seconds - long press for reset

// HomeSpan Configuration
constexpr const char* DEVICE_NAME = "Sputter Lights";
constexpr const char* DEVICE_MANUFACTURER = "0x76656E Labs";
constexpr const char* DEVICE_SERIAL = "SPT-001";
constexpr const char* DEVICE_MODEL = "4CH-LED";
constexpr const char* DEVICE_FIRMWARE = "1.0.0";

// Channel Defaults
constexpr int NUM_CHANNELS = 4;
constexpr int DEFAULT_BRIGHTNESS = 80;    // Minimum visible brightness (was MIN_BRIGHTNESS)
constexpr int DEFAULT_SATURATION = 100;   // Full color saturation

// Default hue per channel (90° spacing around color wheel)
inline int getDefaultHue(int channelNum) {
    switch (channelNum) {
        case 1: return 0;    // Red
        case 2: return 90;   // Yellow/Orange
        case 3: return 180;  // Cyan
        case 4: return 270;  // Purple/Magenta
        default: return 0;
    }
}
