UPLOAD_PORT := `ls /dev/* | grep "tty.usb" | head -n 1`
ENVIRONMENT := "release"
PARALLEL := "4"
MONITOR_SPEED := "115200"

set shell := ["zsh", "-cu"]
set dotenv-load := true

clean:
    cargo clean
    cargo pio exec -- run --target clean -e {{ENVIRONMENT}}
    cargo pio exec -- run --target clean

build:
    cargo pio build -r

upload:
    . ./.espup.sh && cargo pio exec -- run -t upload -e {{ENVIRONMENT}} --upload-port {{UPLOAD_PORT}} --monitor-port {{UPLOAD_PORT}}

ota:
    cargo pio exec -- run -t upload -e ota -j {{PARALLEL}}

erase:
    esptool.py erase_region 0x9000 0x5000 # nvs

monitor:
    tools/monitor.sh --port {{UPLOAD_PORT}} --baud {{MONITOR_SPEED}}

menuconfig:
    cargo pio espidf menuconfig -r true

update:
    cargo pio exec -- pkg update
setup:
    espup install -t esp32c3 -f .espup.sh

test:
    source ./.espup.sh && cargo test
