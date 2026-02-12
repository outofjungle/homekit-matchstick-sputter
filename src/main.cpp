#include <Arduino.h>
#include <FastLED.h>
#include "HomeSpan.h"
#include "config.h"
#include "led_channel.h"
#include "notification_pattern.h"
#include "animation/animation_manager.h"
#include "pairing_config.h"

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

// Button state machine for GPIO39 (3-tier: short/AP/factory reset)
enum class ButtonState {
    BTN_IDLE,               // Not pressed
    BTN_PRESSED,            // Pressed, measuring duration
    BTN_AP_READY,           // 3s reached, solid purple, waiting for release or 10s
    BTN_FACTORY_WARNING,    // 3x warning animation (10s reached)
    BTN_CANCELLED_CONFIRM,  // Warning cancelled, green feedback
    BTN_RESET               // Factory reset executing
};

ButtonState buttonState = ButtonState::BTN_IDLE;
unsigned long buttonPressStartMs = 0;
unsigned long confirmStartMs = 0;
bool buttonLastState = HIGH;  // GPIO39 is pulled high, LOW when pressed
bool animButtonLastState = HIGH;  // GPIO0 is pulled high, LOW when pressed

// Animation button state machine
enum class AnimButtonState {
    ANIM_BTN_IDLE,
    ANIM_BTN_PRESSED
};
AnimButtonState animButtonState = AnimButtonState::ANIM_BTN_IDLE;
unsigned long animButtonPressStartMs = 0;

// Forward declaration
void blankAllLEDs();
void applyChannelDefaults();
void updateAnimationButton();
void activateAPMode();

// AccessoryInformation service with Identify callback
// Subclasses Service::AccessoryInformation so update() fires when Identify is written
struct DEV_Identify : Service::AccessoryInformation {
    DEV_Identify(const char* name) : Service::AccessoryInformation() {
        new Characteristic::Identify();
        new Characteristic::Name(name);
    }

    boolean update() override {
        Serial.println("HomeKit Identify triggered");
        if (notificationMgr) {
            blankAllLEDs();
            notificationMgr->start(NotificationPattern::PATTERN_PAIRING_ID_BLINK, CRGB::Black, 400, 5);
        }
        return true;
    }
};

