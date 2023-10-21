.PHONY: build clean upload monitor erase

# PORT := $(shell ls /dev/* | grep "tty.usbserial" | head -n 1)
PORT := "/dev/cu.SLAB_USBtoUART"
# do not override ENVIROMENT
ENVIROMENT ?= "release"

clean:
	cargo clean
	cargo clean -r
	pio run --target clean -e $(ENVIROMENT)
	pio run --target clean
build:
	. ~/export-esp.sh && cargo pio build -r
upload: erase
	. ~/export-esp.sh && cargo pio exec -- run -t upload -e $(ENVIROMENT) --monitor-port $(PORT)
ota:
	. ~/export-esp.sh && cargo pio exec -- run -t upload -e ota
erase:
	esptool.py erase_region 0x9000 0x5000
monitor:
	./tools/monitor.sh -b 115200 -p $(PORT)
menuconfig:
	. ~/export-esp.sh && cargo pio espidf menuconfig -r true