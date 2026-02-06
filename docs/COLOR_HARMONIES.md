# Color Harmony Animations

This document describes the color harmony types supported by the ambient animation system and their implementation architecture.

## Color Harmony Types

Color harmonies are based on traditional color theory, using hue relationships on the color wheel (0-360°).

| Harmony | Colors | Hue Offsets | Description | Status |
|---------|--------|-------------|-------------|--------|
| Monochromatic | 1 | {0} | Single hue with brightness/saturation variations | ✓ Implemented |
| Complementary | 2 | {0, 180} | Opposite colors, high contrast | ✓ Implemented |
| Split-Complementary | 3 | {0, 150, 210} | Primary + two adjacent to complement | ✓ Implemented |
| Analogous | 3 | {0, 30, 60} | Adjacent colors, soothing blend | Not implemented |
| Triadic | 3 | {0, 120, 240} | Three evenly spaced colors | ✓ Implemented |
| Square | 4 | {0, 90, 180, 270} | Four evenly spaced colors | ✓ Implemented |
| Tetradic (Rectangle) | 4 | {0, 60, 180, 240} | Two complementary pairs (asymmetric) | Removed (similar to Square) |

## Implementation Architecture

### Class Hierarchy

```
AnimationBase (abstract)
├── MonochromaticTwinkle (standalone, no brightness ratio)
├── HarmonyTwinkleBase (abstract)
│   ├── ComplementaryTwinkle
│   ├── SplitComplementaryTwinkle
│   ├── TriadicTwinkle
│   └── SquareTwinkle
├── RunnerAnimationBase (abstract, Gaussian blob movement)
│   ├── MonochromaticRunner
│   ├── ComplementaryRunner
│   ├── SplitComplementaryRunner
│   ├── TriadicRunner
│   └── SquareRunner
└── RainAnimationBase (abstract, Gaussian blob fade-in-place)
    ├── MonochromaticRain
    ├── ComplementaryRain
    ├── SplitComplementaryRain
    ├── TriadicRain
    └── SquareRain
```

### HarmonyTwinkleBase

All multi-color harmony animations inherit from `HarmonyTwinkleBase`, which provides:

#### Abstract Methods (Derived Classes Must Implement)

- **`getHarmonyOffsets()`** - Returns array of hue offsets for the harmony
  - Complementary: `{0, 180}`
  - Split-Complementary: `{0, 150, 210}`
  - Triadic: `{0, 120, 240}`
  - Square: `{0, 90, 180, 270}`

- **`getNumHarmonyHues()`** - Returns number of colors in the harmony
  - Complementary: 2
  - Split-Complementary: 3
  - Triadic: 3
  - Square: 4

#### Key Methods

- **`assignLedHues()`** - Distributes LEDs across harmony colors using brightness ratio
  - Called during `begin()` and when `setChannelBrightnesses()` is invoked
  - Uses brightness-to-ratio formula to determine primary vs secondary color distribution

- **`setChannelHues()`** - Updates channel hues from HomeKit state
  - Called every frame in `renderCurrentAnimation()` to support real-time color changes

- **`setChannelBrightnesses()`** - Updates channel brightnesses and triggers LED reassignment
  - **Currently not called** - see Known Issues below

### Brightness-to-Ratio Formula

The brightness slider controls the ratio of primary (channel hue) to secondary (harmony) colors:

```cpp
float primaryPercent = 0.20f + (brightness / 100.0f) * 0.60f;
```

| Brightness | Primary % | Secondary % | Effect |
|------------|-----------|-------------|--------|
| 0% | 20% | 80% | Mostly harmony colors |
| 50% | 50% | 50% | Balanced mix |
| 100% | 80% | 20% | Mostly channel hue |

This allows users to control how much their chosen channel color dominates vs. the harmony colors.

### Animation Parameters

Shared across all `HarmonyTwinkleBase` animations:

```cpp
static constexpr uint8_t TWINKLE_DENSITY = 16;   // 1/density chance per frame per LED
static constexpr uint8_t FADE_SPEED = 8;         // Fade speed (0-255, higher = faster)
static constexpr unsigned long FRAME_MS = 50;    // Frame interval (50ms = 20fps)
static constexpr uint8_t BASE_BRIGHTNESS = 20;   // Minimum brightness when "off"
static constexpr uint8_t MAX_BRIGHTNESS = 255;   // Maximum brightness when lit
```

## MonochromaticTwinkle

Unlike harmony animations, `MonochromaticTwinkle` uses only the channel's HomeKit hue with no secondary colors:
- Does not inherit from `HarmonyTwinkleBase`
- Does not use the brightness ratio system
- All LEDs use the same hue (channel's HomeKit color)
- Twinkle effect achieved through brightness variations only

## Known Issues

### Brightness Ratio Not Updating (Bug)

**Problem:** HomeKit brightness changes don't affect harmony animation color ratios in real-time.

**Root Cause:** `animation_manager.h:renderCurrentAnimation()` calls `setChannelHues()` every frame but never calls `setChannelBrightnesses()`.

**Affected Animations:**
- ComplementaryTwinkle
- SplitComplementaryTwinkle
- TriadicTwinkle
- SquareTwinkle

**Not Affected:**
- MonochromaticTwinkle (doesn't use brightness ratio)
- Runner animations (don't use brightness ratio)
- Rain animations (don't use brightness ratio)

**Status:** Tracked in beads issue tracker

## Adding New Harmonies

**Note:** As of the Phase 3 refactoring, `AnimationManager` uses polymorphic dispatch instead of switch statements. Adding a new animation no longer requires modifying multiple switch cases.

### For Twinkle Animations

To add a new harmony type to twinkle animations:

1. Create new class inheriting from `HarmonyTwinkleBase` (e.g., `AnalogousTwinkle`)
2. Implement `getHarmonyOffsets()`, `getNumHarmonyHues()`, and `getName()`
3. Add to `AnimationMode` enum in `animation_manager.h`
4. Add instance member in `AnimationManager` class (e.g., `AnalogousTwinkle analogousTwinkleAnim;`)
5. Add to lookup table in constructor: `animations[ANIM_ANALOGOUS_TWINKLE] = &analogousTwinkleAnim;`

That's it! Polymorphic dispatch handles the rest automatically.

### For Runner Animations

To add a new harmony type to runner animations:

1. Create new class inheriting from `RunnerAnimationBase` (e.g., `AnalogousRunner`)
2. Implement `getHarmonyOffsets()`, `getNumHarmonyHues()`, and `getName()`
3. Optional: Override `pickRunnerColor()` for custom color selection
4. Add to `AnimationMode` enum in `animation_manager.h`
5. Add instance member in `AnimationManager` class (e.g., `AnalogousRunner analogousRunnerAnim;`)
6. Add to lookup table in constructor: `animations[ANIM_ANALOGOUS_RUNNER] = &analogousRunnerAnim;`

### For Rain Animations

To add a new harmony type to rain animations:

1. Create new class inheriting from `RainAnimationBase` (e.g., `AnalogousRain`)
2. Implement `getHarmonyOffsets()`, `getNumHarmonyHues()`, and `getName()`
3. Optional: Override `pickRaindropColor()` for custom color selection
4. Add to `AnimationMode` enum in `animation_manager.h`
5. Add instance member in `AnimationManager` class (e.g., `AnalogousRain analogousRainAnim;`)
6. Add to lookup table in constructor: `animations[ANIM_ANALOGOUS_RAIN] = &analogousRainAnim;`
