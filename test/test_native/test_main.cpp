#include <unity.h>
#include "../../src/animation/animation_base.h"
#include "../../src/animation/gaussian_blend.h"
#include "../../src/channel_storage.h"
#include "../../src/led_channel.h"
#include "../../src/notification_pattern.h"
#include "../../src/pairing_config.h"

// ========== AnimationBase test helper ==========

class TestAnimation : public AnimationBase {
public:
    void begin() override {}
    bool update(unsigned long deltaMs) override { (void)deltaMs; return false; }
    void render(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4, uint16_t numLeds) override {
        (void)ch1; (void)ch2; (void)ch3; (void)ch4; (void)numLeds;
    }
    void reset() override {}
    const char* getName() const override { return "Test"; }

    // Expose protected methods for testing
    int testMarkovTransition(int8_t dir) { return markovTransition(dir); }
    int testMarkovTransitionBrightnessBiased(int8_t dir) { return markovTransitionBrightnessBiased(dir); }
    int testGenerateSpread() { return generateSpread(); }
};

// ========== Markov Transition Tests ==========

void test_markov_transition_returns_valid_values() {
    TestAnimation anim;

    for (int i = 0; i < 1000; i++) {
        int result = anim.testMarkovTransition(-1);
        TEST_ASSERT_TRUE(result >= -1 && result <= 1);

        result = anim.testMarkovTransition(0);
        TEST_ASSERT_TRUE(result >= -1 && result <= 1);

        result = anim.testMarkovTransition(1);
        TEST_ASSERT_TRUE(result >= -1 && result <= 1);
    }
}

void test_markov_transition_momentum_positive() {
    TestAnimation anim;
    int positiveCount = 0;
    int trials = 1000;

    for (int i = 0; i < trials; i++) {
        int result = anim.testMarkovTransition(1);
        if (result == 1) positiveCount++;
    }

    // Should continue positive >50% of the time (spec says 60%)
    TEST_ASSERT_GREATER_THAN(500, positiveCount);
}

void test_markov_transition_momentum_negative() {
    TestAnimation anim;
    int negativeCount = 0;
    int trials = 1000;

    for (int i = 0; i < trials; i++) {
        int result = anim.testMarkovTransition(-1);
        if (result == -1) negativeCount++;
    }

    // Should continue negative >50% of the time (spec says 60%)
    TEST_ASSERT_GREATER_THAN(500, negativeCount);
}

void test_markov_transition_neutral_distribution() {
    TestAnimation anim;
    int counts[3] = {0, 0, 0};  // -1, 0, +1
    int trials = 1500;

    for (int i = 0; i < trials; i++) {
        int result = anim.testMarkovTransition(0);
        counts[result + 1]++;  // Map -1,0,1 to 0,1,2
    }

    // Each should be roughly 33% (500 ± tolerance)
    TEST_ASSERT_INT_WITHIN(200, 500, counts[0]);  // -1
    TEST_ASSERT_INT_WITHIN(200, 500, counts[1]);  // 0
    TEST_ASSERT_INT_WITHIN(200, 500, counts[2]);  // +1
}

void test_markov_brightness_biased_upward_trend() {
    TestAnimation anim;
    int upCount = 0;
    int trials = 1000;

    for (int i = 0; i < trials; i++) {
        int result = anim.testMarkovTransitionBrightnessBiased(0);
        if (result == 1) upCount++;
    }

    // From neutral, should trend upward (spec says 60% up)
    TEST_ASSERT_GREATER_THAN(500, upCount);
}

// ========== Generate Spread Tests ==========

void test_generate_spread_centered() {
    TestAnimation anim;
    int sum = 0;
    int trials = 1000;

    for (int i = 0; i < trials; i++) {
        sum += anim.testGenerateSpread();
    }

    int average = sum / trials;

    // Average should be near 0 (within ±2 given normal distribution)
    TEST_ASSERT_INT_WITHIN(2, 0, average);
}

void test_generate_spread_bounded() {
    TestAnimation anim;
    int trials = 1000;

    for (int i = 0; i < trials; i++) {
        int spread = anim.testGenerateSpread();

        // ANGLE_WIDTH is 10, so range is -5 to +5
        TEST_ASSERT_GREATER_OR_EQUAL(-5, spread);
        TEST_ASSERT_LESS_OR_EQUAL(5, spread);
    }
}

// ========== GaussianBlendLUT Tests ==========

void test_gaussian_lut_peak_at_center() {
    GaussianBlendLUT<16> lut;
    lut.compute(2.5f);
    // Center index is 8 (LENGTH/2)
    TEST_ASSERT_EQUAL(255, lut.table[8]);
}

