# ESP32 development environment using Rust base image
FROM rust:1.75

# Install system dependencies for ESP32 development
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    pkg-config \
    libudev-dev \
    libssl-dev \
    cmake \
    ninja-build \
    dfu-util \
    libusb-1.0-0-dev \
    udev \
    && rm -rf /var/lib/apt/lists/*

# Install PlatformIO and ESP32 tools
RUN pip3 install platformio && \
    cargo install espup cargo-pio just && \
    espup install --targets esp32,esp32c3

# Set up environment
ENV ESPUP_PATH="/tmp/espup.sh"
WORKDIR /workspace

# Default command
CMD ["/bin/bash"]