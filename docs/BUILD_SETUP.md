# Build Environment Setup

## Overview

This document describes the build environment setup required for this project, including platform-specific requirements and troubleshooting steps.

## Platform Requirements

### PlatformIO Platform

This project uses the **pioarduino** community platform instead of the official espressif32 platform:

```ini
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
```

**Reason**: HomeSpan 2.1.7+ requires Arduino-ESP32 3.3.0+, which is not supported by the official PlatformIO espressif32 platform. The pioarduino community fork provides Arduino-ESP32 3.3.6 compatibility.

### Partition Scheme

The project uses the `huge_app.csv` partition scheme to accommodate the large firmware size (HomeSpan + FastLED):

```ini
board_build.partitions = huge_app.csv
```

**Without this**: Firmware size (~1.5MB) exceeds the default partition limit (1.3MB).

## Python Environment

### Python Version Requirement

The pioarduino platform requires **Python 3.10 - 3.13**. Python 3.14+ is not yet supported.

### PlatformIO Installation

If you have Python 3.14+ as your system default, you must install PlatformIO using Python 3.13:

```bash
# Uninstall homebrew PlatformIO (if using Python 3.14)
brew uninstall platformio

# Install PlatformIO with Python 3.13 via pipx
PIPX_DEFAULT_PYTHON=/opt/homebrew/Cellar/python@3.13/3.13.11_1/bin/python3.13 pipx install platformio
```

PlatformIO will be installed to: `~/.local/bin/pio`

### Required Python Dependencies

The pioarduino platform requires additional Python packages in the PlatformIO virtual environment:

```bash
# Install required dependencies
~/.local/pipx/venvs/platformio/bin/python -m pip install littlefs-python
~/.local/pipx/venvs/platformio/bin/python -m pip install fatfs-ng
~/.local/pipx/venvs/platformio/bin/python -m pip install pyyaml
```

**These are required** or the build will fail with `ModuleNotFoundError`.

## Makefile Configuration

The Makefile uses the pipx-installed PlatformIO:

```makefile
PIO := $(HOME)/.local/bin/pio
```

If you installed PlatformIO differently, update this path.

## Troubleshooting

### Error: "Python version must be between 3.10 and 3.13"

**Cause**: PlatformIO is running on Python 3.14+

**Solution**: Reinstall PlatformIO with Python 3.13 (see above)

### Error: "ModuleNotFoundError: No module named 'littlefs'"

**Cause**: Missing Python dependency in PlatformIO venv

**Solution**: Install the missing module:
```bash
~/.local/pipx/venvs/platformio/bin/python -m pip install littlefs-python
```

Repeat for `fatfs-ng` and `pyyaml` if needed.

### Error: "The program size exceeds maximum allowed"

**Cause**: Default partition scheme too small

**Solution**: Ensure `board_build.partitions = huge_app.csv` is in platformio.ini

### Error: "HomeSpan requires version 3.3.0 or greater"

**Cause**: Using official espressif32 platform (only supports Arduino-ESP32 2.x)

**Solution**: Use pioarduino platform URL as specified in platformio.ini

## Library Versions

- **FastLED**: ^3.9.15
- **HomeSpan**: ^2.1.7
- **Arduino-ESP32**: 3.3.6 (via pioarduino platform)

## Build Commands

```bash
make clean         # Clean build artifacts
make build         # Build the project
make flash         # Flash to device
make monitor       # Monitor serial output
make flash-monitor # Flash and monitor
```

## References

- [pioarduino Platform](https://github.com/pioarduino/platform-espressif32)
- [HomeSpan Library](https://github.com/HomeSpan/HomeSpan)
- [FastLED Library](https://github.com/FastLED/FastLED)
