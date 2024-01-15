#!/bin/bash

set -e
source .env

cargo +nightly install cargo-binstall
cargo +nightly binstall cargo-pio espup just -y
cargo +nightly pio installpio

# just clean build
espup install -t esp32c3 -s
source  ~/.bashrc
cargo +esp build
