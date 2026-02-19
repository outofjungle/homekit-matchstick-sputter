#include <Arduino.h>
#include <FastLED.h>
#include "HomeSpan.h"
#include "config.h"
#include "led_channel.h"
#include "notification_pattern.h"
#include "animation/animation_manager.h"
#include "pairing_config.h"

// LED Arrays for all channels
CRGB ledChannel1[NUM_LEDS_PER_CHANNEL];        // WS2812B on GPIO 26
CRGB ledChannel2[NUM_LEDS_PER_CHANNEL];        // WS2812B on GPIO 18
CRGB ledChannel3[NUM_LEDS_PER_CHANNEL];        // WS2812B on GPIO 19
CRGB ledChannel4[NUM_LEDS_PER_CHANNEL];        // WS2812B on GPIO 25

// Status LED (SK6812 RGB, 1 pixel) - shows hue of last changed channel
CRGB ledStatus[1];
int statusLedHue = getDefaultHue(2);  // default: channel 2 hue

static void onChannelHueChanged(int hue) {
    statusLedHue = hue;
}

// LED Channel service instances (for boot flash handling)
DEV_LedChannel* channel1Service = nullptr;
DEV_LedChannel* channel2Service = nullptr;
DEV_LedChannel* channel3Service = nullptr;
DEV_LedChannel* channel4Service = nullptr;

// Notification manager for visual feedback
NotificationManager* notificationMgr = nullptr;

// Animation manager for ambient animations
AnimationManager* animationMgr = nullptr;

// Button state machine for GPIO39 (5-tier: short/HSV/AP/pairing display/factory reset)
enum class ButtonState {
    BTN_IDLE,               // Not pressed
    BTN_PRESSED,            // Held, <3s
    BTN_HSV_DONE,           // 3-10s, HSV reset already applied on entry
    BTN_AP_READY,           // 10-15s, purple LEDs, AP mode on release
    BTN_FACTORY_WARNING,    // 15-25s, pairing code display, tracking button release
    BTN_FACTORY_CONFIRM,    // Red feedback (3s), then factory reset
    BTN_FACTORY_CANCELLED,  // Green feedback (3s), then idle
    BTN_RESET               // Factory reset executing
};

ButtonState buttonState = ButtonState::BTN_IDLE;
unsigned long buttonPressStartMs = 0;
unsigned long warningStartMs = 0;           // When pairing display started
unsigned long feedbackStartMs = 0;          // When red/green feedback started
bool buttonReleasedDuringWarning = false;   // Track release during pairing display
bool buttonLastState = HIGH;  // GPIO39 is pulled high, LOW when pressed
bool animButtonLastState = HIGH;  // GPIO0 is pulled high, LOW when pressed

// Animation button state machine
enum class AnimButtonState {
    ANIM_BTN_IDLE,
    ANIM_BTN_PRESSED
};
AnimButtonState animButtonState = AnimButtonState::ANIM_BTN_IDLE;
unsigned long animButtonPressStartMs = 0;

