#include <Arduino.h>
#include <FastLED.h>
#include "HomeSpan.h"
#include "config.h"
#include "led_channel.h"
#include "wifi_credentials.h"
#include "notification_pattern.h"
#include "animation/animation_manager.h"

// LED Arrays for all channels
CRGB ledChannel1[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 26
CRGB ledChannel2[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 18
CRGB ledChannel3[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 25
CRGB ledChannel4[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 19

// LED Channel service instances (for boot flash handling)
DEV_LedChannel* channel1Service = nullptr;
DEV_LedChannel* channel2Service = nullptr;
DEV_LedChannel* channel3Service = nullptr;
DEV_LedChannel* channel4Service = nullptr;

// Notification manager for visual feedback
NotificationManager* notificationMgr = nullptr;

// Animation manager for ambient animations
AnimationManager* animationMgr = nullptr;

// Button state machine for factory reset
enum ButtonState {
    BTN_IDLE,               // Not pressed
    BTN_PRESSED,            // Pressed < 5s (short press for mode cycle)
    BTN_NOTIFICATION,       // Showing warning animation (3x cycles) - runs to completion
    BTN_RESET_CONFIRM,      // Showing red confirmation for 3s before reset
    BTN_RESET,              // Factory reset triggered
    BTN_CANCELLED_CONFIRM   // Animation complete, button was released, showing green
};

ButtonState buttonState = BTN_IDLE;
unsigned long buttonPressStartMs = 0;
unsigned long confirmStartMs = 0;
bool buttonLastState = HIGH;  // GPIO39 is pulled high, LOW when pressed
bool buttonReleasedDuringAnimation = false;  // Track if button was released during 3x sequence
uint8_t currentDisplayMode = 0;  // For display mode cycling

// Animation button state (GPIO0)
bool animButtonLastState = HIGH;  // GPIO0 is pulled high, LOW when pressed

// Forward declaration
void blankAllLEDs();
void updateAnimationButton();
void applyChannelDefaults();

// Handle short press (display mode cycling)
void handleShortPress() {
    currentDisplayMode = (currentDisplayMode + 1) % 4;  // Cycle through 4 modes
    Serial.printf("Display mode: %d\n", currentDisplayMode);
    // TODO: Implement actual display mode logic (placeholder for beads-4vz)
}

// Handle factory reset trigger
void handleFactoryReset() {
    Serial.println("FACTORY RESET TRIGGERED!");

    // Clear all channel storage (colors, brightness, power state)
    Serial.println("Clearing channel state...");
    if (channel1Service) channel1Service->clearStorage();
    if (channel2Service) channel2Service->clearStorage();
    if (channel3Service) channel3Service->clearStorage();
    if (channel4Service) channel4Service->clearStorage();

    // Clear animation mode storage
    if (animationMgr) animationMgr->clearStorage();

    // Blank all LEDs for visual feedback
    blankAllLEDs();
    FastLED.show();

    Serial.println("Erasing HomeKit pairings and rebooting...");

    // Perform factory reset using HomeSpan command
    homeSpan.processSerialCommand("F");

    // Device will reboot after this
}

// Blank all LEDs in preparation for notification
void blankAllLEDs() {
    fill_solid(ledChannel1, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel2, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel3, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel4, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    FastLED.show();
}

// Apply channel defaults and validate NVS state
void applyChannelDefaults() {
    Serial.println("Applying channel defaults...");

    for (int ch = 1; ch <= NUM_CHANNELS; ch++) {
        ChannelStorage storage(ch);
        ChannelStorage::ChannelState state = {false, -1, -1, -1};  // Sentinel init
        bool loaded = storage.load(state);
        bool needsSave = false;

        if (!loaded) {
            // No NVS: apply all defaults
            state.power = true;
            state.hue = getDefaultHue(ch);
            state.saturation = DEFAULT_SATURATION;
            state.brightness = DEFAULT_BRIGHTNESS;
            needsSave = true;
            Serial.printf("  Ch%d: No NVS data, applying all defaults\n", ch);
        }

        // Per-field validation (runs even if NVS existed)
        if (state.hue < 0 || state.hue > 360) {
            state.hue = getDefaultHue(ch);
            needsSave = true;
            Serial.printf("  Ch%d: Hue invalid, defaulting to %d°\n", ch, state.hue);
        }

        if (state.saturation < 0 || state.saturation > 100) {
            state.saturation = DEFAULT_SATURATION;
            needsSave = true;
            Serial.printf("  Ch%d: Saturation invalid, defaulting to %d%%\n", ch, state.saturation);
        }

        if (state.brightness <= 0 || state.brightness > 100) {
            state.brightness = DEFAULT_BRIGHTNESS;
            needsSave = true;
            Serial.printf("  Ch%d: Brightness invalid/zero, defaulting to %d%%\n", ch, state.brightness);
        }

        if (!state.power) {
            state.power = true;
            needsSave = true;
            Serial.printf("  Ch%d: Power off, forcing ON\n", ch);
        }

        if (needsSave) {
            storage.save(state);
        }

        Serial.printf("  Ch%d: H=%d° S=%d%% B=%d%% Power=ON\n",
                      ch, state.hue, state.saturation, state.brightness);
    }

    Serial.println("Channel defaults applied.");
}

// Update animation button (GPIO0)
void updateAnimationButton() {
    bool currentButtonState = digitalRead(PIN_BUTTON_ANIM);
    unsigned long now = millis();

    // Debounce
    static unsigned long lastDebounceTime = 0;
    if ((now - lastDebounceTime) < DEBOUNCE_MS) {
        return;
    }

    // Detect button press edge (only on press, not release)
    bool buttonPressed = (currentButtonState == LOW);
    bool buttonJustPressed = (buttonPressed && animButtonLastState == HIGH);

    if (buttonJustPressed) {
        lastDebounceTime = now;
        // Cycle animation mode
        if (animationMgr) {
            animationMgr->cycleMode();
        }
    }

    animButtonLastState = currentButtonState;
}

// Update button state machine
void updateButtonStateMachine() {
    bool currentButtonState = digitalRead(PIN_BUTTON);
    unsigned long now = millis();

    // Debounce
    static unsigned long lastDebounceTime = 0;
    if ((now - lastDebounceTime) < DEBOUNCE_MS) {
        return;
    }

    // Detect button press/release edges
    bool buttonPressed = (currentButtonState == LOW);
    bool buttonJustPressed = (buttonPressed && buttonLastState == HIGH);
    bool buttonJustReleased = (!buttonPressed && buttonLastState == LOW);

    if (buttonJustPressed || buttonJustReleased) {
        lastDebounceTime = now;
    }

    buttonLastState = currentButtonState;

    // State machine logic
    switch (buttonState) {
        case BTN_IDLE:
            if (buttonJustPressed) {
                buttonState = BTN_PRESSED;
                buttonPressStartMs = now;
                Serial.println("Button pressed");
            }
            break;

        case BTN_PRESSED:
            if (buttonJustReleased) {
                // Short press - cycle display mode
                unsigned long pressDuration = now - buttonPressStartMs;
                if (pressDuration < FACTORY_RESET_WARNING_MS) {
                    handleShortPress();
                }
                buttonState = BTN_IDLE;
            } else if ((now - buttonPressStartMs) >= FACTORY_RESET_WARNING_MS) {
                // Held for 5s - enter notification state
                buttonState = BTN_NOTIFICATION;
                buttonReleasedDuringAnimation = false;  // Reset flag
                Serial.println("Entering factory reset warning mode...");

                // Blank ALL LEDs before starting animation
                blankAllLEDs();

                // Start warning animation (3 complete cycles)
                // ~300ms per step = ~2.4s per cycle, ~7.2s total for 3 cycles
                notificationMgr->start(PATTERN_WARNING, CRGB::Red, 300, 3);
            }
            break;

        case BTN_NOTIFICATION:
            // Track button release but let animation complete
            if (buttonJustReleased) {
                Serial.println("Button released - animation will complete, then show cancellation");
                buttonReleasedDuringAnimation = true;
            }

            // Check if animation completed (3 cycles done)
            if (notificationMgr->getCycleCount() >= 3) {
                notificationMgr->stop();

                if (buttonReleasedDuringAnimation || !buttonPressed) {
                    // Button was released during animation - show green confirmation
                    Serial.println("Animation complete - reset cancelled (button was released)");
                    buttonState = BTN_CANCELLED_CONFIRM;
                    confirmStartMs = now;
                    buttonReleasedDuringAnimation = false;

                    // Show green confirmation
                    notificationMgr->start(PATTERN_SOLID, CRGB::Green, 0, 0);
                } else {
                    // Button still held - show red confirmation for 3s before reset
                    Serial.println("Animation complete - button still held, showing red confirmation");
                    buttonState = BTN_RESET_CONFIRM;
                    confirmStartMs = now;

                    // Show red confirmation (solid for 3 seconds)
                    notificationMgr->start(PATTERN_SOLID, CRGB::Red, 0, 0);
                }
            }
            break;

        case BTN_RESET_CONFIRM:
            if ((now - confirmStartMs) >= FACTORY_RESET_CONFIRM_MS) {
                // 3 seconds elapsed - initiate factory reset
                Serial.println("Red confirmation complete - initiating factory reset");
                buttonState = BTN_RESET;
                handleFactoryReset();
            }
            break;

        case BTN_CANCELLED_CONFIRM:
            if ((now - confirmStartMs) >= FACTORY_RESET_CONFIRM_MS) {
                // 3 seconds elapsed - restore previous state and resume
                Serial.println("Resuming normal operation");
                notificationMgr->stop();
                buttonState = BTN_IDLE;
            }
            break;

        case BTN_RESET:
            // Factory reset in progress, device will reboot
            break;
    }
}

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n========================================");
    Serial.println("homekit-matchstick-sputter - Phase 2");
    Serial.println("HomeKit Integration - 4 Light Channels");
    Serial.println("========================================");

    // Initialize FastLED for all channels
    FastLED.addLeds<WS2811, PIN_LED_CH1, GRB>(ledChannel1, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH2, GRB>(ledChannel2, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH3, GRB>(ledChannel3, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH4, GRB>(ledChannel4, NUM_LEDS_PER_CHANNEL);

    // Set brightness (25% for safe testing)
    FastLED.setBrightness(64);

    // Initialize all LEDs to off
    fill_solid(ledChannel1, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel2, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel3, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel4, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    FastLED.show();

    Serial.println("FastLED initialized.");

    // Initialize notification manager
    notificationMgr = new NotificationManager(ledChannel1, ledChannel2, ledChannel3, ledChannel4);
    Serial.println("Notification manager initialized.");

    // Initialize animation manager
    animationMgr = new AnimationManager(ledChannel1, ledChannel2, ledChannel3, ledChannel4, NUM_LEDS_PER_CHANNEL);
    Serial.println("Animation manager initialized.");

    // Initialize button pins
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    Serial.println("Button pin configured (GPIO39 - factory reset).");
    pinMode(PIN_BUTTON_ANIM, INPUT_PULLUP);
    Serial.println("Button pin configured (GPIO0 - animation cycle).");

    // Initialize status LED pin
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);  // Start off during setup
    Serial.println("Status LED pin configured (GPIO22).");

    // Apply channel defaults before HomeSpan initialization
    applyChannelDefaults();

    // Set WiFi credentials before HomeSpan initialization
    homeSpan.setWifiCredentials(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("WiFi credentials configured.");

    // Initialize HomeSpan
    homeSpan.begin(Category::Bridges, DEVICE_NAME);

    Serial.println("HomeSpan initialized.");
    Serial.println("Creating HomeKit accessories...");

    // Create Bridge Accessory (required first)
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name(DEVICE_NAME);
            new Characteristic::Manufacturer(DEVICE_MANUFACTURER);
            new Characteristic::SerialNumber(DEVICE_SERIAL);
            new Characteristic::Model(DEVICE_MODEL);
            new Characteristic::FirmwareRevision(DEVICE_FIRMWARE);

    // Create Channel 1 Accessory
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("Channel 1");
        channel1Service = new DEV_LedChannel(ledChannel1, NUM_LEDS_PER_CHANNEL, 1);

    // Create Channel 2 Accessory
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("Channel 2");
        channel2Service = new DEV_LedChannel(ledChannel2, NUM_LEDS_PER_CHANNEL, 2);

    // Create Channel 3 Accessory
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("Channel 3");
        channel3Service = new DEV_LedChannel(ledChannel3, NUM_LEDS_PER_CHANNEL, 3);

    // Create Channel 4 Accessory
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("Channel 4");
        channel4Service = new DEV_LedChannel(ledChannel4, NUM_LEDS_PER_CHANNEL, 4);

    // Configure notification manager with channel services
    notificationMgr->setChannelServices(channel1Service, channel2Service, channel3Service, channel4Service);

    // Configure animation manager with channel services
    animationMgr->setChannelServices(channel1Service, channel2Service, channel3Service, channel4Service);

    // Display boot flash colors for channels with brightness=0
    FastLED.show();

    Serial.println("========================================");
    Serial.println("Setup complete!");
    Serial.println("Press 'W' in serial monitor to configure WiFi");
    Serial.println("After WiFi is connected, pair with HomeKit");
    Serial.println("========================================\n");

    // Turn on status LED to indicate device is active
    digitalWrite(PIN_STATUS_LED, HIGH);
    Serial.println("Status LED ON - device active");
}

void loop() {
    // Update button state machines
    updateButtonStateMachine();      // GPIO39: Factory reset
    updateAnimationButton();          // GPIO0: Animation cycling

    // Update notification animations if active (highest priority)
    if (notificationMgr->isActive()) {
        bool stillRunning = notificationMgr->update(NUM_LEDS_PER_CHANNEL);
        // Animation completion is now handled in the state machine
        // (checking getCycleCount() in BTN_NOTIFICATION state)
    }

    // Update ambient animations if active (only if notifications not active)
    if (!notificationMgr->isActive() && animationMgr->isActive()) {
        animationMgr->update();
    }

    // Update FSM state for all channels
    if (channel1Service) channel1Service->updateFSM();
    if (channel2Service) channel2Service->updateFSM();
    if (channel3Service) channel3Service->updateFSM();
    if (channel4Service) channel4Service->updateFSM();

    // Poll HomeSpan for HomeKit events
    homeSpan.poll();

    // Update LED strips (called after every HomeSpan update)
    FastLED.show();
}
