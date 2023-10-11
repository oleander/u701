.PHONY: build clean upload monitor default

# PORT := $(shell ls /dev/* | grep "cu.usbserial" | head -n 1 || echo "/dev/cu.usbserial-110")
PORT := "/dev/cu.usbserial-110"

build:
	cargo pio build && ~/Code/git-ai/target/release/git-ai --all
build_tiny:
	cargo pio build -r && ~/Code/git-ai/target/release/git-ai --all
clean:
	cargo clean
	rm -rf target .embuild build .pio
upload:
	cargo pio exec -- run --target upload --monitor-port $(PORT) -e release
upload_ota:
	m wifi connect u701 11111111
	cargo pio exec -- run --target upload -e ota
	m wifi connect boat
monitor:
	espflash monitor -p $(PORT)
default: upload monitor
