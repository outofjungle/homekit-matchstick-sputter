#pragma once

#include "../markov_base_layer.h"

// Base class for all harmony-based twinkle animations
//
// Base layer: All LEDs show channel's hue with per-LED random-walk undulations
//   - Hue: ±ANGLE_WIDTH/2 around channel hue, using Markov chain
//   - Brightness: BASE_BRIGHTNESS to MAX_BRIGHTNESS, using Markov chain
//   - Saturation: Random walk biased toward high saturation
//   - Markov chain has momentum (60% chance to continue current direction)
//
// Twinkle layer: Slot-based (up to MAX_TWINKLE_SLOTS per channel), each performing a 4-phase cycle
//   - Phase 1 (Crash Down): Rapid fade to brightness=0
//   - Phase 2 (Rise Up): Rapid rise to brightness=255 with random saturation and harmony color
//   - Phase 3 (Saturation Journey): Slow saturation shift toward longest path endpoint
//   - Phase 4 (Final Crash): Rapid fade to brightness=0, rejoin base layer
//   - Twinkle count: 20 (at brightness=100) to 200 (at brightness<=5)
//
// Derived classes implement getHarmonyOffsets(), getNumHarmonyHues(), getName()
class TwinkleAnimationBase : public MarkovBaseLayer
{
public:
    // Tunable parameters
    static constexpr uint8_t MIN_TWINKLES = 20;       // At brightness=100 (10% of 200 LEDs)
    static constexpr uint8_t MAX_TWINKLES = 200;      // At brightness<=5 (100% of 200 LEDs)
    static constexpr uint8_t MAX_TWINKLE_SLOTS = 200; // Must equal MAX_TWINKLES
    static constexpr uint8_t CRASH_FRAMES = 2;        // Duration of rapid brightness changes
    static constexpr uint8_t RISE_FRAMES = 2;
    static constexpr uint8_t FINAL_CRASH_FRAMES = 2;
    static constexpr uint8_t BRIGHTNESS_STEP = 128;   // 255 / 2 frames
    static constexpr uint8_t SAT_STEP = 30;           // Saturation change per frame (~9 frames to traverse 255)
    static constexpr uint8_t MAX_SPAWN_ATTEMPTS = 10;          // Collision retry limit
    static constexpr uint8_t SPAWNS_PER_FRAME = 15;            // Max new twinkles per frame (steady-state cap)
    static constexpr uint16_t RAMP_FRAMES = 10000 / FRAME_MS; // Frames to ramp from 0 to target density (~10s)

    // State machine
    enum TwinklePhase
    {
        PHASE_NONE,               // Inactive (slot available)
        PHASE_CRASH_DOWN,         // Phase 1: Rapid fade to 0
        PHASE_RISE_UP,            // Phase 2: Rise to 255 with random sat
        PHASE_SATURATION_JOURNEY, // Phase 3: Slow saturation shift
        PHASE_FINAL_CRASH         // Phase 4: Rapid fade to 0, rejoin base
    };

    struct TwinkleSlot
    {
        uint16_t ledIndex; // Which LED is twinkling
        TwinklePhase phase;
        uint8_t hue;          // Locked harmony color (0-255)
        uint8_t sat;          // Current saturation (0-255)
        uint8_t targetSat;    // Target saturation (0 or 255)
        uint8_t brightness;   // Current brightness (0-255)
        int8_t satDirection;  // +1 toward 255, -1 toward 0
        uint8_t frameCounter; // Frames in current phase
    };

    TwinkleAnimationBase()
    {
        reset();
    }

    void begin() override
    {
        reset();
    }

    bool update(unsigned long deltaMs) override
    {
        frameAccumulator += deltaMs;

        if (frameAccumulator >= FRAME_MS)
        {
            frameAccumulator -= FRAME_MS;
            updateBaseLayer();
            updateTwinkles();
            return true; // Update needed
        }

        return false; // No update needed yet
    }

