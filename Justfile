UPLOAD_PORT := `ls /dev/* | grep "tty.usbserial" | head -n 1`
ENVIRONMENT := "release"
PARALLEL := "4"

set shell := ["zsh", "-cu"]
set dotenv-load := true

clean:
    cargo clean
    cargo pio exec -- run --target clean -e {{ENVIRONMENT}}
    cargo pio exec -- run --target clean

build:
    cargo pio exec -- run -j {{PARALLEL}} -e {{ENVIRONMENT}}

upload:
    cargo pio exec -- run -t upload -e {{ENVIRONMENT}} --upload-port {{UPLOAD_PORT}}

ota:
    cargo pio exec -- run -t upload -e ota -j {{PARALLEL}}

erase:
    # esptool.py erase_region 0x9000 0x5000

monitor:
    tools/monitor.sh --port {{UPLOAD_PORT}} --baud 115200

menuconfig:
    cargo pio espidf menuconfig -r true

update:
    cargo pio exec -- pkg update
