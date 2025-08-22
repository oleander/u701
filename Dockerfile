# syntax=docker/dockerfile:1.8
FROM espressif/idf-rust:esp32_1.88.0.0

ENV HOME=/home/esp \
    RUSTUP_HOME=/home/esp/.rustup \
    CARGO_HOME=/home/esp/.cargo \
    CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse \
    PATH=/home/esp/.cargo/bin:$PATH

# Installs nightly toolchain
RUN rustup toolchain install nightly --profile minimal -c rust-src -c rustfmt -c clippy
RUN rustup default nightly

# Installs cargo-pio
RUN curl -fsSL https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN cargo binstall -y cargo-pio


# Fetches all dependencies for the Rust project
COPY Cargo.toml Cargo.lock ./
COPY machine/Cargo.toml machine/
ENV CARGO_HOME=/home/esp/.cargo
RUN mkdir -p $CARGO_HOME
RUN cargo fetch

# Installs PlatformIO (pio)
ENV PLATFORMIO_INSTALLER_TMPDIR=/home/esp/.pio-cache-dir
RUN mkdir -p $PLATFORMIO_INSTALLER_TMPDIR
VOLUME $PLATFORMIO_INSTALLER_TMPDIR
# How to join the two lines (curl and python3)?
RUN curl -fsSL -o installer.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
RUN python3 installer.py

RUN cargo pio installpio $PLATFORMIO_INSTALLER_TMPDIR
COPY platformio.ini .
RUN cargo pio build --pio-installation $PLATFORMIO_INSTALLER_TMPDIR

WORKDIR /app
COPY . .

