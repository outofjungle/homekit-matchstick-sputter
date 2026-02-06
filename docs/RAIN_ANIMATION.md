# Rain Animation

## Overview

Rain animations create a "droplet" effect where random positions on the LED strip light up with harmony-based colors, then fade back into the base layer using a time-varying Gaussian blend. Unlike runner animations that move across the strip, raindrops remain stationary and evolve through their lifecycle.

## Visual Metaphor

Imagine raindrops hitting a colored surface:
- **Frame 0 (impact)**: Drop appears bright and concentrated at center, fading quickly at edges
- **Mid-lifecycle**: Drop spreads outward, blending more with base color
- **Frame MAX (dissipation)**: Drop nearly disappears into base color, curve is flat

## Animation Architecture

Like runner animations, rain animations have:

### Base Layer
All LEDs show the channel's hue with per-LED random-walk undulations:
- **Hue**: ±ANGLE_WIDTH/2 around channel hue (Markov chain)
- **Brightness**: BASE_BRIGHTNESS to MAX_BRIGHTNESS (Markov chain)
- **Markov chain momentum**: 60% chance to continue current direction

### Raindrop Layer
Groups of LEDs ("raindrops") spawn at random non-colliding positions, colored by harmony:
- **Raindrop count**: 6 (at brightness=100) to 18 (at brightness=0)
- **Length**: RAINDROP_LENGTH LEDs (centered at spawn position)
- **Lifecycle**: RAINDROP_MAX_FRAMES frames
- **Gaussian blending**: Time-varying bell curve blend between base and raindrop colors

## Key Differences from Runner Animation

| Aspect | Runner | Rain |
|--------|--------|------|
| **Movement** | Travels from position 0 → end | Stationary at spawn position |
| **Gaussian variance** | Fixed (2.5f) | Time-varying (0.1 → 10.0) |
| **Blending evolution** | Spatial (along runner length) | Temporal (over lifetime) |
| **Spawn location** | Always at position 0 | Random non-colliding position |
| **Density control** | Brightness controls count | Brightness controls count |

## Glossary

**RAINDROP**: A set of pixels centered at `basePos[k]`, extending ±n/2 pixels in both directions, where n is `RAINDROP_LENGTH`.

**Active raindrop**: A raindrop currently in its lifecycle (frame 0 to frame MAX).

**Collision**: Two raindrops whose ranges overlap (within RAINDROP_LENGTH of each other).

## Raindrop Lifecycle

Each raindrop progresses through frames 0 → MAX:

### Frame 0 (Impact)
- **Sigma-squared**: 0.1 (very narrow curve)
- **Center pixels**: ~100% chosen harmony color
- **Edge pixels**: Rapid blend to base color
- **Visual**: Bright, concentrated spot

### Mid-Lifecycle (Frame MAX/2)
- **Sigma-squared**: ~5.0 (medium curve)
- **Center pixels**: ~50% harmony color, 50% base color
- **Edge pixels**: Gradual blend
- **Visual**: Diffuse glow

### Frame MAX (Dissipation)
- **Sigma-squared**: 10.0 (very wide, flat curve)
- **Center pixels**: Mostly base color
- **Edge pixels**: Nearly full base color
- **Visual**: Faint remnant, almost invisible

### Sigma-Squared Progression

```
σ² = 0.1 + (currentFrame / MAX_FRAMES) * 9.9
```

| Frame | σ² | Curve Width | Center Intensity |
|-------|-----|-------------|------------------|
| 0 | 0.1 | Very narrow | ~100% harmony |
| 15 | 5.0 | Medium | ~50% harmony |
| 30 | 10.0 | Very wide | ~10% harmony |

## Blend Factor Evolution

The blend factor combines:
1. **Gaussian curve height** at pixel position (spatial)
2. **Frame progression** (temporal decay)

```cpp
// Spatial: Gaussian curve based on distance from center
float spatialFactor = exp(-(x * x) / (2.0f * sigma_squared));

// Temporal: Decay over lifetime
float temporalFactor = 1.0f - (currentFrame / (float)MAX_FRAMES);

// Combined
float blendFactor = spatialFactor * temporalFactor;
```

### Visual Example (11-LED raindrop, MAX_FRAMES=30)