void test_gaussian_lut_symmetric() {
    GaussianBlendLUT<16> lut;
    lut.compute(2.5f);
    for (int i = 0; i < 8; i++) {
        // table[8+k] should equal table[8-k] (within ±1 for integer rounding)
        TEST_ASSERT_INT_WITHIN(1, (int)lut.table[8 - i], (int)lut.table[8 + i]);
    }
}

void test_gaussian_lut_monotone_from_center() {
    GaussianBlendLUT<16> lut;
    lut.compute(2.5f);
    // Values should be non-increasing as we move away from center
    for (int i = 8; i < 15; i++) {
        TEST_ASSERT_TRUE(lut.table[i] >= lut.table[i + 1]);
    }
    for (int i = 8; i > 0; i--) {
        TEST_ASSERT_TRUE(lut.table[i] >= lut.table[i - 1]);
    }
}

void test_gaussian_lut_values_bounded() {
    GaussianBlendLUT<30> lut;
    lut.compute(5.0f);
    for (int i = 0; i < 30; i++) {
        TEST_ASSERT_TRUE(lut.table[i] <= 255);
    }
}

// ========== ChannelStorage Tests ==========

void test_channel_storage_load_empty_returns_false() {
    Preferences::clearAll();
    ChannelStorage cs(1);
    ChannelStorage::ChannelState state;
    TEST_ASSERT_FALSE(cs.load(state));
}

void test_channel_storage_save_load_roundtrip() {
    Preferences::clearAll();
    ChannelStorage cs(2);

    ChannelStorage::ChannelState saved;
    saved.power      = true;
    saved.hue        = 180;
    saved.saturation = 75;
    saved.brightness = 60;
    cs.save(saved);

    ChannelStorage::ChannelState loaded;
    TEST_ASSERT_TRUE(cs.load(loaded));
    TEST_ASSERT_EQUAL(saved.power,      loaded.power);
    TEST_ASSERT_EQUAL(saved.hue,        loaded.hue);
    TEST_ASSERT_EQUAL(saved.saturation, loaded.saturation);
    TEST_ASSERT_EQUAL(saved.brightness, loaded.brightness);
}

void test_channel_storage_clear_then_load_false() {
    Preferences::clearAll();
    ChannelStorage cs(3);

    ChannelStorage::ChannelState state;
    state.power = true; state.hue = 90; state.saturation = 100; state.brightness = 80;
    cs.save(state);
    cs.clear();

    ChannelStorage::ChannelState loaded;
    TEST_ASSERT_FALSE(cs.load(loaded));
}

void test_channel_storage_partial_record_rejected() {
    // Simulate partial NVS write: only 'power' key present
    Preferences::clearAll();
    // Write directly via a throwaway Preferences so only 'power' exists
    {
        Preferences p;
        p.begin("channel4", false);
        p.putBool("power", true);
        // hue, sat, bri intentionally omitted
        p.end();
    }
    ChannelStorage cs(4);
    ChannelStorage::ChannelState loaded;
    // New validation requires all 4 keys — should reject partial record
    TEST_ASSERT_FALSE(cs.load(loaded));
}

// ========== NotificationState PAIRING_ID Rendering ==========

void test_notification_pairing_id_bit_layout() {
    CRGB ch1[8], ch2[8], ch3[8], ch4[8];
    NotificationState ns;

    // Use PATTERN_PAIRING_ID — it renders immediately on first update() tick
    ns.start(NotificationPattern::PATTERN_PAIRING_ID, CRGB::White, /*stepDuration=*/0);

    // Advance time past step duration so update() executes the render
    _stub_millis = 1;
    ns.update(ch1, ch2, ch3, ch4);

    static const CRGB bitOff(0, 0, 128);
    static const CRGB bitOn(255, 0, 0);

    for (int i = 0; i < 8; i++) {
        bool expectedOn = ((PAIRING_CONFIG_ID >> i) & 0x01) != 0;
        CRGB expected = expectedOn ? bitOn : bitOff;
        TEST_ASSERT_EQUAL(expected.r, ch1[i].r);
        TEST_ASSERT_EQUAL(expected.g, ch1[i].g);
        TEST_ASSERT_EQUAL(expected.b, ch1[i].b);
        // All channels should match ch1
        TEST_ASSERT_EQUAL(ch1[i].r, ch4[i].r);
        TEST_ASSERT_EQUAL(ch1[i].g, ch4[i].g);
        TEST_ASSERT_EQUAL(ch1[i].b, ch4[i].b);
    }
}

