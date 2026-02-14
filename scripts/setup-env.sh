#!/usr/bin/env bash
set -euo pipefail

# setup-env.sh — Install the full PlatformIO build environment
# Idempotent: safe to run multiple times.

echo "==> Checking for Homebrew..."
if ! command -v brew &>/dev/null; then
    echo "ERROR: Homebrew is not installed. Install it from https://brew.sh and re-run." >&2
    exit 1
fi

echo "==> Installing Python 3.13..."
if brew list python@3.13 &>/dev/null; then
    echo "    python@3.13 already installed, skipping."
else
    brew install python@3.13
fi

echo "==> Installing pipx..."
if brew list pipx &>/dev/null; then
    echo "    pipx already installed, skipping."
else
    brew install pipx
fi
pipx ensurepath

echo "==> Installing PlatformIO via pipx (Python 3.13)..."
PYTHON313="$(brew --prefix python@3.13)/bin/python3.13"
if pipx list | grep -q "platformio"; then
    echo "    platformio already installed, skipping."
else
    pipx install platformio --python "$PYTHON313"
fi

PIO_PYTHON="$HOME/.local/pipx/venvs/platformio/bin/python"

echo "==> Installing pip packages into PlatformIO venv..."
"$PIO_PYTHON" -m pip install --quiet --upgrade \
    littlefs-python \
    fatfs-ng \
    pyyaml \
    qrcode \
    pillow

echo "==> Installing clangd (clang LSP)..."
if brew list llvm &>/dev/null; then
    echo "    llvm already installed, skipping."
else
    brew install llvm
fi

echo "==> Verifying PlatformIO installation..."
"$HOME/.local/bin/pio" --version

echo ""
echo "Setup complete. Next steps:"
echo "  make build"
