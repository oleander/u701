# syntax=docker/dockerfile:1.8
FROM espressif/idf-rust:esp32_1.88.0.0

ARG RUST_TOOLCHAIN=nightly
ARG CARGO_PIO_VERSION=latest

ENV HOME=/home/esp \
    RUSTUP_HOME=/home/esp/.rustup \
    CARGO_HOME=/home/esp/.cargo \
    PATH=/home/esp/.cargo/bin:$PATH \
    CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse \
    PLATFORMIO_CORE_DIR=/home/esp/.platformio \
    PLATFORMIO_INSTALLER_TMPDIR=/home/esp/.pio-cache-dir

# Create /app directory and change ownership to esp user
USER root
RUN mkdir -p /app && chown esp:esp /app
USER esp

# Make cache dirs once; reuse via --mount=type=cache in later RUNs
RUN mkdir -p \
  /home/esp/.cargo/registry /home/esp/.cargo/git /home/esp/.rustup \
  /home/esp/.platformio /home/esp/.pio-cache-dir /app/target

# Toolchain + cargo-pio (single layer, cached)
RUN --mount=type=cache,id=rustup,target=/home/esp/.rustup,uid=1000,gid=1000 \
    --mount=type=cache,id=cargo-reg,target=/home/esp/.cargo/registry,uid=1000,gid=1000 \
    --mount=type=cache,id=cargo-git,target=/home/esp/.cargo/git,uid=1000,gid=1000 \
    curl -fsSL https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash && \
    rustup toolchain install ${RUST_TOOLCHAIN} --profile minimal -c rust-src -c rustfmt -c clippy && \
    rustup default ${RUST_TOOLCHAIN} && \
    cargo binstall -y cargo-pio

# PlatformIO core (install via pip; cached)
RUN --mount=type=cache,id=pio-core,target=/home/esp/.platformio,uid=1000,gid=1000 \
    pip3 install --user --break-system-packages platformio

WORKDIR /app

# Copy ONLY manifests to prime Cargo layer cache
COPY Cargo.toml Cargo.lock ./
COPY machine/Cargo.toml machine/

# Pre-fetch crates into cache (doesn't depend on source changes)
RUN --mount=type=cache,id=cargo-reg,target=/home/esp/.cargo/registry,uid=1000,gid=1000 \
    --mount=type=cache,id=cargo-git,target=/home/esp/.cargo/git,uid=1000,gid=1000 \
    --mount=type=cache,id=target-cache,target=/app/target,uid=1000,gid=1000 \
    cargo fetch

# Prime PIO packages once, cache them
COPY platformio.ini .
RUN --mount=type=cache,id=pio-core,target=/home/esp/.platformio,uid=1000,gid=1000 \
    cargo pio installpio ${PLATFORMIO_INSTALLER_TMPDIR}

# Source last so edits don’t invalidate deps
COPY . .

# Build using reusable caches for cargo, target, and PlatformIO
RUN --mount=type=cache,id=cargo-reg,target=/home/esp/.cargo/registry,uid=1000,gid=1000 \
    --mount=type=cache,id=cargo-git,target=/home/esp/.cargo/git,uid=1000,gid=1000 \
    --mount=type=cache,id=target-cache,target=/app/target,uid=1000,gid=1000 \
    --mount=type=cache,id=pio-core,target=/home/esp/.platformio,uid=1000,gid=1000 \
    cargo pio build --pio-installation ${PLATFORMIO_INSTALLER_TMPDIR}
