# Multi-stage Dockerfile for ESP32 development with Rust and PlatformIO
# Based on Ubuntu for better compatibility with ESP32 tools

ARG RUST_VERSION=1.75.0
ARG UBUNTU_VERSION=22.04

# Base stage with system dependencies
FROM ubuntu:${UBUNTU_VERSION} as base

# Install system dependencies
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    git \
    build-essential \
    pkg-config \
    libudev-dev \
    libssl-dev \
    python3 \
    python3-pip \
    python3-venv \
    cmake \
    ninja-build \
    ccache \
    libffi-dev \
    libssl-dev \
    dfu-util \
    libusb-1.0-0-dev \
    udev \
    && rm -rf /var/lib/apt/lists/*

# Create user for development
RUN useradd -m -s /bin/bash esp32 && \
    usermod -a -G dialout esp32

# Rust toolchain stage
FROM base as rust-stage

# Install Rust
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain ${RUST_VERSION}
ENV PATH="/root/.cargo/bin:${PATH}"

# Install espup for ESP32 Rust support
RUN cargo install espup

# Install ESP32 Rust toolchain
RUN espup install --targets esp32,esp32c3 --log-level info

# PlatformIO stage
FROM rust-stage as platformio-stage

# Install PlatformIO
RUN python3 -m pip install --upgrade pip && \
    python3 -m pip install platformio

# Install cargo-pio for Rust-PlatformIO integration
RUN cargo install cargo-pio

# Install just command runner
RUN cargo install just

# Development stage
FROM platformio-stage as development

# Switch to esp32 user
USER esp32
WORKDIR /home/esp32

# Copy espup environment for esp32 user
RUN espup install --targets esp32,esp32c3 --log-level info

# Set up environment
ENV PATH="/home/esp32/.cargo/bin:${PATH}"
ENV ESPUP_PATH="/home/esp32/tmp/espup.sh"

# Create working directory
RUN mkdir -p /home/esp32/workspace

# Production build stage
FROM development as builder

# Copy source code
COPY --chown=esp32:esp32 . /home/esp32/workspace/
WORKDIR /home/esp32/workspace

# Build the project
RUN mkdir -p tmp && \
    espup install -t esp32 -f ${ESPUP_PATH} && \
    . ${ESPUP_PATH} && \
    cargo pio build -r

# Final development image
FROM development as final

# Install additional development tools
USER root
RUN apt-get update && apt-get install -y \
    vim \
    nano \
    htop \
    screen \
    tmux \
    && rm -rf /var/lib/apt/lists/*

# Copy udev rules for ESP32 devices
COPY docker/99-esp32.rules /etc/udev/rules.d/
RUN udevadm control --reload-rules

USER esp32
WORKDIR /home/esp32/workspace

# Expose common ESP32 ports
EXPOSE 3232

# Default command
CMD ["/bin/bash"]