```
Frame 0 (σ²=0.1):
Blend: |  0  20  80 200 255 200  80  20   0|
Color: |base→ →  →  →HARM←  ←  ← ←base|

Frame 15 (σ²=5.0):
Blend: | 30  50  80 110 128 110  80  50  30|
Color: |base→ → →blend← ← ← ←base|

Frame 30 (σ²=10.0):
Blend: | 10  12  15  18  20  18  15  12  10|
Color: |base base base base base base base|
```

## Brightness Control

Brightness inversely affects maximum active raindrops:

```cpp
maxRaindrops = MAX_RAINDROPS - (brightness * (MAX_RAINDROPS - MIN_RAINDROPS)) / 100
```

| Brightness | Max Raindrops | Visual Effect |
|------------|---------------|---------------|
| 0% | 18 | Very dense, busy |
| 50% | 12 | Moderate density |
| 100% | 6 | Sparse, calm |

## Spawn Logic

### Collision Detection

Before spawning a raindrop:
1. Generate random position: `pos = random(0, numLeds)`
2. Check all active raindrops:
   - If `abs(pos - activePos) < RAINDROP_LENGTH`, collision detected
3. Retry up to MAX_SPAWN_ATTEMPTS (10) times
4. If all attempts fail, skip spawning this frame

### Spawn Probability

Similar to runner animations:

```cpp
int targetSpawnInterval = (MAX_LEDS + RAINDROP_LENGTH) / maxRaindrops;
int spawnChance = (framesSinceSpawn * 100) / targetSpawnInterval;
if (random(100) < spawnChance) {
    // Spawn raindrop
}
```

This ensures raindrops spawn at a rate that maintains the target density.

## Color Selection

Each raindrop picks a random color from the channel's harmony palette:

```cpp
const int* offsets = getHarmonyOffsets();
int idx = random(getNumHarmonyHues());
int hue360 = (channelHue + offsets[idx] + 360) % 360;
int spread = generateSpread();  // Analogous spread (±ANGLE_WIDTH/2)
hue360 = (hue360 + spread + 360) % 360;
```

### Harmony Types

Same as runner animations:

| Harmony | Offsets | Colors |
|---------|---------|--------|
| Monochromatic | {0} | Primary hue + white (50/50 chance) |
| Complementary | {0, 180} | Primary + opposite |
| Split-Complementary | {0, 150, 210} | Primary + two adjacent to opposite |
| Triadic | {0, 120, 240} | Three evenly spaced |
| Square | {0, 90, 180, 270} | Four evenly spaced |

## Tunable Parameters

### Animation Timing
```cpp
static constexpr unsigned long FRAME_MS = 50;        // 20fps
static constexpr uint8_t RAINDROP_MAX_FRAMES = 30;   // 1.5s lifecycle (30 frames * 50ms)
```

### Raindrop Geometry
```cpp
static constexpr uint8_t RAINDROP_LENGTH = 11;       // LEDs per raindrop (must be odd)
static constexpr float MIN_GAUSSIAN_VARIANCE = 0.1f; // Frame 0 variance
static constexpr float MAX_GAUSSIAN_VARIANCE = 10.0f;// Frame MAX variance
```

### Density Control
```cpp
static constexpr uint8_t MIN_RAINDROPS = 6;          // At brightness=100
static constexpr uint8_t MAX_RAINDROPS = 18;         // At brightness=0
static constexpr uint8_t MAX_RAINDROP_SLOTS = 18;    // Per channel
static constexpr uint8_t MAX_SPAWN_ATTEMPTS = 10;    // Collision retry limit
```

### Base Layer (shared with runners)
```cpp
static constexpr int ANGLE_WIDTH = 10;               // ±5° hue spread
static constexpr uint8_t BASE_BRIGHTNESS = 40;       // Min breathing brightness
static constexpr uint8_t MAX_BRIGHTNESS = 220;       // Max breathing brightness
```

## Implementation Structure

```
src/animation/
  gaussian_blend.h              # Shared Gaussian LUT (used by rain and runner)
  rain/
    rain_base.h                 # RainAnimationBase (abstract)
    monochromatic_rain.h        # Monochromatic harmony
    complementary_rain.h        # Complementary harmony
    split_complementary_rain.h  # Split-complementary harmony
    triadic_rain.h              # Triadic harmony
    square_rain.h               # Square harmony
```

### Class Hierarchy

