FROM espressif/idf-rust:esp32_1.88.0.0

RUN rustup toolchain install nightly
RUN rustup component add rust-src --toolchain nightly
RUN rustup default nightly
RUN rustup override set nightly

RUN curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
RUN cargo binstall -y cargo-pio espflash

WORKDIR /app

COPY Cargo.* .
COPY machine/Cargo.* machine/

RUN cargo build --workspace

COPY . .

ENTRYPOINT ["cargo"]
