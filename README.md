# Matchstick LED Controller

A 4-channel HomeKit LED controller based on the M5Stack Stamp Pico (ESP32). Controls up to 800 WS2811 LEDs across 4 independent channels, each fully controllable from Apple Home or Siri. Includes 34 ambient animation modes.

---

## Pairing QR Code

Scan this code in the Apple Home app to pair the device:

![Pairing QR Code](docs/img/pairing_qr.png)

---

## Setup

### WiFi Configuration

The device ships without WiFi credentials. To configure:

1. **Hold** the reset button (GPIO39) for **3 seconds**
2. LEDs turn **solid purple** — keep holding or release
3. **Release** at 3 seconds to enter AP mode
4. Connect your phone to WiFi network **"Matchstick-Setup"** (open, no password)
5. A captive portal opens — enter your home WiFi SSID and password
6. The device saves credentials and reconnects automatically

### HomeKit Pairing

Once the device is connected to WiFi:

1. Open the **Apple Home** app on your iPhone or iPad
2. Tap **+** → **Add Accessory**
3. Scan the QR code above
4. If prompted about an "Uncertified Accessory", tap **Add Anyway**
5. The device appears as **"Matchstick 0x02"** — a bridge with 4 lights
6. Assign lights to rooms and tap **Done**

You will see 4 light accessories: **Sputter One**, **Sputter Two**, **Sputter Three**, **Sputter Four**.

---

## Using the Device

### HomeKit Controls

Each of the 4 channels can be independently controlled from Apple Home or Siri:

- **Power** — on/off per channel
- **Brightness** — 0–100% (also affects animation density in some modes)
- **Color** — full hue/saturation control
- **Siri** — "Hey Siri, turn on Sputter One" / "Set Sputter Two to blue"

### Animation Button (GPIO0)

The animation button cycles through 34 animation modes independently of HomeKit:

| Press | Action |
|-------|--------|
| **Short press** | Cycle to next animation mode |
| **Long press (2s)** | Toggle between normal and inverted variants of the current mode |

Inverted modes use a dark sparkle base (most LEDs black) instead of the bright base layer.

Animation mode is saved to flash and restored on reboot.

### Reset Button (GPIO39)

| Hold Duration | Action |
|---------------|--------|
| Quick press (<3s) | Restore HomeKit channel defaults (brightness, color) |
| 3 seconds | Enter WiFi AP mode (solid purple LEDs → release) |
| 10+ seconds | Trigger factory reset warning animation (see below) |

---

## Animation Modes

The device cycles through 34 modes in order. Short-press the animation button to advance.

### HomeKit Mode

| # | Mode | Description |
|---|------|-------------|
| 0 | **HomeKit** | Normal HomeKit-controlled operation. No animation. |

### Inverted Animations (dark sparkle base)

These modes use a dark background: 40 of 200 LEDs glow at any moment, each fading in and out with a smooth sine hump (~2–6s per pulse).

| # | Mode | Description |
|---|------|-------------|
| 1 | **Inverted Base** | Pure dark sparkle — no overlay |
| 2 | **Inv. Monochromatic Runner** | Single-color moving blob on dark sparkle |
| 3 | **Inv. Monochromatic Rain** | Single-color fade-in-place bloom on dark sparkle |
| 4 | **Inv. Monochromatic Twinkle** | Single-color twinkle cycle on dark sparkle |
| 5 | **Inv. Complementary Runner** | 2-color (opposite hues) runner on dark sparkle |
| 6 | **Inv. Complementary Rain** | 2-color rain on dark sparkle |
| 7 | **Inv. Complementary Twinkle** | 2-color twinkle on dark sparkle |
| 8 | **Inv. Split-Comp Runner** | 3-color split-complementary runner on dark sparkle |
| 9 | **Inv. Split-Comp Rain** | 3-color split-complementary rain on dark sparkle |
| 10 | **Inv. Split-Comp Twinkle** | 3-color split-complementary twinkle on dark sparkle |
| 11 | **Inv. Triadic Runner** | 3-color triadic runner on dark sparkle |
| 12 | **Inv. Triadic Rain** | 3-color triadic rain on dark sparkle |
| 13 | **Inv. Triadic Twinkle** | 3-color triadic twinkle on dark sparkle |
| 14 | **Inv. Square Runner** | 4-color square runner on dark sparkle |
| 15 | **Inv. Square Rain** | 4-color square rain on dark sparkle |
| 16 | **Inv. Square Twinkle** | 4-color square twinkle on dark sparkle |

### Normal Animations (bright Markov base)

These modes use a bright, undulating base layer where all 200 LEDs glow continuously.

