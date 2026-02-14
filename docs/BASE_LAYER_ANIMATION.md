# Base Layer Animation

The base layer provides a dynamic, undulating foundation for all animations using three independent Markov chain random walks per LED.

## Overview

**Purpose:** Create visual depth and texture by giving each LED subtle, independent variations in hue, brightness, and saturation.

**Implementation:** All animations that inherit from `MarkovBaseLayer` (Runner, Rain, etc.) render their overlay effects on top of this continuously-evolving base layer.

## Three Independent Random Walks

Each LED maintains three independent state variables, each evolving via its own Markov chain:

### 1. Hue Offset Walk

**Range:** `±ANGLE_WIDTH/2` degrees around the channel's HomeKit hue (`ANGLE_WIDTH=10`, so ±5°)
**Momentum:** 60% chance to continue in the same direction
**Step Size:** ±1 degree per frame

**Purpose:** Creates gentle color variation within the channel's hue neighborhood.

### 2. Brightness Walk (Biased)

**Range:** `BASE_BRIGHTNESS` (40) to `MAX_BRIGHTNESS` (220)
**Step Size:** ±2 per frame
**Bias:** Skewed toward higher brightness values (brightness-biased Markov chain)
**Special Effect:** 5% chance of "knock to zero" when hitting max brightness

**Purpose:** Creates a breathing/pulsing effect with most LEDs staying bright.

### 3. Saturation Walk (Power-Law-Biased)

**Range:** 0-255
**Momentum:** 60% chance to continue in the same direction
**Step Size:** ±2 per frame
**Bias:** Follows a **power law distribution** with α=4

**Purpose:** Creates a natural distribution of saturated colors (vivid) and desaturated colors (whitish/pastel).

## Power-Law-Based Saturation Distribution

The saturation random walk is biased toward a target distribution defined by a **power law distribution**.

### Why Power Law?

The power law distribution provides simple, efficient, and mathematically elegant control over saturation distribution:

```
saturation = 255 × (1 - u^α)  where u ~ Uniform(0,1)
```

With our parameter (α=4):
- **~68%** of LEDs maintain **high saturation** (>200) → vivid, fully-saturated colors
- **~7%** of LEDs show **low saturation** (<64) → whitish/pastel tones
- **~25%** smoothly transition between the extremes

### Visual Distribution

```
Saturation Range    Percentage    Appearance
────────────────────────────────────────────
200-255 (High)      ~63%         Vivid colors
128-199 (Medium)    ~20%         Moderate saturation
64-127  (Low)       ~7%          Pastel/desaturated
0-63    (Very Low)  ~10%         Nearly white/grayscale
```

### Power Law Parameters

**α (Alpha) - Shape Parameter:** Currently 4.0
- Controls the skew toward high saturation
- Higher α → stronger concentration at high saturation
- α = 4 gives ~68% at high saturation, ~7% at very low

**Why α=4?**
- Only requires `powf(u, 4)` - much simpler than Weibull's `exp()` and `log()` calls
- Single parameter provides intuitive control
- Achieves the target distribution: most LEDs vivid, few desaturated

### Bias Lookup Table

To avoid repeated PDF calculations, saturation bias values are pre-computed in `SAT_BIAS_TABLE[256]`:

- **Positive bias:** LED should move toward higher saturation
- **Bias magnitude:** Proportional to how far the current saturation is from the equilibrium distribution

The table is computed from the power law PDF:
```cpp
u = (1 - saturation/255)^(1/α)
pdf(u) = α × u^(α-1)
bias = 30 × (1 - pdf/α)
```

**High PDF region** (equilibrium) → small bias → stay put
**Low PDF region** (non-equilibrium) → strong bias → move toward high saturation

## Markov Chain Mechanics

Each random walk uses a simple 3-state Markov chain:

**States:** -1 (decreasing), 0 (stationary), +1 (increasing)

**Transition Probabilities (brightness-biased):**
```
Current State = 0 (stationary):
  60% → +1 (increase — biased upward)
  20% →  0 (stay)
  20% → -1 (decrease)

Current State = +1 (moving up):
  70% → +1 (continue up — strong momentum)
  15% →  0 (stop)
  15% → -1 (reverse)

Current State = -1 (moving down):
  40% → -1 (continue down — weaker momentum)
  30% →  0 (stop)
  30% → +1 (reverse — more likely to turn around)
```

The asymmetric probabilities create a net upward bias, keeping most LEDs near maximum brightness. The 5% knock-to-zero effect at max brightness prevents all LEDs from staying pinned at maximum.

**Boundary Handling:**
- When hitting min/max values, flip the momentum direction
- This creates a natural "bounce" at boundaries without hard stops

## Implementation Details

### State Arrays (per channel, per LED)

```cpp
int8_t hueOffset[4][MAX_LEDS];       // -ANGLE_WIDTH/2 to +ANGLE_WIDTH/2
int8_t hueDir[4][MAX_LEDS];          // -1, 0, +1
uint8_t baseBrightness[4][MAX_LEDS]; // BASE_BRIGHTNESS to MAX_BRIGHTNESS
int8_t brightDir[4][MAX_LEDS];       // -1, 0, +1
uint8_t baseSaturation[4][MAX_LEDS]; // 0-255
int8_t satDir[4][MAX_LEDS];          // -1, 0, +1
```

### Rendering

Overlay effects (runners, raindrops) render on top of the base color:

```cpp
CRGB baseColor = CHSV(hue8, baseSaturation[ch][i], baseBrightness[ch][i]);
CRGB finalColor = blend(baseColor, overlayColor, blendFactor);
```

## Tunable Parameters

```cpp
// Hue walk
static constexpr int ANGLE_WIDTH = 10;  // ±5° around channel hue

// Brightness walk
static constexpr uint8_t BASE_BRIGHTNESS = 40;
static constexpr uint8_t MAX_BRIGHTNESS = 220;
static constexpr uint8_t BRIGHTNESS_KNOCK_ZERO_PCT = 5;

// Saturation walk (Power law distribution)
static constexpr float POWER_LAW_ALPHA = 4.0f;  // Shape parameter (higher = more skewed)
```

## Effect on Different Animation Types

### Runner Animations
The base layer creates a dynamic "river" of subtle color variation, with runners moving across it as bright, saturated overlays.

### Rain Animations
The base layer provides an ever-changing backdrop, with raindrops appearing as fade-in/fade-out Gaussian blooms of harmony colors.

### Future Animations
Any animation inheriting from `MarkovBaseLayer` automatically gets this rich, textured foundation without additional implementation.