// ========== DEV_LedChannel FSM Tests ==========

// Helper: create a channel with default NVS (power=true by defaults)
static CRGB testLeds[200];

static DEV_LedChannel* makeChannel(int num = 1) {
    Preferences::clearAll();
    _stub_millis = 0;
    return new DEV_LedChannel(testLeds, 50, num);
}

void test_fsm_initial_state_normal() {
    DEV_LedChannel* ch = makeChannel();
    TEST_ASSERT_EQUAL((int)ChannelState::NORMAL, (int)ch->currentState);
    delete ch;
}

void test_fsm_yield_to_notification() {
    DEV_LedChannel* ch = makeChannel();
    ch->yieldToNotification();
    TEST_ASSERT_EQUAL((int)ChannelState::NOTIFICATION, (int)ch->currentState);
    delete ch;
}

void test_fsm_resume_from_notification_restores_normal() {
    DEV_LedChannel* ch = makeChannel();
    ch->yieldToNotification();
    ch->resumeFromNotification();
    TEST_ASSERT_EQUAL((int)ChannelState::NORMAL, (int)ch->currentState);
    delete ch;
}

void test_fsm_yield_to_animation_from_normal() {
    DEV_LedChannel* ch = makeChannel();
    ch->yieldToAnimation();
    TEST_ASSERT_EQUAL((int)ChannelState::ANIMATION, (int)ch->currentState);
    delete ch;
}

void test_fsm_resume_from_animation_to_normal() {
    DEV_LedChannel* ch = makeChannel();
    ch->yieldToAnimation();
    ch->resumeFromAnimation();
    TEST_ASSERT_EQUAL((int)ChannelState::NORMAL, (int)ch->currentState);
    delete ch;
}

void test_fsm_notification_preempts_animation_and_restores_it() {
    DEV_LedChannel* ch = makeChannel();
    ch->yieldToAnimation();
    TEST_ASSERT_EQUAL((int)ChannelState::ANIMATION, (int)ch->currentState);

    ch->yieldToNotification();
    TEST_ASSERT_EQUAL((int)ChannelState::NOTIFICATION, (int)ch->currentState);

    // resumeFromNotification should go back to ANIMATION (pre-notification state)
    ch->resumeFromNotification();
    TEST_ASSERT_EQUAL((int)ChannelState::ANIMATION, (int)ch->currentState);
    delete ch;
}

void test_fsm_double_yield_notification_is_noop() {
    DEV_LedChannel* ch = makeChannel();
    ch->yieldToNotification();
    ch->yieldToNotification();  // should not overwrite preNotificationState
    ch->resumeFromNotification();
    TEST_ASSERT_EQUAL((int)ChannelState::NORMAL, (int)ch->currentState);
    delete ch;
}

// ========== Main ==========

int main() {
    UNITY_BEGIN();

    // Markov transition tests
    RUN_TEST(test_markov_transition_returns_valid_values);
    RUN_TEST(test_markov_transition_momentum_positive);
    RUN_TEST(test_markov_transition_momentum_negative);
    RUN_TEST(test_markov_transition_neutral_distribution);
    RUN_TEST(test_markov_brightness_biased_upward_trend);

    // Generate spread tests
    RUN_TEST(test_generate_spread_centered);
    RUN_TEST(test_generate_spread_bounded);

    // GaussianBlendLUT tests
    RUN_TEST(test_gaussian_lut_peak_at_center);
    RUN_TEST(test_gaussian_lut_symmetric);
    RUN_TEST(test_gaussian_lut_monotone_from_center);
    RUN_TEST(test_gaussian_lut_values_bounded);

    // ChannelStorage (NVS) tests
    RUN_TEST(test_channel_storage_load_empty_returns_false);
    RUN_TEST(test_channel_storage_save_load_roundtrip);
    RUN_TEST(test_channel_storage_clear_then_load_false);
    RUN_TEST(test_channel_storage_partial_record_rejected);

    // NotificationState rendering tests
    RUN_TEST(test_notification_pairing_id_bit_layout);

    // DEV_LedChannel FSM tests
    RUN_TEST(test_fsm_initial_state_normal);
    RUN_TEST(test_fsm_yield_to_notification);
    RUN_TEST(test_fsm_resume_from_notification_restores_normal);
    RUN_TEST(test_fsm_yield_to_animation_from_normal);
    RUN_TEST(test_fsm_resume_from_animation_to_normal);
    RUN_TEST(test_fsm_notification_preempts_animation_and_restores_it);
    RUN_TEST(test_fsm_double_yield_notification_is_noop);

    return UNITY_END();
}
