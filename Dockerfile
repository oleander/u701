FROM rust:latest

# Install system dependencies
RUN apt-get update && apt-get install -y \
    git \
    curl \
    build-essential \
    libudev-dev \
    pkg-config \
    python3 \
    python3-pip \
    expect \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install required Rust components
RUN rustup toolchain install nightly \
    && rustup component add rust-src --toolchain nightly

# Install espflash for flashing and monitoring (when hardware is available)
RUN cargo install espflash

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Copy the ESP setup script that acts as a cargo wrapper
COPY setup-esp.sh /usr/local/bin/cargo
RUN chmod +x /usr/local/bin/cargo

# Set nightly toolchain for this directory
RUN rustup override set nightly

# Default command
CMD ["bash"]