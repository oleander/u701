FROM espressif/idf:latest

# Install Rust and required components
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Install required Rust components
RUN rustup toolchain install nightly && \
    rustup component add rust-src --toolchain nightly

# Install espflash for flashing and monitoring (when hardware is available)
RUN cargo install espflash

# Set working directory
WORKDIR /app

# Set nightly toolchain as default
RUN rustup default nightly

# Pre-install cargo-pio
RUN cargo install cargo-pio

# Ensure ESP-IDF environment is always available by adding to .bashrc
RUN if [ -n "$IDF_PATH" ] && [ -f "$IDF_PATH/export.sh" ]; then \
        echo ". $IDF_PATH/export.sh > /dev/null 2>&1" >> /root/.bashrc; \
    fi

# Copy source code
COPY . .

# Set nightly toolchain for this directory
RUN rustup override set nightly

# Default command
CMD ["bash"]
