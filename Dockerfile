# Final working Dockerfile for u701 ESP32 development
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

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
    dfu-util \
    libusb-1.0-0-dev \
    udev \
    ca-certificates \
    sudo \
    vim \
    nano \
    htop \
    screen \
    tmux \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*

# Create user for development
RUN useradd -m -s /bin/bash esp32 && \
    usermod -a -G dialout esp32 && \
    echo "esp32 ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Switch to esp32 user and set up their environment completely
USER esp32
WORKDIR /home/esp32

# Install Python packages for esp32 user
RUN python3 -m pip install --user platformio

# Install Rust properly for esp32 user
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.75.0

# Source cargo environment and install ESP32 tools
RUN . $HOME/.cargo/env && \
    cargo install espup cargo-pio just && \
    espup install --targets esp32,esp32c3 --log-level info

# Set up environment variables
ENV PATH="/home/esp32/.local/bin:/home/esp32/.cargo/bin:${PATH}"
ENV CARGO_HOME="/home/esp32/.cargo"
ENV ESPUP_PATH="/home/esp32/tmp/espup.sh"
ENV PLATFORMIO_CORE_DIR="/home/esp32/.platformio"

# Create working directory
RUN mkdir -p /home/esp32/workspace

# Create comprehensive environment setup
RUN echo '# ESP32 Development Environment' >> ~/.bashrc && \
    echo 'export PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"' >> ~/.bashrc && \
    echo 'export CARGO_HOME="$HOME/.cargo"' >> ~/.bashrc && \
    echo 'export PLATFORMIO_CORE_DIR="$HOME/.platformio"' >> ~/.bashrc && \
    echo 'export ESPUP_PATH="$HOME/tmp/espup.sh"' >> ~/.bashrc && \
    echo 'if [ -f "$ESPUP_PATH" ]; then source "$ESPUP_PATH"; fi' >> ~/.bashrc

# Copy udev rules (need to switch to root)
USER root
COPY docker/99-esp32.rules /etc/udev/rules.d/99-esp32.rules
RUN chmod 644 /etc/udev/rules.d/99-esp32.rules

# Switch back to esp32 user
USER esp32
WORKDIR /home/esp32/workspace

# Expose common ESP32 ports
EXPOSE 3232

# Default command
CMD ["/bin/bash"]