# Twinkle Animation

Twinkle animations create a sparkling effect where individual LEDs temporarily break away from the base layer, perform a distinctive brightness and saturation cycle, then seamlessly rejoin.

## Architecture

Twinkle animations extend `MarkovBaseLayer`, sharing the same base layer system as Rain and Runner animations:
- **Base layer**: All LEDs undulate using three Markov chain random walks (hue offset, brightness, saturation)
- **Twinkle overlay**: Individual LEDs temporarily override their base layer state with a 4-phase twinkle cycle

## The Twinkle Cycle

Each twinkle is a 4-phase animation cycle that an individual LED performs:

```
Base Layer → Phase 1 → Phase 2 → Phase 3 → Phase 4 → Base Layer
             (crash)   (rise)    (sat)     (crash)
```

### Phase 1: Crash Down (Rapid)
The LED rapidly fades from its current base layer brightness to **brightness = 0**.

- Duration: `CRASH_FRAMES` = 2 frames (0.10s at 20fps)
- Step size: `BRIGHTNESS_STEP` = 128 per frame (reaches 0 in 2 frames)
- The LED visually "winks out"

### Phase 2: Rise Up (Rapid)
The LED picks a **random saturation** and a **harmony color**, then rapidly increases brightness from 0 to 255.

- Duration: `RISE_FRAMES` = 2 frames (0.10s at 20fps)
- Saturation: Random value 0-255
- Hue: Selected via `pickHarmonyColor()` (same system as Rain/Runner)
- The LED "pops" back into view with a new color

### Phase 3: Saturation Journey (Slow)
At full brightness, the LED slowly shifts its saturation toward the **longest path endpoint**:

| Initial Saturation | Target | Direction |
|-------------------|--------|-----------|
| ≥ 128 (high)      | 0      | Toward white/desaturated |
| < 128 (low)       | 255    | Toward vivid color |

This creates visual interest:
- High saturation LEDs fade toward white (desaturated)
- Low saturation LEDs intensify toward vivid color

- Duration: Variable, ~4–9 frames depending on starting saturation (SAT_STEP = 30/frame)
- This is the longest, most visible phase

### Phase 4: Final Crash (Rapid)
The LED rapidly fades back to **brightness = 0**, completing the twinkle cycle.

- Duration: `FINAL_CRASH_FRAMES` = 2 frames (0.10s at 20fps)
- After reaching brightness = 0, the LED rejoins the base layer

## Timing Summary

| Phase | Duration | Speed |
|-------|----------|-------|
| Phase 1: Crash Down | 2 frames (~0.10s) | Rapid |
| Phase 2: Rise Up | 2 frames (~0.10s) | Rapid |
| Phase 3: Saturation Journey | ~4–9 frames (~0.2–0.45s) | Slow |
| Phase 4: Final Crash | 2 frames (~0.10s) | Rapid |
| **Total cycle** | **~10–13 frames (~0.5–0.65s)** | |

## Twinkle Density

The number of simultaneously twinkling LEDs per channel is controlled by the HomeKit brightness slider. The density is inverted: lower brightness → more twinkles.

| Brightness | Active Twinkles | Effect |
|------------|-----------------|--------|
| ≤5%        | 200 per channel | Maximum density (every LED) |
| 50%        | ~110 per channel | High density |
| 100%       | 20 per channel  | Minimum density (10% of LEDs) |

Constants: `MIN_TWINKLES = 20` (at brightness=100), `MAX_TWINKLES = 200` (at brightness≤5). Density ramps up over ~10 seconds on startup to avoid synchronized initialization.

## Color Harmonies

Twinkle animations use the same 5 harmony types as other animations:

| Harmony | Hue Offsets | Colors |
|---------|-------------|--------|
| Monochromatic | {0} | 1 |
| Complementary | {0, 180} | 2 |
| Split-Complementary | {0, 150, 210} | 3 |
| Triadic | {0, 120, 240} | 3 |
| Square | {0, 90, 180, 270} | 4 |

Each twinkle selects a random harmony color via `pickHarmonyColor()`:
- **Primary hue (offset 0)**: Rendered desaturated (appears white)
- **Secondary hues**: Full saturation (vivid colors)

## State Machine

```cpp
enum TwinklePhase {
    PHASE_NONE,              // Following base layer
    PHASE_CRASH_DOWN,        // Phase 1: Fading to black
    PHASE_RISE_UP,           // Phase 2: Rising to full brightness
    PHASE_SATURATION_JOURNEY,// Phase 3: Slowly shifting saturation
    PHASE_FINAL_CRASH        // Phase 4: Fading back to black
};
```

## Rendering Logic

During each frame:
1. `updateBaseLayer()` evolves the base layer for all LEDs
2. `updateTwinkles()` advances the state machine for twinkling LEDs
3. `renderChannel()` outputs either:
   - **Base layer color** for LEDs in `PHASE_NONE`
   - **Twinkle color** for LEDs in phases 1-4

When a twinkle completes Phase 4, the LED seamlessly transitions back to base layer control since both are at brightness = 0.

## Implementation Notes

- Twinkle state is tracked in slot arrays (`TwinkleSlot twinkles[4][MAX_TWINKLE_SLOTS]`), not per-LED
- Up to `MAX_TWINKLE_SLOTS` = 200 slots per channel; each slot tracks one active twinkle
- Spawn rate is capped at `SPAWNS_PER_FRAME` = 15 new twinkles per frame to avoid stuttering
- A 10-second ramp (`RAMP_FRAMES`) prevents all twinkles from spawning simultaneously on startup
- Base layer continues updating for all LEDs (including twinkling ones) to maintain coherence when rejoining
