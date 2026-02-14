# Inverted Base Animation

The inverted base is a dark-field variant of the animation system. Instead of a bright, breathing strip, it produces a mostly-black field where 20% of LEDs glow at any moment, each fading in and out smoothly.

## Overview

**Purpose:** Create an atmospheric dark ambiance — nearly-off strip with subtle, twinkling color glimmers.

**Implementation:** `InvertedBaseAnimation` (and all inverted animation variants) use `SparkleBaseLayer` as their base rendering layer (`src/animation/sparkle_base_layer.h`). This replaces the Markov-chain random walk used by the normal base layer.

**Visual character:** 40 of 200 LEDs are active at any moment (~20%). Each active LED fades in and out following a smooth sine hump, with colors pinned at birth. The rest of the strip is black.

## Algorithm

### Active LED Pool

At any time, exactly `DARK_MAX_ACTIVE_LEDS` (40) LEDs are active across 200 total. The other 160 LEDs are black.

On initialization, 40 unique LEDs are randomly selected and activated with staggered phases to avoid synchronized startup.

### Per-LED Lifecycle

Each active LED goes through a single sin8 brightness hump from birth to death:

```
phase: 0 ──────────────────────────> 255 → wrap (death)
brightness: 0 → peaks at ~255 → back to 0
```

**Phase progression:** Each frame, `lifePhase += lifeSpeed` where `lifeSpeed` is random in `[LIFE_SPEED_MIN, LIFE_SPEED_MAX]` = `[2, 6]`. This gives:

| lifeSpeed | Frames per hump | Duration at 20fps |
|-----------|-----------------|-------------------|
| 2         | 128 frames      | ~6.4 s            |
| 6         | ~43 frames      | ~2.1 s            |

**Death on wrap:** When `lifePhase` wraps past 255 (uint8 overflow), the LED dies. A random inactive LED immediately takes its place, keeping the active count fixed at 40.

### Brightness Formula

```cpp
uint8_t raw    = sin8(lifePhase >> 1);       // maps phase 0-255 → sin8 index 0-127
                                              // sin8 gives 128..255 on positive lobe
uint8_t factor = raw - 128;                  // 0..127
uint8_t brightness = (factor * 255) / 127;   // 0..255
```

This traces the positive lobe of sin8, producing a smooth 0 → 255 → 0 hump.

### Birth Colors (Fixed at Birth)

When a LED is born, its hue and saturation are chosen randomly and **pinned for its entire lifetime**:

**Hue:** Random within ±5° of the channel's HomeKit hue:
```cpp
int hue360 = ((chHue + random(-5, 6)) + 360) % 360;
sparkleHue8[ch][i] = map(hue360, 0, 360, 0, 255);
```

**Saturation:** Power-law biased toward high saturation (α=4), clamped to `[SPARKLE_MIN_SAT, 255]` = `[128, 255]`:
```cpp
float u = random(1000) / 1000.0f;
sat = constrain((int)((1.0f - powf(u, 4.0f)) * 255), 128, 255);
```

With α=4, virtually all LEDs get saturation ≥ 128, so colors are vivid and never white.

## Constants

```cpp
static constexpr uint16_t SPARKLE_MAX_LEDS     = 200;   // Total LED pool size
static constexpr uint16_t DARK_MAX_ACTIVE_LEDS = 40;    // 20% of pool active
static constexpr uint8_t  LIFE_SPEED_MIN       = 2;     // ~6.4s hump at 20fps
static constexpr uint8_t  LIFE_SPEED_MAX       = 6;     // ~2.1s hump at 20fps
static constexpr uint8_t  SPARKLE_MIN_SAT      = 128;   // Minimum birth saturation
```

## Comparison: Normal Base vs Inverted Base

| Property              | Normal Base (MarkovBaseLayer)          | Inverted Base (SparkleBaseLayer)          |
|----------------------|----------------------------------------|-------------------------------------------|
| Background           | All LEDs lit, bright                   | All LEDs black by default                 |
| Active LEDs          | 100% (all 200)                         | 20% (40 of 200)                           |
| Brightness mechanism | Markov chain random walk               | sin8 lifecycle hump                        |
| Brightness range     | 40–220 (16%–86%)                       | 0–255 (full hump per lifecycle)            |
| Hue per LED          | Markov walk within ±5° of channel hue  | Fixed at birth, ±5° of channel hue        |
| Saturation per LED   | Power-law Markov walk                  | Fixed at birth, power-law α=4, min=128    |
| LED lifetime         | Continuous, no death                   | ~2–6s per hump, then replaced             |
| Knock-to-zero effect | 5% chance at max brightness            | None (hump naturally returns to 0)        |
| Visual effect        | Bright, breathing, pulsing strip       | Dark field with slow twinkling glimmers   |

## Inverted Animation Variants

All inverted animation variants use `SparkleBaseLayer` as their base layer (via `InvertedBaseAnimation`, `InvertedRunnerBase`, `InvertedRainBase`, `InvertedTwinkleBase`). The SparkleBaseLayer renders the dark background, while the overlay effect (runner/rain/twinkle) renders on top using harmony colors.

This means all 16 inverted modes share the same dark sparkle foundation:
- `ANIM_INVERTED_BASE` — sparkle background only
- `ANIM_INVERTED_*_RUNNER` — runner overlay on sparkle background (5 harmonies)
- `ANIM_INVERTED_*_RAIN` — rain overlay on sparkle background (5 harmonies)
- `ANIM_INVERTED_*_TWINKLE` — twinkle overlay on sparkle background (5 harmonies)

## State Arrays

```cpp
uint8_t sparkleHue8[4][SPARKLE_MAX_LEDS];     // Fixed birth hue (FastLED 0-255)
uint8_t sparkleBirthSat[4][SPARKLE_MAX_LEDS]; // Fixed birth saturation (128-255)
uint8_t lifePhase[4][SPARKLE_MAX_LEDS];       // Position in sin8 cycle (0-255)
uint8_t lifeSpeed[4][SPARKLE_MAX_LEDS];       // Phase increment per frame (0 = inactive)
```

`lifeSpeed == 0` indicates an inactive (black) LED.
