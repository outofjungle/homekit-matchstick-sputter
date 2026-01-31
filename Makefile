# Makefile for homekit-matchstick-sputter
# PlatformIO wrapper for common development tasks

.PHONY: all build clean erase flash monitor flash-monitor help

# Default target
all: build

# Build the project
build:
	@echo "Building project..."
	pio run

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	pio run --target clean

# Erase flash memory on device
erase:
	@echo "Erasing flash memory..."
	pio run --target erase

# Flash firmware to device
flash:
	@echo "Flashing firmware to device..."
	pio run --target upload

# Monitor serial output
monitor:
	@echo "Starting serial monitor (Ctrl+C to exit)..."
	pio device monitor

# Flash and immediately start monitoring
flash-monitor: flash
	@echo "Starting serial monitor (Ctrl+C to exit)..."
	pio device monitor

# Show help
help:
	@echo "Available targets:"
	@echo "  make build         - Build the project"
	@echo "  make clean         - Clean build artifacts"
	@echo "  make erase         - Erase flash memory on device"
	@echo "  make flash         - Flash firmware to device"
	@echo "  make monitor       - Monitor serial output"
	@echo "  make flash-monitor - Flash and start monitoring"
	@echo "  make help          - Show this help message"
