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
1. **Channel 0 (SK6812 RGB, GPIO 27)**: Solid red (initial color)
2. **Channel 1 (WS2811, GPIO 26)**: First pixel blinks red at 1Hz
3. **Channel 2 (WS2811, GPIO 18)**: First pixel blinks green at 1Hz
4. **Channel 3 (WS2811, GPIO 25)**: First pixel blinks blue at 1Hz
5. **Channel 4 (WS2811, GPIO 19)**: First pixel blinks white at 1Hz
6. **Status LED (GPIO 22)**: Off

#### Button Control (GPIO39):
- **Channel 0** cycles through: Red → Green → Blue → White → Red...
- **Status LED** toggles: Off → On → Off...
- Serial output shows current state

### Serial Output Example:
```
Channels 1-4: ON (R/G/B/W)
Channels 1-4: OFF
Button pressed! Channel 0: Green, Status LED: ON
Channels 1-4: ON (R/G/B/W)
Channels 1-4: OFF
```

### Hardware Testing Checklist

Before committing and pushing:
1. ✓ Code compiles without errors
2. ☐ All 4 external channels blink correctly (R/G/B/W)
3. ☐ Channel 0 (SK6812) displays initial red color
4. ☐ Button press cycles channel 0 through colors
5. ☐ Button press toggles status LED
6. ☐ Serial output matches expected behavior

### Configuration

- **Brightness**: 25% (64/255) for safe testing
- **Blink Rate**: 1Hz (500ms on, 500ms off)
- **Button Debounce**: 50ms
- **LEDs per Channel**: 200 (only first pixel controlled in Phase 1)

### Troubleshooting

If colors are wrong:
- Try changing `GRB` to `RGB` in main.cpp:52-56
- Try changing `WS2811` to `NEOPIXEL` if needed
