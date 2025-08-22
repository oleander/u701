FROM espressif/idf-rust:esp32_1.88.0.0

# Build arguments for configuration
ARG RUST_TOOLCHAIN=nightly
ARG USER_NAME=esp
ARG USER_UID=1000
ARG USER_GID=1000
ARG APP_DIR=/app
ARG HOME_DIR=/home/esp

# Environment variables derived from args
ENV HOME=${HOME_DIR} \
    RUSTUP_HOME=${HOME_DIR}/.rustup \
    CARGO_HOME=${HOME_DIR}/.cargo \
    PATH=${HOME_DIR}/.cargo/bin:$PATH \
    CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse \
    PLATFORMIO_CORE_DIR=${HOME_DIR}/.platformio \
    PLATFORMIO_INSTALLER_TMPDIR=${HOME_DIR}/.pio-cache-dir \
    USER_UID=${USER_UID} \
    USER_GID=${USER_GID} \
    APP_DIR=${APP_DIR}

# Setup directories and permissions in single layer
USER root
RUN mkdir -p ${APP_DIR} ${HOME_DIR}/.cargo/registry ${HOME_DIR}/.cargo/git ${HOME_DIR}/.rustup \
             ${HOME_DIR}/.platformio ${HOME_DIR}/.pio-cache-dir ${APP_DIR}/target && \
    chown ${USER_NAME}:${USER_NAME} ${APP_DIR}
USER ${USER_NAME}

RUN echo "OK"
# Toolchain + cargo-pio (single layer, cached)
RUN --mount=type=cache,id=rustup,target=${RUSTUP_HOME},uid=${USER_UID},gid=${USER_GID} \
    --mount=type=cache,id=cargo-reg,target=${CARGO_HOME}/registry,uid=${USER_UID},gid=${USER_GID} \
    --mount=type=cache,id=cargo-git,target=${CARGO_HOME}/git,uid=${USER_UID},gid=${USER_GID} \
    curl -fsSL https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash && \
    rustup toolchain install ${RUST_TOOLCHAIN} --profile minimal -c rust-src -c rustfmt -c clippy && \
    rustup default ${RUST_TOOLCHAIN} && \
    cargo binstall -y cargo-pio

# Install PlatformIO via pip (needed for cargo-pio)
RUN --mount=type=cache,id=pio-core,target=${PLATFORMIO_CORE_DIR},uid=${USER_UID},gid=${USER_GID} \
    pip3 install --user --break-system-packages platformio

WORKDIR ${APP_DIR}

# Copy ONLY manifests to prime Cargo layer cache
COPY Cargo.toml Cargo.lock ./
COPY machine/Cargo.toml machine/

# Pre-fetch crates into cache (doesn't depend on source changes)
RUN --mount=type=cache,id=cargo-reg,target=${CARGO_HOME}/registry,uid=${USER_UID},gid=${USER_GID} \
    --mount=type=cache,id=cargo-git,target=${CARGO_HOME}/git,uid=${USER_UID},gid=${USER_GID} \
    --mount=type=cache,id=target-cache,target=${APP_DIR}/target,uid=${USER_UID},gid=${USER_GID} \
    cargo fetch

# Prime PIO packages once, cache them
COPY platformio.ini .
USER root
RUN mkdir -p ${PLATFORMIO_INSTALLER_TMPDIR} && \
    chown -R ${USER_NAME}:${USER_NAME} ${PLATFORMIO_INSTALLER_TMPDIR}
USER ${USER_NAME}
RUN cargo pio installpio ${PLATFORMIO_INSTALLER_TMPDIR}

# Build using reusable caches for cargo, target, and PlatformIO
ENV PATH=${PLATFORMIO_INSTALLER_TMPDIR}/penv/bin:$PATH
RUN --mount=type=cache,id=cargo-reg,target=${CARGO_HOME}/registry,uid=${USER_UID},gid=${USER_GID} \
    --mount=type=cache,id=cargo-git,target=${CARGO_HOME}/git,uid=${USER_UID},gid=${USER_GID} \
    --mount=type=cache,id=target-cache,target=${APP_DIR}/target,uid=${USER_UID},gid=${USER_GID} \
    --mount=type=cache,id=pio-core,target=${PLATFORMIO_CORE_DIR},uid=${USER_UID},gid=${USER_GID} \
    cargo pio build --pio-installation ${PLATFORMIO_INSTALLER_TMPDIR}

# Source last so edits don't invalidate deps
COPY . .

RUN echo "/home/esp/export-esp.sh" > /home/esp/.bashrc
ENTRYPOINT ["/bin/bash", "-l"]
