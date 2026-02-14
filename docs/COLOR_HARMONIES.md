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
└── MarkovBaseLayer (abstract - shared base-layer logic)
    ├── BaseOnlyAnimation → InvertedBaseAnimation (+SparkleBaseLayer)
    ├── RunnerAnimationBase (abstract, Gaussian blob movement)
    │   ├── MonochromaticRunner
    │   ├── ComplementaryRunner
    │   ├── SplitComplementaryRunner
    │   ├── TriadicRunner
    │   ├── SquareRunner
    │   └── InvertedRunnerBase (+SparkleBaseLayer)
    │       ├── InvertedMonochromaticRunner
    │       ├── InvertedComplementaryRunner
    │       ├── InvertedSplitComplementaryRunner
    │       ├── InvertedTriadicRunner
    │       └── InvertedSquareRunner
    ├── RainAnimationBase (abstract, Gaussian blob fade-in-place)
    │   ├── MonochromaticRain
    │   ├── ComplementaryRain
    │   ├── SplitComplementaryRain
    │   ├── TriadicRain
    │   ├── SquareRain
    │   └── InvertedRainBase (+SparkleBaseLayer)
    │       ├── InvertedMonochromaticRain
    │       ├── InvertedComplementaryRain
    │       ├── InvertedSplitComplementaryRain
    │       ├── InvertedTriadicRain
    │       └── InvertedSquareRain
    └── TwinkleAnimationBase (abstract, slot-based twinkle cycle)
        ├── MonochromaticTwinkle
        ├── ComplementaryTwinkle
        ├── SplitComplementaryTwinkle
        ├── TriadicTwinkle
        ├── SquareTwinkle
        └── InvertedTwinkleBase (+SparkleBaseLayer)
            ├── InvertedMonochromaticTwinkle
            ├── InvertedComplementaryTwinkle
            ├── InvertedSplitComplementaryTwinkle
            ├── InvertedTriadicTwinkle
            └── InvertedSquareTwinkle
RainbowAnimation (standalone, no MarkovBaseLayer)
```

**Note:** `MarkovBaseLayer` provides shared base-layer state and Markov chain logic for all non-rainbow animations. Inverted variants additionally mix in `SparkleBaseLayer` to replace the normal base with a dark sparkle field. See `docs/INVERTED_BASE_ANIMATION.md` for SparkleBaseLayer details.

### TwinkleAnimationBase

All twinkle harmony animations (including Monochromatic) inherit from `TwinkleAnimationBase`, which provides the slot-based 4-phase twinkle cycle. Abstract methods derived classes must implement:

- **`getHarmonyOffsets()`** - Returns array of hue offsets for the harmony
  - Monochromatic: `{0}`
  - Complementary: `{0, 180}`
  - Split-Complementary: `{0, 150, 210}`
  - Triadic: `{0, 120, 240}`
  - Square: `{0, 90, 180, 270}`

- **`getNumHarmonyHues()`** - Returns number of colors in the harmony

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


## MonochromaticTwinkle

`MonochromaticTwinkle` inherits from `TwinkleAnimationBase` like all other twinkle animations. It uses only the channel's HomeKit hue with no secondary colors:
- Returns `{0}` from `getHarmonyOffsets()` — single hue offset
- All twinkle LEDs use the channel's base hue with varying saturation
- Primary hue (offset 0) is rendered desaturated (appears white) per the standard `pickHarmonyColor()` logic

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

1. Create new class inheriting from `TwinkleAnimationBase` (e.g., `AnalogousTwinkle`)
2. Implement `getHarmonyOffsets()`, `getNumHarmonyHues()`, and `getName()`
3. Add to `AnimationMode` enum in `animation_manager.h`
4. Add `case AnimationMode::ANIM_ANALOGOUS_TWINKLE: return new AnalogousTwinkle();` to `createAnimation()` in `animation_manager.h`

Polymorphic dispatch handles the rest automatically.

### For Runner Animations

To add a new harmony type to runner animations:

1. Create new class inheriting from `RunnerAnimationBase` (e.g., `AnalogousRunner`)
2. Implement `getHarmonyOffsets()`, `getNumHarmonyHues()`, and `getName()`
3. Optional: Override `pickRunnerColor()` for custom color selection
4. Add to `AnimationMode` enum in `animation_manager.h`
5. Add `case AnimationMode::ANIM_ANALOGOUS_RUNNER: return new AnalogousRunner();` to `createAnimation()`

### For Rain Animations

To add a new harmony type to rain animations:

1. Create new class inheriting from `RainAnimationBase` (e.g., `AnalogousRain`)
2. Implement `getHarmonyOffsets()`, `getNumHarmonyHues()`, and `getName()`
3. Optional: Override `pickRaindropColor()` for custom color selection
4. Add to `AnimationMode` enum in `animation_manager.h`
5. Add `case AnimationMode::ANIM_ANALOGOUS_RAIN: return new AnalogousRain();` to `createAnimation()`