// Forward declarations
void blankAllLEDs();
void applyChannelDefaults();
void updateAnimationButton();
void activateAPMode();
void handleFactoryReset();
void resetAllChannelColors();
void forceAllChannelsPowerOn();

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
                // Long press: toggle inverted
                Serial.println("Animation button long press - toggling inverted");
                if (animationMgr) {
                    animationMgr->toggleInverted();
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

// Validate NVS state and apply defaults only where needed.
// Despite the name, this is NOT a full reset — it's a repair/validation pass.
// Existing hue/saturation/brightness values are preserved unless they are out
// of valid range or missing entirely. A full color reset requires factory reset
// (20s hold on GPIO39).
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


// Reset all channels to their compile-time default H/S/B and force power ON
void resetAllChannelColors() {
    if (channel1Service) channel1Service->applyDefaults();
    if (channel2Service) channel2Service->applyDefaults();
    if (channel3Service) channel3Service->applyDefaults();
    if (channel4Service) channel4Service->applyDefaults();
    Serial.println("All channels reset to default colors");
}

// Force power ON on all channels (preserves current H/S/B)
void forceAllChannelsPowerOn() {
    if (channel1Service) channel1Service->forcePowerOn();
    if (channel2Service) channel2Service->forcePowerOn();
    if (channel3Service) channel3Service->forcePowerOn();
    if (channel4Service) channel4Service->forcePowerOn();
    Serial.println("All channels forced power ON");
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
                // Short press (<3s) — force all channels power ON, stop animations
                forceAllChannelsPowerOn();
                if (animationMgr) {
                    animationMgr->setMode(AnimationMode::ANIM_NONE);
                }
                Serial.println("GPIO39 short press: all channels forced ON, animations stopped");
                buttonState = ButtonState::BTN_IDLE;
            } else if ((now - buttonPressStartMs) >= HSV_RESET_MS) {
                // 3s reached — apply HSV reset immediately, no orange indicator
                buttonState = ButtonState::BTN_HSV_DONE;
                resetAllChannelColors();
                if (animationMgr) {
                    animationMgr->setMode(AnimationMode::ANIM_NONE);
                }
                Serial.println("3s hold: HSV reset applied immediately");
            }
            break;

        case ButtonState::BTN_HSV_DONE:
            if (buttonJustReleased) {
                // Released between 3s and 10s — HSV already applied, return to idle
                Serial.println("GPIO39 3-10s release: idle (HSV already applied)");
                buttonState = ButtonState::BTN_IDLE;
            } else if ((now - buttonPressStartMs) >= AP_ACTIVATE_MS) {
                // Held for 10s — show purple, AP mode on release
                buttonState = ButtonState::BTN_AP_READY;
                Serial.println("10s hold: AP mode ready (purple indicator)");
                blankAllLEDs();
                notificationMgr->start(NotificationPattern::PATTERN_SOLID, CRGB::Purple, 0, 0);
            }
            break;

        case ButtonState::BTN_AP_READY:
            if (buttonJustReleased) {
                // Released between 10s and 15s — activate AP mode
                notificationMgr->stop();
                Serial.println("GPIO39 10-15s release: activating AP mode");
                activateAPMode();
                buttonState = ButtonState::BTN_IDLE;
            } else if ((now - buttonPressStartMs) >= FACTORY_WARNING_MS) {
                // Held for 15s — start pairing code display, track release for cancel/confirm
                buttonState = ButtonState::BTN_FACTORY_WARNING;
                warningStartMs = now;
                buttonReleasedDuringWarning = false;
                Serial.println("15s hold: pairing code display started");
                notificationMgr->stop();
                blankAllLEDs();
                notificationMgr->start(NotificationPattern::PATTERN_PAIRING_ID, CRGB::Black, 0, 0);
            }
            break;

        case ButtonState::BTN_FACTORY_WARNING:
            // Track if button is released at any point during the display window
            if (buttonJustReleased) {
                buttonReleasedDuringWarning = true;
                Serial.println("Button released during pairing display — will cancel on timeout");
            }
            // After minimum display duration, branch to confirm or cancel
            if ((now - warningStartMs) >= PAIRING_DISPLAY_MS) {
                notificationMgr->stop();
                feedbackStartMs = now;
                if (buttonReleasedDuringWarning) {
                    // Cancelled — green feedback, then idle
                    buttonState = ButtonState::BTN_FACTORY_CANCELLED;
                    blankAllLEDs();
                    notificationMgr->start(NotificationPattern::PATTERN_SOLID, CRGB::Green, 0, 0);
                    Serial.println("Factory reset cancelled — showing green feedback");
                } else {
                    // Still held — confirmed, red feedback, then factory reset
                    buttonState = ButtonState::BTN_FACTORY_CONFIRM;
                    blankAllLEDs();
                    notificationMgr->start(NotificationPattern::PATTERN_SOLID, CRGB::Red, 0, 0);
                    Serial.println("Factory reset confirmed — showing red feedback");
                }
            }
            break;

        case ButtonState::BTN_FACTORY_CONFIRM:
            // Show red for FEEDBACK_DISPLAY_MS, then execute factory reset
            if ((now - feedbackStartMs) >= FEEDBACK_DISPLAY_MS) {
                notificationMgr->stop();
                buttonState = ButtonState::BTN_RESET;
                handleFactoryReset();
            }
            break;

        case ButtonState::BTN_FACTORY_CANCELLED:
            // Show green for FEEDBACK_DISPLAY_MS, then return to idle
            if ((now - feedbackStartMs) >= FEEDBACK_DISPLAY_MS) {
                notificationMgr->stop();
                buttonState = ButtonState::BTN_IDLE;
                Serial.println("Factory reset cancel complete — resuming normal operation");
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
    randomSeed(esp_random());

    Serial.println("\n\n========================================");
    Serial.println("homekit-matchstick-sputter - Phase 2");
    Serial.println("HomeKit Integration - 4 Light Channels");
    Serial.println("========================================");

    // Initialize FastLED for all channels
    FastLED.addLeds<WS2812B, PIN_LED_CH1, GRB>(ledChannel1, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2812B, PIN_LED_CH2, GRB>(ledChannel2, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2812B, PIN_LED_CH3, GRB>(ledChannel3, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2812B, PIN_LED_CH4, GRB>(ledChannel4, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<SK6812, PIN_STATUS_SK6812, GRB>(ledStatus, 1);  // Status LED (CH0)

    // Set brightness (50% global cap — power/thermal headroom for 4×200 LEDs)
    FastLED.setBrightness(GLOBAL_BRIGHTNESS);

    // Initialize all LEDs to off
    fill_solid(ledChannel1, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel2, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel3, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel4, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    ledStatus[0] = CRGB::Black;
    FastLED.show();

    Serial.println("FastLED initialized.");

    // Initialize notification manager
    notificationMgr = new NotificationManager(ledChannel1, ledChannel2, ledChannel3, ledChannel4);
    if (!notificationMgr) { Serial.println("FATAL: NotificationManager allocation failed"); return; }
    Serial.println("Notification manager initialized.");

    // Initialize animation manager
    animationMgr = new AnimationManager(ledChannel1, ledChannel2, ledChannel3, ledChannel4, NUM_LEDS_PER_CHANNEL);
    if (!animationMgr) { Serial.println("FATAL: AnimationManager allocation failed"); return; }
    Serial.println("Animation manager initialized.");

    // Initialize button pins
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    Serial.println("Button pin configured (GPIO39 - 4-tier hold actions).");
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
        new DEV_Identify(DEVICE_NAME);
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

    // Initialize status LED hue from channel 2's loaded state, then track any channel changes
    statusLedHue = channel2Service->getDesiredHue();
    channel1Service->onHueChanged = onChannelHueChanged;
    channel2Service->onHueChanged = onChannelHueChanged;
    channel3Service->onHueChanged = onChannelHueChanged;
    channel4Service->onHueChanged = onChannelHueChanged;

    // Display boot flash colors for channels with brightness=0
    FastLED.show();

    Serial.println("========================================");
    Serial.println("Setup complete!");
    Serial.println("GPIO39: short=force ON | 3s=HSV reset | 10s=AP mode | 15s=pairing display | 25s=factory reset");
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
        bool running = notificationMgr->update();
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

    // Update status LED (CH0): full saturation and brightness, hue of last changed channel
    ledStatus[0] = CHSV(map(statusLedHue, 0, 360, 0, 255), 255, 255);

    // Update LED strips (called after every HomeSpan update)
    FastLED.show();
}
