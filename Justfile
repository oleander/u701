
set shell := ["zsh", "-cu"]
set dotenv-load := true

clean:
    rm -rf .pio .embuild target
    cargo clean
    cargo pio exec -- run --target clean -e $ENVIRONMENT
    cargo pio exec -- run --target clean
super_clean: clean
    rm -f sdkconfig*
ota:
    cargo pio exec -- run -t upload -e ota
erase:
    esptool.py erase_region 0x9000 0x5000 # nvs
monitor:
    tools/monitor.sh --baud $MONITOR_SPEED
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

# menuconfig release | debug
menuconfig mod = "release":
    cargo pio espidf menuconfig {{ if mod == "release" { "-r true" } else { "" } }}

# upload release | debug
upload $ENVIRONMENT = "release":
    . ./.espup.sh && cargo pio exec -- run -t upload -e $ENVIRONMENT

# build release | debug
build mod = "release": setup
    cargo pio build {{ if mod == "release" { "-r" } else { "" } }}

update_deps:
    platformio pkg update
    cargo upgrade
    cargo update
    cargo udeps
reload-docker:
    devcontainer build --workspace-folder .
    devcontainer up --workspace-folder .
    devcontainer exec --workspace-folder . ./setup.sh
