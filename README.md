# u701 [![Rust Build Status](https://github.com/oleander/u701/actions/workflows/rust.yml/badge.svg)](https://github.com/oleander/u701/actions/workflows/rust.yml)

**u701** is a BLE proxy developed in Rust and C++ for the [Terrain Command](https://carpe-iter.com/support/rally-command-getting-started/) controller by [Carpe Iter](https://carpe-iter.com). This software enables BLE event remapping for iOS device compatibility, expanding upon the original Android-only support. It operates on an ESP32C3 microcontroller but is designed to be portable to other platforms.

## Setup

### Native Setup

* `cargo install cargo-pio espup just`
* `just setup build`

### Docker Setup (Recommended)

For a consistent development environment across all platforms:

* Install [Docker](https://docs.docker.com/get-docker/) and [Docker Compose](https://docs.docker.com/compose/install/)
* `./docker/dev.sh build` - Build the development environment
* `./docker/dev.sh dev` - Start the development container
* `./docker/dev.sh shell` - Open a shell in the container

See [DOCKER.md](DOCKER.md) for complete Docker documentation.

## Hardware Support

- Terrain Command v2
- ESP32C3

> For Terrain Command V3, update event IDs in `machine/src/constants.rs`.

## Flashing the ESP32

### Native Flashing

1. Connect the ESP32 to your computer.
2. Run `just upload` to flash the ESP32.
3. Run `just monitor` to view the serial output.

### Docker Flashing

1. Connect the ESP32 to your computer.
2. Run `./docker/dev.sh upload` or `just docker-upload` to flash the ESP32.
3. Run `./docker/dev.sh monitor` or `just docker-monitor` to view the serial output.
4. Or combine both with `./docker/dev.sh install` or `just docker-install`.

## Features

- **Direct Button Mapping**: Facilitates direct mapping of buttons to HID events, such as Play/Pause.
- **Meta Key Combinations**: Utilizes red buttons on the Terrain Command as meta keys for triggering complex events.
- **iOS Shortcuts Integration**: Enables triggering of iOS shortcuts through BLE events for advanced functionality.

## Event Mapping Flow

Event processing in the Terrain Command follows this sequence:

1. A button press occurs on the Terrain Command.
2. The Terrain Command emits a BLE event.
3. The ESP32, equipped with this software, receives and remaps the event.
4. The iPhone receives the remapped BLE event.

The ESP32 functions both as a BLE host and server.

## Terrain Command Button Layout

The controller features eight buttons, outlined as follows:

| Row 1                  | Row 2                  |
| ---------------------- | ---------------------- |
| M1 :red_circle:        | M2 :red_circle:        |
| A2 :black_circle:      | B2 :black_circle:      |
| A3 :large_blue_circle: | B3 :large_blue_circle: |
| A4 :black_circle:      | B4 :black_circle:      |

## Meta Keys

Meta keys (`M1` and `M2`) can be used in combination with other buttons to initiate unique BLE events:

1. Press and release a Meta key.
2. Press another button to trigger the corresponding event.

### M1 and M2 Combinations

| Key | M1 Combination | M2 Combination |
| --- | -------------- | -------------- |
| A2  | Shortcut A     | Shortcut G     |
| A3  | Shortcut B     | Shortcut H     |
| A4  | Shortcut C     | Shortcut I     |
| B2  | Shortcut D     | Shortcut J     |
| B3  | Shortcut E     | Shortcut K     |
| B4  | Shortcut F     | Shortcut L     |

## Shortcuts

Steps to map iOS shortcuts:

1. Create a shortcut in the iOS Shortcuts app.
2. Navigate to `Settings -> Accessibility -> Keyboards -> Commands` on your iPhone.
3. Scroll to the bottom and select your shortcut.
4. Press a key combination on the Terrain Command (e.g., `M1` + `A2`).
5. The shortcut is now mapped and will be triggered with the key combination.

## HID Events

HID events are pre-mapped to the following keys:

- `A2`: Volume Down
- `A3`: Previous Song
- `A4`: Play/Pause
- `B2`: Volume Up
- `B3`: Next Song
- `B4`: Eject

> `Eject` in iOS is used to toggle the keyboard visibility. When iOS connects to an external keyboard, the on-screen keyboard is hidden. Pressing `Eject` on the Terrain Command will toggle the on-screen keyboard.

## Custom Events

For custom events, modify mappings in `machine/src/constants.rs`.
