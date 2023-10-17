.PHONY: build clean upload monitor default erase release

# PORT := $(shell ls /dev/* | grep "cu.usbserial" | head -n 1 || echo "/dev/cu.usbserial-110")
PORT := "/dev/cu.usbserial-10"

build:
	cargo pio build
build_release:
	cargo pio build -r
clean:
	cargo clean
	pio run --target clean
flash_release:
	cargo pio exec -- run --target upload -e release
upload_ota:
	cargo pio exec -- run --target upload -e ota
erase: build_release
	espflash flash --erase-parts nvs,phy_init .pio/build/release/firmware.elf --partition-table partitions.csv -b 921600 -p $(PORT)
monitor:
	espflash monitor -b 115200 -p $(PORT)
release: clean erase flash_release
default: upload monitor
