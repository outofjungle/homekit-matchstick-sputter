# Twinkle Animation Periodicity Analysis

Analysis of potential periodicity and synchronization patterns in the twinkle animation system.

---

## System Overview

The twinkle animation runs at **20 fps** (`FRAME_MS = 50ms`). It has two layers:

- **Base layer** (`MarkovBaseLayer`): 800 LEDs (4 channels × 200) each performing independent Markov-chain random walks on hue offset, brightness, and saturation.
- **Twinkle layer** (`TwinkleAnimationBase`): Up to 200 slot-based twinkles per channel, each cycling through a 4-phase state machine.

---

## Sources of Randomness

### 1. ESP32 Arduino `random()` — No Explicit Seeding

No `randomSeed()`, `esp_random()`, or `srand()` call exists in the codebase. The ESP32 Arduino framework auto-seeds its PRNG from hardware noise at startup, but this is **not guaranteed to be unique across reboots**. In practice, the sequence varies between boots, but this is an undocumented behavior rather than a design guarantee.

**Risk**: If the PRNG seed were ever fixed (e.g., during development testing with a deterministic seed), every animation would be identical across reboots.

**Recommendation**: Call `esp_random()` once at startup and pass it to `randomSeed()` to guarantee cryptographic-quality entropy on every boot.

### 2. Base Layer Initialization (`resetBaseLayer()`)

At reset, each of the 800 LEDs gets independent random values:

```cpp
// markov_base_layer.h:67-77
hueOffset[ch][i] = random(-ANGLE_WIDTH / 2, ANGLE_WIDTH / 2 + 1);
hueDir[ch][i] = random(3) - 1;
baseBrightness[ch][i] = random(BASE_BRIGHTNESS, MAX_BRIGHTNESS + 1);
brightDir[ch][i] = random(3) - 1;
float u = random(1000) / 1000.0f;
float power_sample = powf(u, POWER_LAW_ALPHA);
baseSaturation[ch][i] = constrain((int)((1.0f - power_sample) * 255), MIN_SATURATION, 255);
satDir[ch][i] = random(3) - 1;
```

This was addressed by commit `6903b8b` ("Randomize initial base layer LED states to eliminate uniform startup appearance"). The initialization is well-diversified — no periodicity concern here.

---

## Identified Periodicities

### 3. Fixed Phase Durations — Twinkle Synchronization

The fast phases of the twinkle cycle have constant durations:

```cpp
// twinkle_base.h:28-30
static constexpr uint8_t CRASH_FRAMES = 2;        // 100ms
static constexpr uint8_t RISE_FRAMES = 2;          // 100ms
static constexpr uint8_t FINAL_CRASH_FRAMES = 2;  // 100ms
```

The saturation journey phase has variable duration depending on the initial saturation drawn at spawn time:

```cpp
// twinkle_base.h:32
static constexpr uint8_t SAT_STEP = 30; // ~9 frames at mid-saturation, up to ~9 frames
```

The journey traverses from the random initial saturation to either 0 or 255:
- If `sat >= 128`: journey to 0 = `sat / 30` frames (4–8 frames = 200–400ms)
- If `sat < 128`: journey to 255 = `(255 - sat) / 30` frames (5–9 frames = 250–450ms)

**Total cycle length**: 6 frames (fixed) + 4–9 frames (variable) = **10–15 frames (500–750ms)**

**Pattern**: Two twinkles that spawn at the same frame will remain synchronized through their fast phases (crash/rise/final-crash) because those durations are identical. The saturation journey phase adds stochastic de-correlation since initial saturation is randomized per `random(256)`. In practice, with up to 200 simultaneous twinkles, some visual pulse synchronization is expected but is partially masked by the saturation randomness.

### 4. Startup Ramp — All Channels Synchronized

```cpp
// twinkle_base.h:35, 115
static constexpr uint16_t RAMP_FRAMES = 10000 / FRAME_MS; // = 200 frames
rampFrame[ch] = 0; // All channels start at frame 0
```

The effective spawn target ramps linearly from 0 to `maxTwinkles` over 200 frames (10 seconds):

```cpp
// twinkle_base.h:266
int effectiveTarget = (int)((long)maxTwinkles * rampFrame[ch] / RAMP_FRAMES);
```

**Pattern**: All 4 channels start their ramp at the same moment. This means they reach the same density milestones simultaneously. The ramp itself is not visible (it just controls spawn rate, not display), but all channels "fill up" on the same schedule. If someone could observe spawn events, the channels would be correlated. Visually, this is low-impact — each channel's twinkles are independently randomized in position and color.

### 5. Per-Frame Spawn Cap — Regularity at Low Density

```cpp
// twinkle_base.h:34, 269
static constexpr uint8_t SPAWNS_PER_FRAME = 15; // Max new twinkles per frame
int toSpawn = min(deficit, (int)SPAWNS_PER_FRAME);
```

During the ramp-up phase (first 10 seconds), the deficit grows one unit per frame, so one twinkle spawns per frame per channel. This creates a perfectly regular spawn cadence at 20 Hz during ramp-up. However, since each twinkle's LED position is random (`random(MAX_LEDS)`), the visual effect is that scattered LEDs activate at a steady rate rather than in a structured pattern.

After the ramp completes, twinkles respawn whenever a slot is freed, which is stochastic and the cap of 15/frame is rarely hit in steady state.

---

## Summary Table

| Source | Type | Severity | Notes |
|---|---|---|---|
| No explicit `randomSeed()` | Potential | Low | ESP32 auto-seeds from hardware noise, varies per boot |
| Base layer initialization | None | N/A | 800 independent random values, well-diversified |
| Fixed phase durations | Structural | Low-Medium | Co-spawned twinkles pulse in sync; partially broken by sat randomness |
| All channels same ramp start | Structural | Low | Density correlation, not visible in normal operation |
| Ramp-up spawn cadence | Structural | Low | One spawn/frame per channel for ~10s, visually scattered |

---

## Recommendations

### High Priority

**Explicit entropy seeding** — Add one line at firmware startup:
```cpp
randomSeed(esp_random()); // In app_main() or setup()
```
This makes the PRNG sequence demonstrably unique per boot, removing any latent reproducibility risk.

### Medium Priority

**Stagger channel ramp starts** — Shift each channel's ramp by a small offset to eliminate inter-channel density correlation:
```cpp
rampFrame[ch] = ch * (RAMP_FRAMES / 8); // 1.25s stagger between channels
```

### Low Priority

**Jitter fixed phase durations** — Add ±1 frame of randomness to CRASH_FRAMES and RISE_FRAMES at spawn time to break synchronization between co-spawned twinkles:
```cpp
tw.crashFrames = CRASH_FRAMES + random(3) - 1; // 1–3 frames
```
This would require storing per-slot frame limits rather than using the shared constants.

---

## Files Referenced

| File | Relevance |
|---|---|
| `src/animation/twinkle/twinkle_base.h` | Twinkle state machine, ramp logic, phase constants |
| `src/animation/markov_base_layer.h` | Base layer initialization, Markov chain transitions |
| `src/animation/animation_base.h` | `FRAME_MS = 50` (20fps), `MAX_LEDS = 200` |