| # | Mode | Description |
|---|------|-------------|
| 17 | **Base** | Markov base layer only — gentle color undulation |
| 18 | **Monochromatic Runner** | Single-color moving Gaussian blob |
| 19 | **Monochromatic Rain** | Single-color fade-in-place bloom |
| 20 | **Monochromatic Twinkle** | Single-color twinkle cycle |
| 21 | **Complementary Runner** | 2-color (opposite hues) runner |
| 22 | **Complementary Rain** | 2-color rain |
| 23 | **Complementary Twinkle** | 2-color twinkle |
| 24 | **Split-Comp Runner** | 3-color split-complementary runner |
| 25 | **Split-Comp Rain** | 3-color split-complementary rain |
| 26 | **Split-Comp Twinkle** | 3-color split-complementary twinkle |
| 27 | **Triadic Runner** | 3-color triadic (120° spacing) runner |
| 28 | **Triadic Rain** | 3-color triadic rain |
| 29 | **Triadic Twinkle** | 3-color triadic twinkle |
| 30 | **Square Runner** | 4-color square (90° spacing) runner |
| 31 | **Square Rain** | 4-color square rain |
| 32 | **Square Twinkle** | 4-color square twinkle |

### Rainbow

| # | Mode | Description |
|---|------|-------------|
| 33 | **Rainbow** | Full-spectrum rainbow sweep across all channels |

### Color Harmony Reference

| Harmony | Colors | Hue Offsets | Character |
|---------|--------|-------------|-----------|
| Monochromatic | 1 | {0°} | Single hue, brightness/sat variations |
| Complementary | 2 | {0°, 180°} | High contrast opposite colors |
| Split-Complementary | 3 | {0°, 150°, 210°} | Primary + two flanking the complement |
| Triadic | 3 | {0°, 120°, 240°} | Three evenly spaced colors |
| Square | 4 | {0°, 90°, 180°, 270°} | Four evenly spaced colors |

---

## Identifying Your Firmware Version

Each verified firmware build has a `PAIRING_CONFIG_ID` and a corresponding git tag.

**Current release:** `PAIRING_CONFIG_ID = 8` → git tag `release-0x08`

### Reading the Version from LEDs

During the factory reset warning animation, the first 8 LEDs display `PAIRING_CONFIG_ID` in binary:
- **Red LED** = bit 1
- **Blue LED** = bit 0
- LED 1 = MSB (bit 7), LED 8 = LSB (bit 0)

### Finding the QR Code for a Specific Release

Each release tag tracks the exact pairing QR code used at that build. To view the QR code for a specific release, navigate to the tag on GitHub:

```
https://github.com/outofjungle/homekit-matchstick-sputter/blob/release-0x08/docs/img/pairing_qr.png
```

Replace `release-0x08` with the tag for the firmware version you're looking up.

---

## Factory Reset

A full factory reset clears all HomeKit pairings, WiFi credentials, saved animation mode, and channel defaults.

### Step-by-Step

1. **Hold** the reset button (GPIO39)
2. At **3 seconds** — LEDs turn solid purple. Keep holding.
3. At **10 seconds** — warning animation begins (3 cycles, ~7 seconds): 8 LEDs flash your firmware version in binary
4. **Keep holding** through the warning animation
5. When the animation ends, **factory reset executes** — device reboots fresh

### Cancelling

Release the button at any point during or after the warning animation (before it ends) to cancel:
- LEDs turn **solid green** for 3 seconds as confirmation of cancellation
- Device resumes normal operation

---

## Hardware Summary

| Spec | Value |
|------|-------|
| Board | M5Stack Stamp Pico |
| MCU | ESP32-PICO-D4 |
| LED Channels | 4 × WS2811, 200 LEDs each |
| Total LEDs | 800 |
| LED Protocol | Single-wire serial (RMT) |
| Power Required | 5V @ 50A minimum for full brightness |
| WiFi | 2.4GHz 802.11 b/g/n |
| HomeKit | Via HomeSpan (HAP protocol) |

See [`docs/HARDWARE.md`](docs/HARDWARE.md) for full wiring details and power calculations.

---

## Developer Documentation

| Document | Contents |
|----------|----------|
| [`docs/HARDWARE.md`](docs/HARDWARE.md) | GPIO pin mapping, wiring diagram, power requirements |
| [`docs/M5STAMP-PICO.md`](docs/M5STAMP-PICO.md) | M5Stack Stamp Pico hardware reference |
| [`docs/HOMESPAN.md`](docs/HOMESPAN.md) | HomeSpan setup, WiFi config, serial commands, troubleshooting |
| [`docs/BUILD_SETUP.md`](docs/BUILD_SETUP.md) | Build environment setup (PlatformIO) |
| [`docs/BASE_LAYER_ANIMATION.md`](docs/BASE_LAYER_ANIMATION.md) | Markov chain base layer algorithm |
| [`docs/INVERTED_BASE_ANIMATION.md`](docs/INVERTED_BASE_ANIMATION.md) | SparkleBaseLayer (dark field) algorithm |
| [`docs/COLOR_HARMONIES.md`](docs/COLOR_HARMONIES.md) | Animation class hierarchy and color harmony system |
| [`docs/RAIN_ANIMATION.md`](docs/RAIN_ANIMATION.md) | Rain animation algorithm |
| [`docs/TWINKLE_ANIMATION.md`](docs/TWINKLE_ANIMATION.md) | Twinkle animation algorithm and timing |
| [`docs/GAUSSIAN_BLENDING.md`](docs/GAUSSIAN_BLENDING.md) | Gaussian blend math used by Runner/Rain |