// Update animation button (GPIO0) - state machine with long press support
void updateAnimationButton() {
    bool currentButtonState = digitalRead(PIN_BUTTON_ANIM);
    unsigned long now = millis();

    // Debounce
    static unsigned long lastDebounceTime = 0;
    if ((now - lastDebounceTime) < DEBOUNCE_MS) {
        return;
    }

    // Detect button press/release edges
    bool buttonPressed = (currentButtonState == LOW);
    bool buttonJustPressed = (buttonPressed && animButtonLastState == HIGH);
    bool buttonJustReleased = (!buttonPressed && animButtonLastState == LOW);

    if (buttonJustPressed || buttonJustReleased) {
        lastDebounceTime = now;
    }

    animButtonLastState = currentButtonState;

    // State machine logic
    switch (animButtonState) {
        case AnimButtonState::ANIM_BTN_IDLE:
            if (buttonJustPressed) {
                animButtonState = AnimButtonState::ANIM_BTN_PRESSED;
                animButtonPressStartMs = now;
                Serial.println("Animation button pressed");
            }
            break;

        case AnimButtonState::ANIM_BTN_PRESSED:
            // Check if long press threshold reached
            if (buttonPressed && (now - animButtonPressStartMs) >= ANIM_BUTTON_LONG_PRESS_MS) {
                // Long press: reset to defaults immediately
                Serial.println("Animation button long press - resetting to defaults");
                applyChannelDefaults();
                if (animationMgr) {
                    animationMgr->setMode(AnimationMode::ANIM_NONE);
                }
                animButtonState = AnimButtonState::ANIM_BTN_IDLE;
            }
            else if (buttonJustReleased) {
                // Short press: cycle animation mode
                unsigned long pressDuration = now - animButtonPressStartMs;
                if (pressDuration < ANIM_BUTTON_LONG_PRESS_MS) {
                    Serial.println("Animation button short press - cycling mode");
                    if (animationMgr) {
                        animationMgr->cycleMode();
                    }
                }
                animButtonState = AnimButtonState::ANIM_BTN_IDLE;
            }
            break;
    }
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

    // Show red confirmation on first 8 LEDs for 1 second
    blankAllLEDs();
    for (int i = 0; i < NOTIFICATION_LEDS; i++) {
        ledChannel1[i] = CRGB::Red;
        ledChannel2[i] = CRGB::Red;
        ledChannel3[i] = CRGB::Red;
        ledChannel4[i] = CRGB::Red;
    }
    FastLED.show();
    delay(1000);

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

// Activate WiFi AP mode for credential setup
void activateAPMode() {
    Serial.println("Activating WiFi AP mode...");
    // Set persistent purple on first 8 LEDs as AP indicator
    blankAllLEDs();
    for (int i = 0; i < NOTIFICATION_LEDS; i++) {
        ledChannel1[i] = CRGB::Purple;
        ledChannel2[i] = CRGB::Purple;
        ledChannel3[i] = CRGB::Purple;
        ledChannel4[i] = CRGB::Purple;
    }
    FastLED.show();
    // Blocking call — runs captive portal until creds entered (reboot) or timeout (returns)
    homeSpan.processSerialCommand("A");
    // If we get here, AP timed out — resume normal operation
    Serial.println("AP mode timed out, resuming normal operation");
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
        case ButtonState::BTN_IDLE:
            if (buttonJustPressed) {
                buttonState = ButtonState::BTN_PRESSED;
                buttonPressStartMs = now;
            }
            break;

        case ButtonState::BTN_PRESSED:
            if (buttonJustReleased) {
                // Released before 3s — short press placeholder
                Serial.println("GPIO39 short press");
                buttonState = ButtonState::BTN_IDLE;
            } else if ((now - buttonPressStartMs) >= AP_ACTIVATE_MS) {
                // Held for 3s — show solid purple immediately, AP is ready
                buttonState = ButtonState::BTN_AP_READY;
                Serial.println("3s hold detected - AP mode ready");
                blankAllLEDs();
                notificationMgr->start(NotificationPattern::PATTERN_SOLID, CRGB::Purple, 0, 0);
            }
            break;

        case ButtonState::BTN_AP_READY:
            if (buttonJustReleased) {
                // Released — activate AP mode
                notificationMgr->stop();
                activateAPMode();
                buttonState = ButtonState::BTN_IDLE;
            } else if ((now - buttonPressStartMs) >= FACTORY_RESET_WARNING_MS) {
                // Held for 10s total — start factory reset warning
                buttonState = ButtonState::BTN_FACTORY_WARNING;
                confirmStartMs = now;
                Serial.println("10s hold detected - entering factory reset warning mode...");
                notificationMgr->stop();
                blankAllLEDs();
                notificationMgr->start(NotificationPattern::PATTERN_PAIRING_ID, CRGB::Black, 0, 0);
            }
            break;

        case ButtonState::BTN_FACTORY_WARNING:
            // Show static pairing ID for 3 seconds, then proceed
            if ((now - confirmStartMs) >= 10000) {
                notificationMgr->stop();

                if (buttonPressed) {
                    // Still held — execute factory reset
                    Serial.println("Warning animation complete - button held, triggering factory reset");
                    buttonState = ButtonState::BTN_RESET;
                    handleFactoryReset();
                } else {
                    // Released — show green cancellation feedback
                    Serial.println("Warning animation complete - button released, reset cancelled");
                    buttonState = ButtonState::BTN_CANCELLED_CONFIRM;
                    confirmStartMs = now;
                    notificationMgr->start(NotificationPattern::PATTERN_SOLID, CRGB::Green, 0, 0);
                }
            }
            break;

        case ButtonState::BTN_CANCELLED_CONFIRM:
            if ((now - confirmStartMs) >= FACTORY_RESET_CONFIRM_MS) {
                // 3 seconds elapsed - resume normal operation
                Serial.println("Resuming normal operation");
                notificationMgr->stop();
                buttonState = ButtonState::BTN_IDLE;
            }
            break;

        case ButtonState::BTN_RESET:
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
    Serial.println("Button pin configured (GPIO0 - animation cycling).");

    Serial.println("Status LED pin configured (GPIO22 - HomeSpan managed).");

    // Apply channel defaults before HomeSpan initialization
    applyChannelDefaults();

    // Configure HomeSpan status LED and AP (open network, activated via GPIO39 3s hold)
    homeSpan.setStatusPin(PIN_STATUS_LED);
    homeSpan.setApSSID(AP_SSID);
    homeSpan.setApPassword("");

    // Set per-device pairing credentials (generated by scripts/generate_pairing.py)
    homeSpan.setPairingCode(PAIRING_SETUP_CODE);
    homeSpan.setQRID(PAIRING_SETUP_ID);

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
        new DEV_Identify("Sputter One");
        channel1Service = new DEV_LedChannel(ledChannel1, NUM_LEDS_PER_CHANNEL, 1);

    // Create Channel 2 Accessory
    new SpanAccessory();
        new DEV_Identify("Sputter Two");
        channel2Service = new DEV_LedChannel(ledChannel2, NUM_LEDS_PER_CHANNEL, 2);

    // Create Channel 3 Accessory
    new SpanAccessory();
        new DEV_Identify("Sputter Three");
        channel3Service = new DEV_LedChannel(ledChannel3, NUM_LEDS_PER_CHANNEL, 3);

    // Create Channel 4 Accessory
    new SpanAccessory();
        new DEV_Identify("Sputter Four");
        channel4Service = new DEV_LedChannel(ledChannel4, NUM_LEDS_PER_CHANNEL, 4);

    // Configure notification manager with channel services
    notificationMgr->setChannelServices(channel1Service, channel2Service, channel3Service, channel4Service);

    // Configure animation manager with channel services
    animationMgr->setChannelServices(channel1Service, channel2Service, channel3Service, channel4Service);

    // Display boot flash colors for channels with brightness=0
    FastLED.show();

    Serial.println("========================================");
    Serial.println("Setup complete!");
    Serial.println("Hold GPIO39 for 3s to activate AP mode (Sputter-Setup, open network)");
    Serial.println("After WiFi is connected, pair with HomeKit");
    Serial.println("========================================\n");

    Serial.println("Status LED active (HomeSpan managed)");
}

void loop() {
    // Update button state machine
    updateButtonStateMachine();      // GPIO39: Factory reset
    updateAnimationButton();         // GPIO0: Animation cycling

    // Update notification animations if active (highest priority)
    if (notificationMgr->isActive()) {
        bool running = notificationMgr->update(NUM_LEDS_PER_CHANNEL);
        if (!running) {
            notificationMgr->stop();
        }
    }

    // Update ambient animations if active (only if notifications not active)
    if (!notificationMgr->isActive() && animationMgr->isActive()) {
        animationMgr->update();
    }

    // Poll HomeSpan for HomeKit events
    homeSpan.poll();

    // Update LED strips (called after every HomeSpan update)
    FastLED.show();
}
