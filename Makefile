# Makefile for homekit-matchstick-sputter
# PlatformIO wrapper for common development tasks

.PHONY: all build clean erase flash monitor flash-monitor test help

# PlatformIO binary location
PIO := $(HOME)/.local/bin/pio

# Default target
all: build

# Build the project
build:
	@echo "Building project..."
	$(PIO) run

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	$(PIO) run --target clean

# Erase flash memory on device
erase:
	@echo "Erasing flash memory..."
	$(PIO) run --target erase

# Flash firmware to device
flash:
	@echo "Flashing firmware to device..."
	$(PIO) run --target upload

# Monitor serial output
monitor:
	@echo "Starting serial monitor (Ctrl+C to exit)..."
	$(PIO) device monitor

# Flash and immediately start monitoring
flash-monitor: flash
	@echo "Starting serial monitor (Ctrl+C to exit)..."
	$(PIO) device monitor

# Run native tests
test:
	@echo "Running native tests..."
	$(PIO) test -e native

# Show help
help:
	@echo "Available targets:"
	@echo "  make build         - Build the project"
	@echo "  make clean         - Clean build artifacts"
	@echo "  make erase         - Erase flash memory on device"
	@echo "  make flash         - Flash firmware to device"
	@echo "  make monitor       - Monitor serial output"
	@echo "  make flash-monitor - Flash and start monitoring"
	@echo "  make test          - Run native tests"
	@echo "  make help          - Show this help message"
