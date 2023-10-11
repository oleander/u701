.PHONY: build clean upload monitor default

PORT := $(shell ls /dev/cu.usbserial-* | head -n 1 || echo "/dev/cu.usbserial-110")

build:
	cargo pio build && ~/Code/git-ai/target/release/git-ai --all
clean:
	cargo clean
	rm -rf target .embuild build .pio
upload:
	cargo pio exec -- run --target upload --monitor-port $(PORT) -e release
monitor:
	espflash monitor -p $(PORT)
default: upload monitor
