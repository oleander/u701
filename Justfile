
set shell := ["bash", "-cu"]
set dotenv-load := true

UPLOAD_PORT := `ls /dev/tty.usb* 2>/dev/null | head -n 1 || echo "/dev/ttyUSB0"`
ESPUP_PATH := "tmp/espup.sh"

clean:
    rm -rf .pio .embuild target
    cargo clean
    cargo +esp pio exec -- run --target clean -e $ENVIRONMENT
    cargo +esp pio exec -- run --target clean
erase:
    esptool.py erase_region 0x9000 0x5000 # nvs
monitor:
    tools/monitor.sh --port {{UPLOAD_PORT}}
update:
    cargo +esp pio exec -- pkg update
setup:
    espup install -t esp32 -f {{ESPUP_PATH}}
    espup update -f {{ESPUP_PATH}}

test: setup
    . {{ESPUP_PATH}} && cargo test

# menuconfig release | debug
menuconfig mod = "release": setup
    . {{ESPUP_PATH}} && cargo pio espidf menuconfig {{ if mod == "release" { "-r true" } else { "" } }}

# upload release | debug
upload:
    . {{ESPUP_PATH}} && cargo +esp pio exec -- run -t upload

# build release | debug | ci
build mod = "release": setup
    . {{ESPUP_PATH}} && cargo pio build {{ if mod == "release" { "-r" } else if mod == "ci" { "--no-debug-info --force-smart-build --build-cache" } else { "" } }}

ota mod = "release": setup
    . {{ESPUP_PATH}} && cargo +esp pio exec -- run -t upload -e ota

fmt: setup
    . {{ESPUP_PATH}} && cargo fmt --all
    cargo fmt --package machine
    . {{ESPUP_PATH}} && RUSTUP_TOOLCHAIN=esp cargo clippy --workspace --all-targets --all-features --target xtensa-esp32-espidf --fix --allow-dirty
    cd machine && cargo clippy --all-targets --all-features --fix --allow-dirty

install: upload monitor

# Docker commands
docker-build:
    docker/dev.sh build

docker-dev:
    docker/dev.sh dev

docker-shell:
    docker/dev.sh shell

docker-build-project:
    docker/dev.sh build-project

docker-test:
    docker/dev.sh test

docker-upload:
    docker/dev.sh upload

docker-monitor:
    docker/dev.sh monitor

docker-install:
    docker/dev.sh install

docker-ota:
    docker/dev.sh ota

docker-clean:
    docker/dev.sh clean

docker-logs:
    docker/dev.sh logs

docker-stop:
    docker/dev.sh stop
