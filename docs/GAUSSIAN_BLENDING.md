# Gaussian Blending for Runner Animations

## Overview

Runner animations use **Gaussian curve blending** to create smooth, bell-shaped transitions between the base layer and runner colors. This produces a natural "blob" effect where the runner color is strongest at the center and gradually fades into the base layer at the edges.

## The Math

### Gaussian Function

The blend factor at each position is calculated using the Gaussian (normal distribution) function:

```
g(x) = e^(-(x²) / (2σ²))
```

Where:
- `x` = distance from center (ranging from -15 to +15 for RUNNER_LENGTH=30)
- `σ²` = variance (controls curve width)
- `e` = Euler's number (~2.718)

### Normalization

The result is normalized to 0-255 for FastLED's `blend()` function:

```
blendFactor = constrain(g(x) * 255, 0, 255)
```

### Visual Shape

With `GAUSSIAN_VARIANCE = 2.5`:

```
Blend Factor
255 |        ***
    |       *   *
    |      *     *
    |     *       *
128 |    *         *
    |   *           *
    |  *             *
  0 |**               **
    +-------------------
    0   5   10  15  20  25  30
        Position in Runner (LEDs)
```

The curve is:
- **Centered** at position 15 (middle of 30-LED runner)
- **Strongest** at center (~255 = nearly full runner color)
- **Weakest** at edges (~0 = nearly full base color)
- **Smooth** throughout (no discontinuities or banding)

## Implementation

### Precomputed Lookup Table

The Gaussian curve is precomputed once in `reset()` and stored in a lookup table:

```cpp
GaussianBlendLUT<RUNNER_LENGTH> gaussianLUT;
gaussianLUT.compute(GAUSSIAN_VARIANCE);
```

This avoids expensive `expf()` calls during rendering (which happens 20 times per second across 200 LEDs × 4 channels).

**Memory cost:** 30 bytes per `RunnerAnimationBase` instance

### Per-Pixel Blending

Every LED within the runner uses the LUT for blending:

```cpp
int posInRunner = i - tailPos;  // 0 to RUNNER_LENGTH-1
uint8_t blendFactor = gaussianLUT.table[posInRunner];
finalColor = blend(baseColor, runnerColor, blendFactor);
```

**Result:**
- Position 0 (tail): blend factor ~0 → mostly base color
- Position 15 (center): blend factor ~255 → mostly runner color
- Position 29 (head): blend factor ~0 → mostly base color

## Variance Tuning

The `GAUSSIAN_VARIANCE` constant controls the width of the visible "blob":

| Variance | Visible Width | Visual Effect |
|----------|---------------|---------------|
| 1.5 | ~4-5 pixels | Very narrow, sharp blob |
| 2.5 | ~6-8 pixels | Narrow blob (current) |
| 5.0 | ~12-14 pixels | Medium blob |
| 10.0 | ~20-22 pixels | Wide blob, fills most of runner |

**Current setting:** `2.5f` produces a narrow, focused blob as specified.

### How to Adjust

To change the blob width, modify `GAUSSIAN_VARIANCE` in `runner_base.h`:

```cpp
static constexpr float GAUSSIAN_VARIANCE = 2.5f;  // Adjust this value
```

- **Smaller values** = narrower, more concentrated blob
- **Larger values** = wider, more diffuse blob

The LUT is automatically recomputed on reset.

## Before vs. After

### Before: Linear Tail Blending

The old implementation used two zones:

1. **Tail zone** (33% of runner, 10 LEDs): Linear blend from base to runner
2. **Body zone** (67% of runner, 20 LEDs): Full runner color replacement

```
Blend Factor
255 |          ┌─────────────
    |         /
    |        /
    |       /
128 |      /
    |     /
    |    /
  0 |───┘
    +-------------------
    0   5   10  15  20  25  30
```

**Issues:**
- Visible discontinuity at tail/body boundary
- Abrupt transition at tail start
- No blending at runner head (hard edge)

### After: Gaussian Curve Blending

The new implementation blends smoothly across the entire runner:

```
Blend Factor
255 |        ***
    |      **   **
    |     *       *
128 |    *         *
    |   *           *
  0 |***             ***
    +-------------------
    0   5   10  15  20  25  30
```

**Improvements:**
- Smooth, continuous blending throughout
- No visible discontinuities or banding
- Natural bell-curve shape
- Blends at both tail and head for symmetry

## Performance

**Computational Cost:**
- **One-time:** `expf()` called 30 times during `reset()` (negligible)
- **Per-frame:** Single array lookup per LED in runner (extremely fast)

**Memory Cost:**
- 30 bytes per animation instance (one `uint8_t` per runner position)

**Frame Rate Impact:** None (LUT lookup is faster than the old `map()` calculation)

## Code Structure

```
src/animation/
  gaussian_blend.h           # Template struct with compute()
  runner/
    runner_base.h            # Uses GaussianBlendLUT
    monochromatic_runner.h   # Inherits from runner_base.h
    complementary_runner.h   # Inherits from runner_base.h
    split_complementary_runner.h
    triadic_runner.h
    square_runner.h
```

All 5 runner variants automatically inherit Gaussian blending from `RunnerAnimationBase`.

## References

- [Gaussian Function (Wikipedia)](https://en.wikipedia.org/wiki/Gaussian_function)
- [Normal Distribution](https://en.wikipedia.org/wiki/Normal_distribution)
- FastLED `blend()` function: Linear interpolation between two colors
