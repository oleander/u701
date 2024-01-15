
set shell := ["zsh", "-cu"]
set dotenv-load := true

UPLOAD_PORT := `ls /dev/tty.usb* | head -n 1`
ESPUP_PATH := "tmp/espup.sh"

clean:
    rm -rf .pio .embuild target
    cargo clean
    cargo pio exec -- run --target clean -e $ENVIRONMENT
    cargo pio exec -- run --target clean
erase:
    esptool.py erase_region 0x9000 0x5000 # nvs
monitor:
    tools/monitor.sh --port {{UPLOAD_PORT}}
update:
    cargo pio exec -- pkg update
setup:
    espup install -t $MCU -f {{ESPUP_PATH}}
    espup update -t $MCU -f {{ESPUP_PATH}}

test: setup
    . {{ESPUP_PATH}} && cargo test

# menuconfig release | debug
menuconfig mod = "release": setup
    . {{ESPUP_PATH}} && cargo pio espidf menuconfig {{ if mod == "release" { "-r true" } else { "" } }}

# upload release | debug
upload $ENVIRONMENT = "release": setup
    . {{ESPUP_PATH}} && cargo pio exec -- run -t upload -e $ENVIRONMENT --upload-port {{UPLOAD_PORT}} --monitor-port {{UPLOAD_PORT}}

# build release | debug
build mod = "release": setup
    . {{ESPUP_PATH}} && cargo pio build {{ if mod == "release" { "-r" } else { "" } }}

install: upload monitor
