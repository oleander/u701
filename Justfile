set dotenv-load := true

DOCKER_IMAGE := "u701:latest"
UPLOAD_PORT := env_var_or_default("UPLOAD_PORT", "/dev/ttyUSB0")

_build-image:
    docker build -t {{DOCKER_IMAGE}} .

_docker-esp args:
    docker run --rm -v $(pwd):/app -w /app {{DOCKER_IMAGE}} bash -c "\
        wget -O /tmp/espup https://github.com/esp-rs/espup/releases/download/v0.14.0/espup-x86_64-unknown-linux-gnu && \
        chmod +x /tmp/espup && \
        mv /tmp/espup /usr/local/bin/espup && \
        espup install && \
        . /root/.espup/export-esp.sh && \
        {{args}}"

clean: _build-image
    docker run --rm -v $(pwd):/app -w /app {{DOCKER_IMAGE}} bash -c "\
        rm -rf .pio .embuild target && \
        cargo clean"

erase:
    @echo "Hardware operations require local execution with proper device access"
    esptool.py erase_region 0x9000 0x5000 # nvs

monitor:
    @echo "Monitor requires direct hardware access"
    tools/monitor.sh --port {{UPLOAD_PORT}}

update: 
    @just _docker-esp "cargo pio exec -- pkg update"

setup: 
    @echo "Docker image setup complete. ESP toolchain will be installed on-demand."

# Test command with fallback to local execution if Docker fails
test:
    #!/usr/bin/env bash
    set -euo pipefail
    
    # Try Docker first, fall back to local if it fails
    if docker build -t {{DOCKER_IMAGE}} . > /dev/null 2>&1; then
        echo "Using Docker for tests..."
        docker run --rm -v $(pwd):/app -w /app {{DOCKER_IMAGE}} bash -c "\
            if [ -f .cargo/config.toml ]; then mv .cargo/config.toml .cargo/config.toml.bak; fi && \
            cargo test --workspace && \
            if [ -f .cargo/config.toml.bak ]; then mv .cargo/config.toml.bak .cargo/config.toml; fi"
    else
        echo "Docker build failed, using local Rust toolchain for tests..."
        # Ensure nightly toolchain is available
        if ! rustup toolchain list | grep -q nightly; then
            echo "Installing nightly Rust toolchain..."
            rustup toolchain install nightly
        fi
        rustup override set nightly
        
        # Temporarily move cargo config if it exists
        if [ -f .cargo/config.toml ]; then 
            mv .cargo/config.toml .cargo/config.toml.bak
        fi
        
        # Run tests
        cargo test --workspace
        
        # Restore cargo config
        if [ -f .cargo/config.toml.bak ]; then 
            mv .cargo/config.toml.bak .cargo/config.toml
        fi
    fi

# menuconfig release | debug
menuconfig mod = "release": _build-image
    docker run --rm -it -v $(pwd):/app -w /app {{DOCKER_IMAGE}} bash -c "\
        wget -O /tmp/espup https://github.com/esp-rs/espup/releases/download/v0.14.0/espup-x86_64-unknown-linux-gnu && \
        chmod +x /tmp/espup && \
        mv /tmp/espup /usr/local/bin/espup && \
        espup install && \
        . /root/.espup/export-esp.sh && \
        cargo pio espidf menuconfig {{ if mod == "release" { "-r true" } else { "" } }}"

# upload release | debug  
upload: _build-image
    @echo "Upload requires hardware access - using privileged Docker container"
    docker run --rm -v $(pwd):/app -w /app --privileged -v /dev:/dev {{DOCKER_IMAGE}} bash -c "\
        wget -O /tmp/espup https://github.com/esp-rs/espup/releases/download/v0.14.0/espup-x86_64-unknown-linux-gnu && \
        chmod +x /tmp/espup && \
        mv /tmp/espup /usr/local/bin/espup && \
        espup install && \
        . /root/.espup/export-esp.sh && \
        cargo pio exec -- run -t upload"

# build release | debug
build mod = "release": 
    @just _docker-esp "cargo pio build {{ if mod == "release" { "-r" } else { "" } }}"

ota mod = "release": 
    @just _docker-esp "cargo pio exec -- run -t upload -e ota"

install: upload monitor
