#!/bin/bash
set -e

# Function to setup ESP environment with robust retry logic
setup_esp() {
    local max_retries=3
    local retry_count=0
    
    while [ $retry_count -lt $max_retries ]; do
        if [ -f "/root/.espup/export-esp.sh" ]; then
            echo "ESP environment already set up"
            return 0
        fi
        
        echo "Setting up ESP development environment (attempt $((retry_count + 1))/$max_retries)..."
        
        # Method 1: Try specific stable version with wget (most reliable)
        echo "Downloading espup v0.15.1..."
        if wget --timeout=60 --tries=3 --retry-connrefused --waitretry=10 \
            -O /tmp/espup https://github.com/esp-rs/espup/releases/download/v0.15.1/espup-x86_64-unknown-linux-gnu; then
            chmod +x /tmp/espup
            mv /tmp/espup /usr/local/bin/espup
            
            echo "Installing ESP toolchain..."
            # Use specific targets to reduce download size and improve reliability
            if espup install --targets esp32,esp32c3 --log-level info; then
                echo "ESP environment setup completed successfully"
                return 0
            else
                echo "ESP toolchain installation failed"
            fi
        else
            echo "Failed to download espup"
        fi
        
        retry_count=$((retry_count + 1))
        if [ $retry_count -lt $max_retries ]; then
            echo "Retrying in 20 seconds..."
            sleep 20
        fi
    done
    
    echo "Failed to set up ESP environment after $max_retries attempts"
    return 1
}

# Check if this is a pio command that needs ESP environment
if [ "$1" = "pio" ] || [[ "$*" == *"pio"* ]]; then
    setup_esp
    . /root/.espup/export-esp.sh
fi

# Execute the original cargo command
exec /usr/local/cargo/bin/cargo "$@"