    void render(CRGB *ch1, CRGB *ch2, CRGB *ch3, CRGB *ch4, uint16_t numLeds) override
    {
        renderChannel(ch1, numLeds, 0);
        renderChannel(ch2, numLeds, 1);
        renderChannel(ch3, numLeds, 2);
        renderChannel(ch4, numLeds, 3);
    }

    void reset() override
    {
        resetBaseLayer();

        for (int ch = 0; ch < 4; ch++)
        {
            // Deactivate all twinkle slots
            for (int t = 0; t < MAX_TWINKLE_SLOTS; t++)
            {
                twinkles[ch][t].phase = PHASE_NONE;
                twinkles[ch][t].ledIndex = 0;
                twinkles[ch][t].frameCounter = 0;
            }
            rampFrame[ch] = 0;
        }
        frameAccumulator = 0;
    }

protected:
    // Twinkle slots (slot-based, not per-LED — saves memory vs [4][MAX_LEDS])
    // RAM: TwinkleSlot[4][200] ≈ 4ch × 200 × ~12 bytes/slot = ~9,600 bytes
    TwinkleSlot twinkles[4][MAX_TWINKLE_SLOTS];
    uint16_t rampFrame[4]; // Frames elapsed since reset per channel, saturates at RAMP_FRAMES

    // Update twinkles (spawning and state machine progression)
    void updateTwinkles()
    {
        for (int ch = 0; ch < 4; ch++)
        {
            // Update existing twinkle slots
            for (int t = 0; t < MAX_TWINKLE_SLOTS; t++)
            {
                TwinkleSlot &tw = twinkles[ch][t];

                if (tw.phase == PHASE_NONE)
                    continue;

                tw.frameCounter++;

                switch (tw.phase)
                {
                case PHASE_CRASH_DOWN:
                    // Rapidly decrease brightness to 0
                    if (tw.brightness >= BRIGHTNESS_STEP)
                        tw.brightness -= BRIGHTNESS_STEP;
                    else
                        tw.brightness = 0;

                    if (tw.frameCounter >= CRASH_FRAMES || tw.brightness == 0)
                    {
                        // Transition to Phase 2: pick random saturation and harmony color
                        tw.phase = PHASE_RISE_UP;
                        tw.frameCounter = 0;
                        tw.sat = random(256);
                        uint8_t dummyV;
                        pickHarmonyColor(ch, tw.hue, tw.sat, dummyV);

                        // Determine saturation journey direction (longest path)
                        if (tw.sat >= 128)
                        {
                            tw.targetSat = 0;
                            tw.satDirection = -1;
                        }
                        else
                        {
                            tw.targetSat = 255;
                            tw.satDirection = 1;
                        }
                        tw.brightness = 0; // Start Phase 2 at 0
                    }
                    break;

                case PHASE_RISE_UP:
                    // Rapidly increase brightness to 255
                    if ((int)tw.brightness + BRIGHTNESS_STEP <= 255)
                        tw.brightness += BRIGHTNESS_STEP;
                    else
                        tw.brightness = 255;

                    if (tw.frameCounter >= RISE_FRAMES || tw.brightness == 255)
                    {
                        tw.phase = PHASE_SATURATION_JOURNEY;
                        tw.frameCounter = 0;
                        tw.brightness = 255;
                    }
                    break;

                case PHASE_SATURATION_JOURNEY:
                    // Slowly adjust saturation toward target
                    if (tw.satDirection > 0)
                    {
                        tw.sat = (tw.sat + SAT_STEP <= 255) ? tw.sat + SAT_STEP : 255;
                    }
                    else
                    {
                        tw.sat = (tw.sat >= SAT_STEP) ? tw.sat - SAT_STEP : 0;
                    }

                    if ((tw.satDirection > 0 && tw.sat >= tw.targetSat) ||
                        (tw.satDirection < 0 && tw.sat <= tw.targetSat))
                    {
                        tw.phase = PHASE_FINAL_CRASH;
                        tw.frameCounter = 0;
                    }
                    break;

                case PHASE_FINAL_CRASH:
                    // Rapidly decrease brightness to 0, then release slot
                    if (tw.brightness >= BRIGHTNESS_STEP)
                        tw.brightness -= BRIGHTNESS_STEP;
                    else
                        tw.brightness = 0;

                    if (tw.frameCounter >= FINAL_CRASH_FRAMES || tw.brightness == 0)
                    {
                        tw.phase = PHASE_NONE; // Release slot, LED rejoins base layer
                    }
                    break;

                default:
                    break;
                }
            }

            // Advance ramp counter (saturates at RAMP_FRAMES)
            if (rampFrame[ch] < RAMP_FRAMES)
                rampFrame[ch]++;

            // Spawn twinkles to fill deficit toward target count
            spawnTwinklesForChannel(ch);
        }
    }