```
AnimationBase (abstract)
└── MarkovBaseLayer (abstract - shared base-layer logic)
    └── RainAnimationBase (abstract)
        ├── MonochromaticRain (primary hue + white)
        ├── ComplementaryRain (2 colors)
        ├── SplitComplementaryRain (3 colors)
        ├── TriadicRain (3 colors)
        └── SquareRain (4 colors)
```

**Note:** MarkovBaseLayer (introduced in Phase 2 refactoring) provides shared base-layer state and Markov chain logic used by both Rain and Runner animations.

### Derived Classes Implement

```cpp
virtual const int* getHarmonyOffsets() const = 0;  // Hue offsets
virtual int getNumHarmonyHues() const = 0;         // Number of colors
virtual const char* getName() const = 0;           // Animation name
```

Optional override:
```cpp
virtual void pickRaindropColor(int channelIndex, uint8_t& h, uint8_t& s, uint8_t& v);
```

## Performance Considerations

**Memory per instance:**
- Base layer state: 4 channels × 200 LEDs × 4 bytes = 3.2 KB
- Raindrop slots: 4 channels × 18 raindrops × ~12 bytes = 864 bytes
- Gaussian LUT cache: Not used (computed per-frame for time-varying variance)
- **Total**: ~4.1 KB per rain animation instance

**Computational cost per frame:**
- Base layer update: 800 Markov transitions (4ch × 200 LEDs)
- Raindrop updates: ~24-72 active raindrops (increment frame, check completion)
- Spawn checks: 4 channels (collision detection + spawn probability)
- Rendering: 800 LEDs × (base color + raindrop blend check)

**Why no LUT caching?**
Unlike runners (fixed variance), raindrops use time-varying variance. Each raindrop has a unique σ² every frame, making precomputed LUTs impractical. Instead, we compute `exp()` on-the-fly during rendering.

**Optimization**: Could cache σ² progression table (30 values) and recompute Gaussian only when σ² changes significantly.

## Comparison with Runner Animation

| Metric | Runner | Rain |
|--------|--------|------|
| Movement | Linear (1 pixel/frame) | Stationary |
| Gaussian variance | Fixed (2.5) | Time-varying (0.1 → 10.0) |
| Blob size | 30 LEDs | 11 LEDs |
| Lifecycle | Spatial (travels strip) | Temporal (fades in place) |
| Spawn location | Position 0 only | Random, non-colliding |
| Max active per channel | 6 runners | 18 raindrops |
| LUT caching | Yes (1 per instance) | No (variance changes) |
| Computational cost | Low (LUT lookup) | Medium (exp() per pixel) |

## Visual Behavior

### Expected Appearance

1. **Startup**: Base layer begins breathing/undulating in channel hue
2. **First raindrop**: Random position lights up with harmony color, starts fading
3. **Subsequent drops**: New drops spawn at different positions (no collision)
4. **Steady state**: 6-18 raindrops active simultaneously (based on brightness)
5. **Color changes**: Raindrop colors update immediately if HomeKit hue changes mid-lifecycle

### Differences from Twinkle

| Aspect | Twinkle | Rain |
|--------|---------|------|
| **Effect size** | Single LED | 11-LED blob |
| **Spawn** | Random per LED | Random position, no collision |
| **Fade** | Linear (FADE_SPEED) | Gaussian temporal decay |
| **Base layer** | Static hue | Markov chain breathing |

Rain is "smoother" and "blobby" compared to twinkle's "sparkly" individual LEDs.

## Testing Checklist

Before committing:
1. ☐ All 4 channels render raindrops at random positions
2. ☐ Raindrops don't collide (no overlapping blobs)
3. ☐ Raindrops fade smoothly (frame 0 → MAX)
4. ☐ Brightness slider controls raindrop density (6-18 drops)
5. ☐ HomeKit hue changes affect raindrop colors immediately
6. ☐ OFF channels stay black (HomeKit power state respected)
7. ☐ Base layer breathes/undulates in channel hue

## Future Enhancements

- **Directional rain**: Raindrops "fall" (move) slowly instead of staying stationary
- **Variable lifecycle**: Randomize MAX_FRAMES per raindrop (some short, some long)
- **Impact intensity**: Vary MIN_VARIANCE per raindrop (some bright, some dim)
- **Color temperature**: Add "cool rain" (blues) vs "warm rain" (oranges) presets
- **Gaussian LUT caching**: Precompute 30-frame progression table to reduce `exp()` calls
