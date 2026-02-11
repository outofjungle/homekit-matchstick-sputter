# Inverted Base Animation

The inverted base is a dark-field variant of the normal base layer. Instead of a bright, breathing strip, it produces a mostly-black shimmer where LEDs barely glow and are continuously pulled back toward zero.

## Overview

**Purpose:** Create an atmospheric dark ambiance — nearly-off strip with subtle, dim color glimmers.

**Implementation:** `InvertedBaseAnimation` inherits from `BaseOnlyAnimation` and overrides only the brightness walk. Hue and saturation evolve identically to the normal base layer, so the color character is preserved at very low intensity.

**Visual character:** Most LEDs sit under ~5% brightness at any given moment. Occasional LEDs drift up toward ~20% before being pulled back down by the bias. No bright flares occur.

## Three Independent Random Walks

Each LED maintains three state variables evolved via Markov chains, same as the normal base layer:

### 1. Hue Offset Walk

**Range:** `±ANGLE_WIDTH/2` degrees around the channel's HomeKit hue
**Momentum:** 60% chance to continue in the same direction
**Step Size:** ±1 degree per frame

Identical to the normal base layer — color variation remains within the channel's hue neighborhood.

### 2. Brightness Walk (Power-Law-Biased, toward dark)

**Range:** 0 to `DARK_MAX_BRIGHTNESS` (51, ~20% of 255)
**Step Size:** ±2 per frame
**Bias:** Strongly skewed toward 0 — higher brightness means stronger downward pull

**Key difference from normal base:** The normal base layer biases brightness *upward* toward max. Here the bias is *reversed*, continuously pulling LEDs back toward black.

### 3. Saturation Walk (Power-Law-Biased)

**Range:** `MIN_SATURATION` to 255
**Step Size:** ±2 per frame
**Bias:** Identical to normal base layer — most LEDs maintain high saturation

Saturation walk is unchanged. Even at near-zero brightness the saturation values evolve normally, ready if brightness rises.

## Power-Law Dark Brightness Distribution

### Initial State

On `begin()`, brightness values are sampled from a power-law distribution concentrated near 0:

```
brightness = DARK_MAX_BRIGHTNESS × u^α    where u ~ Uniform(0, 1), α = 3
```

With α=3, the CDF gives:
- **~63%** of LEDs start below brightness **13** (~5% of 255)
- **~87%** start below brightness **26** (~10% of 255)
- **~100%** start at or below **51** (~20% of 255)

This avoids the uniform-distribution startup appearance where LEDs are scattered evenly across the dark range.

### Steady-State Bias

The `DARK_BRIGHT_BIAS` table encodes a downward pull proportional to `sqrt(b / DARK_MAX_BRIGHTNESS)`:

```
bias[b] = round(30 × sqrt(b / 51))
```

| Brightness | Bias | Downward Pull |
|-----------|------|---------------|
| 0         | 0    | No pull (at floor) |
| 13 (~5%)  | 13   | Moderate |
| 26 (~10%) | 18   | Strong |
| 51 (~20%) | 30   | Maximum |

The sqrt shape means bias increases rapidly in the lower range (0–20) and then grows more slowly, giving a gradual squeeze rather than a hard ceiling.

### Bias Table

```cpp
static constexpr int8_t DARK_BRIGHT_BIAS[52] = {
     0,  4,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23,
    24, 24, 24, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29,
    29, 29, 30, 30
};
```

### Transition Probabilities

```
Current State = 0 (stationary):
  (40 + bias)% → -1 (decrease toward dark)   [bias 0–30 → 40–70%]
  30%           →  0 (stay)
  remainder     → +1 (increase)

Current State = -1 (decreasing, toward dark):
  (60 + bias/2)% → -1 (continue)
  25%             →  0 (stop)
  remainder       → +1 (reverse)

Current State = +1 (increasing, toward bright):
  (60 - bias/2)% → +1 (continue)
  25%             →  0 (stop)
  remainder       → -1 (reverse)
```

The bias reduces the probability of continuing upward and increases the probability of turning back. At maximum brightness (bias=30), an increasing LED has only a 45% chance to continue rising, versus 60% at floor.

## Comparison: Normal Base vs Inverted Base

| Property              | Normal Base                        | Inverted Base                     |
|----------------------|------------------------------------|-----------------------------------|
| Brightness range     | 40–220 (16%–86%)                   | 0–51 (0%–20%)                     |
| Brightness bias      | Upward (toward 220)                | Downward (toward 0)               |
| Bias mechanism       | `SAT_BIAS_TABLE` (pushes up)       | `DARK_BRIGHT_BIAS` (pushes down)  |
| Knock-to-zero effect | 10% chance at max brightness       | None needed                       |
| Initial distribution | Uniform in [BASE_BRIGHTNESS, MAX]  | Power-law α=3, concentrated at 0  |
| Visual effect        | Bright, breathing, pulsing strip   | Dark shimmer, near-black          |
| Hue walk             | Identical                          | Identical                         |
| Saturation walk      | Identical                          | Identical                         |

## Implementation Details

### State Arrays

Shared with `BaseOnlyAnimation` (inherited from `MarkovBaseLayer`):

```cpp
int8_t hueOffset[4][MAX_LEDS];       // -ANGLE_WIDTH/2 to +ANGLE_WIDTH/2
int8_t hueDir[4][MAX_LEDS];          // -1, 0, +1
uint8_t baseBrightness[4][MAX_LEDS]; // 0 to DARK_MAX_BRIGHTNESS
int8_t brightDir[4][MAX_LEDS];       // -1, 0, +1
uint8_t baseSaturation[4][MAX_LEDS]; // MIN_SATURATION to 255
int8_t satDir[4][MAX_LEDS];          // -1, 0, +1
```

### Rendering

Direct HSV render, no overlay blending:

```cpp
CHSV(hue8, baseSaturation[ch][i], baseBrightness[ch][i])
```

The low brightness values naturally produce the dim appearance — no brightness scaling is applied at render time.

## Tunable Parameters

```cpp
// Brightness ceiling
static constexpr uint8_t DARK_MAX_BRIGHTNESS = 51;  // ~20% of 255

// Power-law exponent for initial distribution (higher = more concentrated at 0)
// α=3 → ~63% of LEDs start below 5% brightness
float alpha = 3.0f;  // Used in: DARK_MAX_BRIGHTNESS * powf(u, alpha)

// Bias table shape: bias[b] = round(30 × sqrt(b / DARK_MAX_BRIGHTNESS))
// Increase the 30 multiplier for stronger pull toward 0
// Change sqrt to a higher power for faster bias growth
```

To shift more LEDs darker, increase α in the initial distribution or the `30` multiplier in the bias table formula. To allow occasional brighter excursions, raise `DARK_MAX_BRIGHTNESS`.
