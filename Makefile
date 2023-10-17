.PHONY: build clean upload monitor default erase release

PORT := $(shell ls /dev/* | grep "tty.usbserial" | head -n 1)
# PORT := "/dev/cu.usbserial-10"
# do not override ENVIROMENT
ENVIROMENT ?= "release"

build:
	cargo pio build --release
clean:
	cargo clean
	pio run --target clean -e $(ENVIROMENT)
upload:
	cargo pio exec -- run --target upload -e $(ENVIROMENT) --monitor-port $(PORT)
flash:
	cargo pio exec -- run --target upload -e $(ENVIROMENT) --monitor-port $(PORT)
erase: build
	espflash flash \
		--erase-parts nvs,phy_init \
		--partition-table partitions.csv \
		--baud 921600 \
		--port $(PORT) \
		.pio/build/${ENVIROMENT}/firmware.elf
monitor:
	espflash monitor -b 115200 -p $(PORT)
release: clean erase flash monitor

default: upload monitor
