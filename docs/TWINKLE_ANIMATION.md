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
The LED rapidly but smoothly fades from its current base layer brightness to **brightness = 0**.

- Duration: ~5 frames (0.25s at 20fps)
- The LED visually "winks out"

### Phase 2: Rise Up (Rapid)
The LED picks a **random saturation** and a **harmony color**, then rapidly increases brightness from 0 to 100%.

- Duration: ~5 frames (0.25s at 20fps)
- Saturation: Random value 0-255
- Hue: Selected via `pickHarmonyColor()` (same system as Rain/Runner)
- The LED "pops" back into view with a new color

### Phase 3: Saturation Journey (Slow)
At full brightness, the LED slowly shifts its saturation toward the **longest path endpoint**:

| Initial Saturation | Target | Path Length | Example |
|-------------------|--------|-------------|---------|
| >= 50% (128-255)  | 0%     | sat steps   | 80% → 0 = 204 steps |
| < 50% (0-127)     | 100%   | 255-sat steps | 49% → 100 = 130 steps |

This creates visual interest:
- High saturation LEDs fade toward white (desaturated)
- Low saturation LEDs intensify toward vivid color

- Duration: Variable, 2-5 seconds depending on starting saturation
- Step size: ~2 units per frame
- This is the longest, most visible phase

### Phase 4: Final Crash (Rapid)
The LED rapidly fades back to **brightness = 0**, completing the twinkle cycle.

- Duration: ~5 frames (0.25s at 20fps)
- After reaching brightness = 0, the LED rejoins the base layer

## Timing Summary

| Phase | Duration | Speed |
|-------|----------|-------|
| Phase 1: Crash Down | 0.25s | Rapid |
| Phase 2: Rise Up | 0.25s | Rapid |
| Phase 3: Saturation Journey | 2-5s | Slow |
| Phase 4: Final Crash | 0.25s | Rapid |
| **Total cycle** | **~3-6s** | |

## Twinkle Density

The number of simultaneously twinkling LEDs per channel is controlled by the HomeKit brightness slider:

| Brightness | Active Twinkles | Effect |
|------------|-----------------|--------|
| 0%         | 8 per channel   | Very sparkly |
| 50%        | 5 per channel   | Moderate activity |
| 100%       | 3 per channel   | Subtle, occasional |

This follows the same inverted brightness pattern as Rain and Runner animations.

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

- Twinkle state is tracked per-LED (not per-slot like Rain/Runner)
- Spawn probability increases over time to maintain target density
- Random LED selection avoids clustering
- Base layer continues updating for all LEDs (including twinkling ones) to maintain coherence when rejoining
