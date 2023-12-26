
UPLOAD_PORT := `ls /dev/* | grep "tty.usb" | head -n 1`

set shell := ["zsh", "-cu"]
set dotenv-load := true

clean:
    rm -rf .pio .embuild target
    cargo clean
    cargo pio exec -- run --target clean -e $ENVIRONMENT
    cargo pio exec -- run --target clean
super_clean: clean
    rm -f sdkconfig*
build RELEASE:
    cargo pio build {{RELEASE}}
upload $ENVIRONMENT = "release": setup
    . ./.espup.sh && cargo pio exec -- run -t upload -e $ENVIRONMENT --upload-port {{UPLOAD_PORT}} --monitor-port {{UPLOAD_PORT}}
ota:
    cargo pio exec -- run -t upload -e ota
erase:
    esptool.py erase_region 0x9000 0x5000 # nvs
monitor:
    tools/monitor.sh --port {{UPLOAD_PORT}} --baud $MONITOR_SPEED
menuconfig:
    cargo pio espidf menuconfig -r true
update:
    cargo pio exec -- pkg update
setup:
    espup install -t $MCU -f .espup.sh
test:
    source ./.espup.sh && cargo test
unset_cache:
    unset RUSTC_WRAPPER
redo: super_clean unset_cache upload monitor
try: upload && monitor
    git add .
    git commit --no-edit
install: upload monitor
