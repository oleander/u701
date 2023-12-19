# u701 Project Documentation

[![Rust Build Status](https://github.com/oleander/u701/actions/workflows/rust.yml/badge.svg)](https://github.com/oleander/u701/actions/workflows/rust.yml)

## Introduction

**u701** is a BLE proxy developed in Rust and C++ for the [Terrain Command](https://carpe-iter.com/support/rally-command-getting-started/) controller by [Carpe Iter](https://carpe-iter.com). This software enables BLE event remapping for iOS device compatibility, expanding upon the original Android-only support. It operates on an ESP32C3 microcontroller but is designed to be portable to other platforms.

## Event Mapping Flow

Event processing in the Terrain Command follows this sequence:

1. A button press occurs on the Terrain Command.
2. The Terrain Command emits a BLE event.
3. The ESP32, equipped with this software, receives and remaps the event.
4. The iPhone receives the remapped BLE event.

The ESP32 functions both as a BLE host and server.

## Features

- **Direct Button Mapping**: Facilitates direct mapping of buttons to HID events, such as Play/Pause.
- **Meta Key Combinations**: Utilizes red buttons on the Terrain Command as meta keys for triggering complex events.
- **iOS Shortcuts Integration**: Enables triggering of iOS shortcuts through BLE events for advanced functionality.

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

## Hardware Support

- Terrain Command v2
- ESP32C3

> For Terrain Command V3, update event IDs in `machine/src/constants.rs`.

## Flashing the ESP32

Execute `just upload monitor` to flash the ESP32.

## About the Project

This project leverages the ESP32C3, Platformio, Rust, and various Arduino libraries to implement a BLE proxy. (Further details on libraries are to be provided.)

## Setup

Configure the build target for the ESP32 with `rustup target add riscv32imc-esp-espidf`.

## Development Tools

The project utilizes `just` as the build system. The `Justfile` includes various commands for managing the ESP32, with detailed explanations.

## Project Goal

- Transition fully to Rust, eliminating the need for Platformio and C++.
- Resolve issues related to reconnecting when using Rust exclusively.
