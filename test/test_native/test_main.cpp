#include <unity.h>
#include "../../src/animation/animation_base.h"

// Test helper: Create a concrete animation class for testing
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

void test_markov_transition_returns_valid_values() {
    TestAnimation anim;

    // Test 1000 transitions from each direction
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
    // Allow 20% deviation
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

    return UNITY_END();
}
