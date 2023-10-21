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
upload: erase
	. /Users/linusoleander/export-esp.sh && cargo pio exec -- run --target upload -e $(ENVIROMENT) --monitor-port $(PORT)
erase:
	esptool.py erase_region 0x9000 0x5000
monitor:
	./tools/monitor.sh -b 115200 -p $(PORT)
menuconfig:
	cargo pio espidf menuconfig -r true
release: clean erase flash monitor

default: upload monitor
