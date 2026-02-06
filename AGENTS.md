# Agent Instructions

This project uses **bd** (beads) for issue tracking. Run `bd onboard` to get started.

## Quick Reference

```bash
bd ready              # Find available work
bd show <id>          # View issue details
bd update <id> --status in_progress  # Claim work
bd close <id>         # Complete work
bd sync               # Sync with git
```

## Development Requirements

### Build & Hardware Workflow

**NEVER run build/flash/monitor commands directly.** Instead, prompt the user to run:
- `make build` - Build the project
- `make flash` - Flash to device
- `make monitor` - Monitor serial output
- `make flash-monitor` - Flash and monitor
- `make erase` - Erase flash

**When the user reports a build is working:**
1. Draft a commit message summarizing what was working in that build
2. **MUST confirm the commit message with the user before committing**
3. Only commit after user approval

### LED Channel State Management

All LED channel state changes MUST use the FSM (Finite State Machine) defined in `led_channel.h`. Never use ad-hoc boolean flags or direct state manipulation.

**FSM States:**
- `NORMAL`: Normal HomeKit-controlled operation
- `NOTIFICATION`: Yielded to notification system (highest priority)
- `ANIMATION`: Ambient animation active
- `OFF`: Power is off

**State Transitions:**
- Use `enterState()` to transition states (handles rendering)
- Use `updateFSM()` for FSM updates (currently no time-based transitions)
- Use `yieldToNotification()` / `resumeFromNotification()` for notifications
- Use `yieldToAnimation()` / `resumeFromAnimation()` for animations

**Priority System:**
- NOTIFICATION > ANIMATION > NORMAL/OFF
- HomeKit power state is respected during animations (OFF channels stay off)

**Brightness Clamping:**
- Any brightness=0 value is automatically forced to 80% (MIN_BRIGHTNESS)
- This applies both at boot (loading from NVS) and when receiving HomeKit updates

## Phase 1: Multi-Channel LED Blink Test

### Build & Flash

```bash
make clean         # Clean build artifacts
make build         # Build the project
make flash         # Flash to device
make monitor       # Monitor serial output
make flash-monitor # Flash and start monitoring
```

### Expected Behavior

#### On Startup:
1. **Channel 0 (SK6812 RGB, GPIO 27)**: Status indicator
2. **Channel 1 (WS2811, GPIO 26)**: Default hue 0° (Red)
3. **Channel 2 (WS2811, GPIO 18)**: Default hue 90° (Yellow/Orange)
4. **Channel 3 (WS2811, GPIO 25)**: Default hue 180° (Cyan)
5. **Channel 4 (WS2811, GPIO 19)**: Default hue 270° (Purple/Magenta)
6. **Status LED (GPIO 22)**: On when device active

#### Default Channel Colors (90° spacing):
| Channel | Hue | Color |
|---------|-----|-------|
| 1 | 0° | Red |
| 2 | 90° | Yellow/Orange |
| 3 | 180° | Cyan |
| 4 | 270° | Purple/Magenta |

Users can change any channel's color via HomeKit.

#### Button Control:
- **GPIO39 (long press 5s)**: Factory reset with warning animation
- **GPIO0 (short press)**: Cycle through 15 animation modes (harmonies: Monochromatic, Complementary, Split-Complementary, Triadic, Square)

**Animation Requirements (all existing and future):**
- Must use channel's HomeKit hue as primary color (90° default spacing)
- Must reflect real-time hue changes from HomeKit (update each frame)
- Must respect HomeKit power state (OFF channels stay off)
- Must use same rendering logic for all 4 channels (no code duplication)
- Animation mode persists in NVS across power cycles

### Hardware Testing Checklist

Before committing and pushing:
1. ☐ Code compiles without errors
2. ☐ All 4 channels display default colors (Red, Yellow, Cyan, Purple)
3. ☐ GPIO0 button cycles through all 15 animation modes
4. ☐ All animations use channel hues (not hardcoded colors)
5. ☐ Turning off a channel in HomeKit blacks it out during animation
6. ☐ GPIO39 long press triggers factory reset warning

### Configuration

- **Brightness**: 80% minimum (forced for brightness=0)
- **Blink Rate**: 1Hz (500ms on, 500ms off)
- **Button Debounce**: 50ms
- **LEDs per Channel**: 200 (only first pixel controlled in Phase 1)

### LED Strip Length Limitation

The firmware is configured to control exactly `NUM_LEDS_PER_CHANNEL` (200) LEDs per channel. If your physical LED strip has more than 200 LEDs:

- **LEDs 1-200**: Controlled by firmware, display correct colors
- **LEDs 201+**: Not addressed by firmware, show random/garbage colors (power-on state)

This is expected WS2811/WS2812 protocol behavior. LEDs that don't receive data retain their power-on state.

**Future Solutions:**
- Make LED count a runtime NVS setting
- Hardware termination at position 200
- Document maximum supported strip length

### Troubleshooting

If colors are wrong:
- Try changing `GRB` to `RGB` in main.cpp:52-56
- Try changing `WS2811` to `NEOPIXEL` if needed
