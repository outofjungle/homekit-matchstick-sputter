#pragma once

// GPIO Pin Definitions - M5Stack Stamp Pico
// Based on docs/HARDWARE.md

// Status LEDs (Matter State Indication)
constexpr uint8_t PIN_STATUS_SK6812 = 27;  // SK6812 RGB LED (1 pixel) - Channel 0 status indicator
constexpr uint8_t PIN_STATUS_LED = 22;     // Single-color LED for status indication

// External LED Strip Channels (WS2811, 200 LEDs each)
constexpr uint8_t PIN_LED_CH1 = 26;        // LED Strip Channel 1 Data
constexpr uint8_t PIN_LED_CH2 = 18;        // LED Strip Channel 2 Data
constexpr uint8_t PIN_LED_CH3 = 25;        // LED Strip Channel 3 Data
constexpr uint8_t PIN_LED_CH4 = 19;        // LED Strip Channel 4 Data

// Button Input
constexpr uint8_t PIN_BUTTON = 39;         // Button input (GPIO39 - input only)

// LED Configuration
constexpr uint8_t NUM_LEDS_PER_CHANNEL = 200;  // 200 LEDs per channel

// Blink Configuration
constexpr unsigned long BLINK_INTERVAL_MS = 500;  // 500ms on, 500ms off = 1Hz blink
constexpr unsigned long DEBOUNCE_MS = 50;        // Button debounce time

// HomeSpan Configuration
constexpr const char* DEVICE_NAME = "Sputter Lights";
constexpr const char* DEVICE_MANUFACTURER = "0x76656E Labs";
constexpr const char* DEVICE_SERIAL = "SPT-001";
constexpr const char* DEVICE_MODEL = "4CH-LED";
constexpr const char* DEVICE_FIRMWARE = "1.0.0";
