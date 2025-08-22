
set shell := ["bash", "-cu"]
set dotenv-load := true

# Docker image name
DOCKER_IMAGE := "u701"
UPLOAD_PORT := `ls /dev/tty.usb* 2>/dev/null | head -n 1 || echo "/dev/ttyUSB0"`

# Build Docker image if it doesn't exist
_build-image:
    @if ! docker image ls {{DOCKER_IMAGE}} | grep -q {{DOCKER_IMAGE}}; then \
        echo "Building Docker image..."; \
        docker build -t {{DOCKER_IMAGE}} .; \
    fi

# Run command in Docker container
_docker-run *args: _build-image
    docker run --rm -v $(pwd):/app -w /app {{DOCKER_IMAGE}} {{args}}

# Run command in Docker container with device access for hardware operations
_docker-run-hw *args: _build-image
    docker run --rm -v $(pwd):/app -w /app --privileged -v /dev:/dev {{DOCKER_IMAGE}} {{args}}

# Run cargo command in Docker container
_docker-cargo *args: _build-image
    docker run --rm -v $(pwd):/app -w /app {{DOCKER_IMAGE}} cargo {{args}}

clean: _build-image
    docker run --rm -v $(pwd):/app -w /app {{DOCKER_IMAGE}} sh -c "rm -rf .pio .embuild target && cargo clean"

erase:
    @echo "Hardware operations require local execution with proper device access"
    esptool.py erase_region 0x9000 0x5000 # nvs

monitor:
    @echo "Monitor requires direct hardware access"
    tools/monitor.sh --port {{UPLOAD_PORT}}

update:
    @just _docker-cargo "pio exec -- pkg update"

setup:
    @echo "Docker image setup complete. ESP toolchain will be installed on-demand."

test: _build-image
    docker run --rm -v $(pwd):/app -w /app {{DOCKER_IMAGE}} cargo test --workspace

# menuconfig release | debug
menuconfig mod = "release": _build-image
    docker run --rm -it -v $(pwd):/app -w /app {{DOCKER_IMAGE}} cargo pio espidf menuconfig {{ if mod == "release" { "-r true" } else { "" } }}

# upload release | debug
upload: _build-image
    @echo "Upload requires hardware access - using privileged Docker container"
    docker run --rm -v $(pwd):/app -w /app --privileged -v /dev:/dev {{DOCKER_IMAGE}} cargo pio exec -- run -t upload

# build release | debug
build mod = "release":
    @just _docker-cargo "pio build {{ if mod == "release" { "-r" } else { "" } }}"

ota mod = "release":
    @just _docker-cargo "pio exec -- run -t upload -e ota"

install: upload monitor
