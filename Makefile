.PHONY: build clean upload monitor default erase release

PORT := $(shell ls /dev/* | grep "tty.usbserial" | head -n 1)
# PORT := "/dev/cu.usbserial-10"
ENVIROMENT := debug # release, debug, ota

build:
	cargo pio build --release
clean:
	cargo clean
	pio run --target clean -e $(ENVIROMENT)
flash:
	cargo pio exec -- run --target upload -e $(ENVIROMENT)
erase: build
	espflash flash --erase-parts \
		nvs,phy_init .pio/build/${ENVIROMENT}/firmware.elf \
		--partition-table partitions.csv -b 921600 -p $(PORT)
monitor:
	espflash monitor -b 115200 -p $(PORT)
release: clean erase flash monitor

default: upload monitor
