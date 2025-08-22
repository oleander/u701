# syntax=docker/dockerfile:1.7-labs
FROM espressif/idf-rust:esp32_1.88.0.0

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Build args for user ID (defaults to common esp user)
ARG USER_UID=1000
ARG USER_GID=1000

# Set HOME and use user-specific locations + faster crate index
ENV HOME=/home/esp \
    RUSTUP_HOME=/home/esp/.rustup \
    CARGO_HOME=/home/esp/.cargo \
    CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse \
    PATH=/home/esp/.cargo/bin:$PATH

# Always at the top, as it does not change often
RUN rustup toolchain install nightly --profile minimal -c rust-src -c rustfmt -c clippy
RUN rustup default nightly
RUN curl -fsSL https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN cargo binstall -y cargo-pio

WORKDIR /app

# ---- dependency caching: copy manifests first ----
COPY Cargo.toml Cargo.lock ./
COPY machine/Cargo.toml machine/

ENV CARGO_HOME=/home/esp/.cargo
RUN mkdir -p $CARGO_HOME

RUN cargo fetch

# ---- now copy sources and build ----
COPY . .

RUN rustup target add xtensa-esp32-espidf
ENV CARGO_BUILD_TARGET=xtensa-esp32-espidf
ENV CARGO_TARGET_DIR=/home/esp/.cargo/target

RUN cargo build --workspace

# SHELL ["/bin/bash", "--rcfile", "/home/esp/.bashrc"]
ENTRYPOINT ["cargo"]
