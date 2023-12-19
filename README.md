# u701 [![Rust](https://github.com/oleander/u701/actions/workflows/rust.yml/badge.svg)](https://github.com/oleander/u701/actions/workflows/rust.yml)

## Introduction

**u701** is a Rust and C++ based BLE proxy for the [Terrain Command](https://carpe-iter.com/support/rally-command-getting-started/) controller from Carpe Iter, designed to remap BLE events to be compatible with iOS devices. Originally supporting only Android, this implementation extends functionality to iOS, running on an ESP32C3 microcontroller.

## Event Mapping Flow

Events from the Terrain Command are remapped as follows:

1. Button is pressed on the Terrain Command.
2. Terrain Command sends a BLE event.
3. ESP32 (running this software) receives and remaps the event.
4. Remapped BLE event is transmitted to the connected iPhone.

The ESP32 acts simultaneously as a BLE host and server.

## Features

- **Direct Button Mapping**: Buttons can be mapped directly to HID events like Play/Pause.
- **Meta Key Combinations**: Uses the red buttons on the Terrain Command as meta keys to trigger additional events.
- **iOS Shortcuts Integration**: Events can trigger iOS shortcuts for extended functionality.

## Terrain Command Button Layout

For reference, the Terrain Command controller features 8 buttons:

M1 | M2 | :red_circle:
A2 | B2 | :black_circle:
A3 | B3 | :large_blue_circle:
A4 | B4 | :black_circle:

## Meta Keys

The Meta keys (`M1` and `M2`) can be combined with other buttons to trigger unique BLE events:

1. Press and release a Meta key.
2. Press another key to trigger the event.

### Current Mappings

#### M1 Combinations

- `A2`: Shortcut A
- `A3`: Shortcut B
- `A4`: Shortcut C
- `B2`: Shortcut D
- `B3`: Shortcut E
- `B4`: Shortcut F

#### M2 Combinations

- `A2`: Shortcut G
- `A3`: Shortcut H
- `A4`: Shortcut I
- `B2`: Shortcut J
- `B3`: Shortcut K
- `B4`: Shortcut L

## Shortcuts

To map a key combination to an iOS shortcut:

1. Create a shortcut in the iOS Shortcuts app.
2. On your iPhone go to `Settings -> Accessibility -> Keyboards -> Commands`
3. Scroll to the bottom
4. Select the created shortcut
5. Press a key combination on the Terrain Command (e.g. `M1` + `A2`)
6. Done! The shortcut should now be triggered when the key combination is pressed.

## HID Events

The other keys on the Terrain Command are mapped to HID events:

- `A2`: Volume Down
- `A3`: Previous Song
- `A4`: Play/Pause
- `B2`: Volume Up
- `B3`: Next Song
- `B4`: Eject (toggles keyboard visibility in iOS)

## Custom Events

Modify mappings in `machine/src/constants.rs` to add custom events.

## Hardware Support

- Terrain Command v2
- ESP32C3

For Terrain Command V3, update event IDs in `machine/src/constants.rs`.

## Flashing the ESP32

Use the command `just upload monitor`.

## About

This project implements a BLE proxy using ESP32C3, Platformio, Rust, and several Arduino libraries. (Libraries to be detailed further)

## Setup

Set the correct build target for the ESP32: `rustup target add riscv32imc-esp-espidf`

## Development Tools

The project uses `just` as a build system. The `Justfile` contains commands for interacting with the ESP32. (Detailed explanations of these commands are pending.)

## Goal

- Transition entirely to Rust, eliminating Platformio and C++.
- Address reconnecting issues when using plain Rust.
