.PHONY: build clean upload monitor default erase release

# PORT := $(shell ls /dev/* | grep "tty.usbserial" | head -n 1)
PORT := "/dev/cu.SLAB_USBtoUART"
# do not override ENVIROMENT
ENVIROMENT ?= "release"

build:
	 . /Users/linusoleander/export-esp.sh && cargo pio build --release
clean:
	cargo clean
	pio run --target clean -e $(ENVIROMENT)
upload:
	. /Users/linusoleander/export-esp.sh && cargo pio exec -- run --target upload -e $(ENVIROMENT) --monitor-port $(PORT)
flash:
	cargo pio exec -- run --target upload -e $(ENVIROMENT) --monitor-port $(PORT)
erase: build
	espflash flash \
		--erase-parts nvs \
		--partition-table partitions.csv \
		--baud 921600 \
		--port $(PORT) \
		.pio/build/${ENVIROMENT}/firmware.elf
monitor:
	espflash monitor -b 115200 -p $(PORT)
menuconfig:
	cargo pio espidf menuconfig -r true -t xtensa-esp32-espidf
release: clean erase flash monitor

default: upload monitor
