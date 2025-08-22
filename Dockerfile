FROM rust:latest

# Install required components
RUN rustup toolchain install nightly \
    && rustup component add rust-src --toolchain nightly

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Set nightly toolchain for this directory
RUN rustup override set nightly

# Temporarily disable ESP32-specific cargo config for testing
RUN if [ -f .cargo/config.toml ]; then mv .cargo/config.toml .cargo/config.toml.bak; fi

# Build and test the project
RUN cargo test --workspace

# Default command to run tests (with config still disabled)
CMD ["cargo", "test", "--workspace"]