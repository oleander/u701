# syntax=docker/dockerfile:1.7-labs
FROM espressif/idf-rust:esp32_1.88.0.0

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Set HOME and use user-specific locations + faster crate index
ENV HOME=/home/esp \
    RUSTUP_HOME=/home/esp/.rustup \
    CARGO_HOME=/home/esp/.cargo \
    CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse \
    PATH=/home/esp/.cargo/bin:$PATH

# Create cargo/rustup directories with proper permissions
RUN mkdir -p /home/esp/.cargo /home/esp/.rustup \
 && chown -R $(id -u):$(id -g) /home/esp/.cargo /home/esp/.rustup

# Nightly in one shot (adds rust-src; rustfmt/clippy are handy but optional)
RUN rustup toolchain install nightly --profile minimal -c rust-src -c rustfmt -c clippy \
 && rustup default nightly

# Fast binaries via cargo-binstall, with fallback to cargo install
RUN --mount=type=cache,target=/home/esp/.cargo/registry \
    --mount=type=cache,target=/home/esp/.cargo/git \
    (command -v cargo-binstall >/dev/null || \
      curl -fsSL https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash) \
 && for bin in cargo-pio espflash; do \
      cargo binstall -y "$bin" || cargo install "$bin"; \
    done

WORKDIR /app

# ---- dependency caching: copy manifests first ----
COPY Cargo.toml Cargo.lock ./
COPY machine/Cargo.toml machine/

# Pre-resolve deps (keeps this layer hot unless manifests change)
RUN --mount=type=cache,target=/home/esp/.cargo/registry \
    --mount=type=cache,target=/home/esp/.cargo/git \
    cargo fetch

# ---- now copy sources and build ----
COPY . .

RUN --mount=type=cache,target=/home/esp/.cargo/registry \
    --mount=type=cache,target=/home/esp/.cargo/git \
    --mount=type=cache,target=/app/target \
    cargo build --workspace

ENTRYPOINT ["cargo"]
