#pragma once

#include <Arduino.h>
#include <Preferences.h>

// NVS storage for LED channel state
// Stores HSV values and power state for each channel
class ChannelStorage {
private:
    Preferences prefs;
    char namespace_name[16];  // e.g., "channel1", "channel2", etc.

public:
    struct ChannelState {
        bool power;
        int hue;           // 0-360
        int saturation;    // 0-100
        int brightness;    // 0-100
    };

    ChannelStorage(int channelNumber) {
        snprintf(namespace_name, sizeof(namespace_name), "channel%d", channelNumber);
    }

    // Load channel state from NVS
    // Returns true if state was loaded, false if no saved state exists
    bool load(ChannelState& state) {
        if (!prefs.begin(namespace_name, true)) {  // true = read-only
            return false;
        }

        // Check if any data exists (check for a known key)
        if (!prefs.isKey("power")) {
            prefs.end();
            return false;
        }

        state.power = prefs.getBool("power", false);
        state.hue = prefs.getInt("hue", 0);
        state.saturation = prefs.getInt("sat", 100);
        state.brightness = prefs.getInt("bri", 100);

        prefs.end();
        return true;
    }

    // Save channel state to NVS
    void save(const ChannelState& state) {
        if (!prefs.begin(namespace_name, false)) {  // false = read-write
            Serial.printf("Failed to open NVS namespace: %s\n", namespace_name);
            return;
        }

        prefs.putBool("power", state.power);
        prefs.putInt("hue", state.hue);
        prefs.putInt("sat", state.saturation);
        prefs.putInt("bri", state.brightness);

        prefs.end();
    }

    // Clear channel state from NVS
    void clear() {
        if (prefs.begin(namespace_name, false)) {
            prefs.clear();
            prefs.end();
        }
    }
};