    // Spawn as many twinkles as needed to reach the target count for the channel
    void spawnTwinklesForChannel(int ch)
    {
        // Linear interpolation: brightness<=5 -> MAX_TWINKLES, brightness=100 -> MIN_TWINKLES
        int b = cachedBrightness[ch] < 5 ? 5 : cachedBrightness[ch];
        int maxTwinkles = MAX_TWINKLES - ((b - 5) * (MAX_TWINKLES - MIN_TWINKLES)) / 95;

        // Ramp effective target from 0 to maxTwinkles over RAMP_FRAMES to avoid synchronized startup
        int effectiveTarget = (int)((long)maxTwinkles * rampFrame[ch] / RAMP_FRAMES);

        int deficit = effectiveTarget - countActiveTwinkles(ch);
        int toSpawn = min(deficit, (int)SPAWNS_PER_FRAME); // Cap per-frame spawns

        for (int spawn = 0; spawn < toSpawn; spawn++)
        {
            // Find a random non-twinkling LED and a free slot
            bool spawned = false;
            for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; attempt++)
            {
                uint16_t ledIndex = random(MAX_LEDS);
                if (!isLedTwinkling(ch, ledIndex))
                {
                    for (int t = 0; t < MAX_TWINKLE_SLOTS; t++)
                    {
                        if (twinkles[ch][t].phase == PHASE_NONE)
                        {
                            twinkles[ch][t].ledIndex = ledIndex;
                            twinkles[ch][t].phase = PHASE_CRASH_DOWN;
                            twinkles[ch][t].frameCounter = 0;
                            twinkles[ch][t].brightness = baseBrightness[ch][ledIndex];
                            spawned = true;
                            break;
                        }
                    }
                    break;
                }
            }
            if (!spawned)
                break; // No free LEDs found, stop trying
        }
    }

private:
    // Check if any active slot is using this LED index
    bool isLedTwinkling(int ch, uint16_t ledIndex)
    {
        for (int t = 0; t < MAX_TWINKLE_SLOTS; t++)
        {
            if (twinkles[ch][t].phase != PHASE_NONE && twinkles[ch][t].ledIndex == ledIndex)
                return true;
        }
        return false;
    }

    // Count active twinkle slots on a channel
    int countActiveTwinkles(int ch)
    {
        int count = 0;
        for (int t = 0; t < MAX_TWINKLE_SLOTS; t++)
        {
            if (twinkles[ch][t].phase != PHASE_NONE)
                count++;
        }
        return count;
    }

    // Render a single channel
    void renderChannel(CRGB *leds, uint16_t numLeds, uint8_t channelIndex)
    {
        for (int i = 0; i < (int)numLeds; i++)
        {
            // Check if this LED is in an active twinkle slot
            const TwinkleSlot *activeTwinkle = nullptr;
            for (int t = 0; t < MAX_TWINKLE_SLOTS; t++)
            {
                if (twinkles[channelIndex][t].phase != PHASE_NONE &&
                    twinkles[channelIndex][t].ledIndex == (uint16_t)i)
                {
                    activeTwinkle = &twinkles[channelIndex][t];
                    break;
                }
            }

            if (activeTwinkle == nullptr)
            {
                leds[i] = getBaseLedColor(channelIndex, i);
            }
            else
            {
                // Render twinkle
                leds[i] = CHSV(activeTwinkle->hue, activeTwinkle->sat, activeTwinkle->brightness);
            }
        }
    }
};
