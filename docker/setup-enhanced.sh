#!/bin/bash
# Enhanced ESP32 development environment setup for Docker

set -e

echo "=== ESP32 Development Environment Setup ==="
echo ""

# Set up environment variables
export PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"
export CARGO_HOME="$HOME/.cargo"
export PLATFORMIO_CORE_DIR="$HOME/.platformio"
export ESPUP_PATH="$HOME/tmp/espup.sh"

# Create necessary directories
mkdir -p "$HOME/.cargo" "$HOME/.platformio" "$HOME/tmp"

echo "✓ Environment variables configured"

# Check if Rust is installed, install if not
if ! command -v cargo &> /dev/null; then
    echo "⚙️  Installing Rust for user..."
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.75.0
    source "$HOME/.cargo/env"
    echo "✓ Rust installed"
else
    echo "✓ Rust already available"
fi

# Install Rust tools
echo "⚙️  Installing ESP32 Rust tools..."
cargo install espup cargo-pio just 2>/dev/null || true
echo "✓ Rust tools installed"

# Set up ESP32 toolchain
echo "⚙️  Setting up ESP32 toolchain..."
espup install --targets esp32,esp32c3 --log-level info 2>/dev/null || {
    echo "⚠️  ESP32 toolchain installation failed, will retry later"
}

# Verify installations
echo ""
echo "=== Verification ==="
command -v pio && echo "✓ PlatformIO: $(pio --version)"
command -v cargo && echo "✓ Rust: $(cargo --version)"
command -v just && echo "✓ Just: $(just --version)" || echo "⚠️  Just not available"

# Create environment file
cat > "$HOME/.bashrc_esp32" << 'EOF'
# ESP32 Development Environment
export PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"
export CARGO_HOME="$HOME/.cargo"
export PLATFORMIO_CORE_DIR="$HOME/.platformio"
export ESPUP_PATH="$HOME/tmp/espup.sh"

# Source ESP32 environment if available
if [ -f "$ESPUP_PATH" ]; then
    source "$ESPUP_PATH"
fi

# Aliases for convenience
alias pio-list="pio device list"
alias pio-monitor="pio device monitor"
alias esp32-build="just build"
alias esp32-upload="just upload"
alias esp32-monitor="just monitor"

echo "ESP32 development environment loaded!"
EOF

# Add to .bashrc
if ! grep -q "source.*\.bashrc_esp32" "$HOME/.bashrc" 2>/dev/null; then
    echo "source ~/.bashrc_esp32" >> "$HOME/.bashrc"
fi

echo ""
echo "🎉 Setup complete!"
echo ""
echo "To use the environment:"
echo "  source ~/.bashrc_esp32"
echo ""
echo "Available commands:"
echo "  pio --help         - PlatformIO commands"
echo "  cargo --help       - Rust commands"
echo "  just --list        - Available just commands"
echo "  esp32-build        - Build the project"
echo "  esp32-upload       - Upload to ESP32"
echo "  esp32-monitor      - Monitor serial output"