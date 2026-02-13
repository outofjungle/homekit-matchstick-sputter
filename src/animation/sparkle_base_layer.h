#pragma once

#include <FastLED.h>
#include <math.h>

// Pure mixin for the sparkle/inverted base layer.
//
// Each active LED has:
//   - A fixed birth hue (random within ±5° of channel hue, stays until death)
//   - A fixed birth saturation (power-law biased toward high sat, stays until death)
//
// Brightness follows a sin8 lifecycle hump (0 → 255 → 0):
//   phase 0→255 maps onto sin8 indices 0→127 (positive lobe, values 128→255)
//   brightness = (sin8(phase>>1) - 128) * 255 / 127
// Hue and saturation are pinned at birth and stay constant until death.
//
// On phase wrap the LED dies, and a random inactive LED is born in its place.
// Pool size is fixed at DARK_MAX_ACTIVE_LEDS — no bookkeeping needed.
class SparkleBaseLayer
{
public:
    static constexpr uint16_t SPARKLE_MAX_LEDS     = 200;
    static constexpr uint16_t DARK_MAX_ACTIVE_LEDS = SPARKLE_MAX_LEDS / 5;   // 40 = 20% cap
    static constexpr uint8_t  LIFE_SPEED_MIN       = 2;                       // ~6.4 s hump at 20 fps
    static constexpr uint8_t  LIFE_SPEED_MAX       = 6;                       // ~2.1 s hump at 20 fps
    static constexpr uint8_t  SPARKLE_MIN_SAT      = 128;

protected:
    uint8_t sparkleHue8[4][SPARKLE_MAX_LEDS];    // Fixed birth hue (FastLED 0-255)
    uint8_t sparkleBirthSat[4][SPARKLE_MAX_LEDS]; // Fixed birth saturation (128-255)
    uint8_t lifePhase[4][SPARKLE_MAX_LEDS];       // Position in sin8 cycle (0-255)
    uint8_t lifeSpeed[4][SPARKLE_MAX_LEDS];       // Phase increment per frame (0 = inactive)

    // Call from begin()/reset()
    void initSparkleBaseLayer(const int channelHue[4])
    {
        for (int ch = 0; ch < 4; ch++)
        {
            for (int i = 0; i < SPARKLE_MAX_LEDS; i++)
            {
                lifeSpeed[ch][i] = 0;
                lifePhase[ch][i] = 0;
            }
            // Activate exactly DARK_MAX_ACTIVE_LEDS unique LEDs with staggered phases
            uint16_t activated = 0;
            while (activated < DARK_MAX_ACTIVE_LEDS)
            {
                int i = random(SPARKLE_MAX_LEDS);
                if (lifeSpeed[ch][i] == 0)
                {
                    birthLed(ch, i, channelHue[ch]);
                    lifePhase[ch][i] = random(256 - LIFE_SPEED_MAX); // stagger; clamp to avoid first-frame wrap
                    activated++;
                }
            }
        }
    }

    // Call every frame
    void updateSparkleBaseLayer(const int channelHue[4])
    {
        for (int ch = 0; ch < 4; ch++)
        {
            for (int i = 0; i < SPARKLE_MAX_LEDS; i++)
            {
                if (lifeSpeed[ch][i] == 0) continue;

                uint8_t oldPhase = lifePhase[ch][i];
                lifePhase[ch][i] += lifeSpeed[ch][i];

                // uint8_t wrap → LED dies; activate a random inactive LED instead
                if (lifePhase[ch][i] < oldPhase)
                {
                    lifeSpeed[ch][i] = 0;
                    lifePhase[ch][i] = 0;

                    // Pick a random inactive LED to come alive
                    // ~80% of LEDs are inactive, so this terminates quickly
                    int newIdx;
                    do { newIdx = random(SPARKLE_MAX_LEDS); }
                    while (lifeSpeed[ch][newIdx] != 0);

                    birthLed(ch, newIdx, channelHue[ch]);
                    lifePhase[ch][newIdx] = 0;
                }
            }
        }
    }

    // Render sparkle color for one LED
    CRGB computeSparkleColor(int ch, int i) const
    {
        if (lifeSpeed[ch][i] == 0) return CRGB::Black;

        // Positive sin8 lobe: phase 0→255 → sin8(phase>>1) gives 128→255→128
        uint8_t raw    = sin8(lifePhase[ch][i] >> 1); // 128..255
        uint16_t factor = (uint16_t)(raw - 128);       // 0..127

        uint8_t brightness = (uint8_t)((factor * 255) / 127);

        return CHSV(sparkleHue8[ch][i], sparkleBirthSat[ch][i], brightness);
    }

private:
    void birthLed(int ch, int i, int chHue)
    {
        lifeSpeed[ch][i] = random(LIFE_SPEED_MIN, LIFE_SPEED_MAX + 1);

        // Hue: fixed random offset within ±5° of channel hue
        int hue360 = ((chHue + random(-5, 6)) + 360) % 360;
        sparkleHue8[ch][i] = (uint8_t)map(hue360, 0, 360, 0, 255);

        // Saturation: power-law biased toward high (α=4 → heavily high-sat)
        float u = random(1000) / 1000.0f;
        sparkleBirthSat[ch][i] = (uint8_t)constrain(
            (int)((1.0f - powf(u, 4.0f)) * 255), (int)SPARKLE_MIN_SAT, 255);
    }
};
