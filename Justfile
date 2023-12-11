# Variables
PORT := `ls /dev/* | grep "tty.usbserial" | head -n 1`
ENVIROMENT := "release"

# Recipes
clean:
    cargo clean
    cargo clean -r
    pio run --target clean -e {{ENVIROMENT}}
    pio run --target clean

build:
    cargo pio build -r

upload:
    cargo pio exec -- run -t upload -e release

ota:
    cargo pio exec -- run -t upload -e ota

erase:
    # esptool.py erase_region 0x9000 0x5000

monitor:
    # cargo pio exec -- run -t monitor -e release
    # espflash monitor --port /dev/cu.usbserial-10 --baud 115200
    tools/monitor.sh --port /dev/cu.usbserial-10 --baud 115200

menuconfig:
    cargo pio espidf menuconfig -r true

update:
    pio pkg update
