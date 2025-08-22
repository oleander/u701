#!/bin/bash
# Test script for u701 Docker development environment

set -e

echo "=== u701 Docker Development Test ==="
echo ""

# Source environment
source ~/.bashrc_esp32 2>/dev/null || {
    echo "⚙️  Setting up environment..."
    /home/esp32/setup.sh
    source ~/.bashrc_esp32
}

echo "📁 Current directory: $(pwd)"
echo "🔧 Available tools:"
echo "   - PlatformIO: $(which pio 2>/dev/null && pio --version || echo 'Not found')"
echo "   - Rust: $(which cargo 2>/dev/null && cargo --version || echo 'Not found')"
echo "   - Just: $(which just 2>/dev/null && just --version || echo 'Not found')"
echo ""

# Test if we're in the project directory
if [ -f "Cargo.toml" ] && [ -f "platformio.ini" ]; then
    echo "✓ Found u701 project files"
    
    # Try to run tests
    echo ""
    echo "🧪 Running Rust tests..."
    if cargo test --no-default-features 2>&1; then
        echo "✅ Tests passed!"
    else
        echo "⚠️  Tests failed or not available"
    fi
    
    # Try to build (without hardware)
    echo ""
    echo "🔨 Testing build (simulation)..."
    if command -v just &> /dev/null; then
        echo "Using just commands:"
        just --list || echo "No just commands available"
    else
        echo "Just not available, trying cargo directly:"
        if cargo check 2>&1; then
            echo "✅ Cargo check passed!"
        else
            echo "⚠️  Cargo check failed"
        fi
    fi
    
else
    echo "⚠️  Not in u701 project directory or files missing"
    echo "Expected files: Cargo.toml, platformio.ini"
fi

echo ""
echo "🎯 Development environment ready!"
echo "   Run './docker/dev.sh shell' to access this environment"