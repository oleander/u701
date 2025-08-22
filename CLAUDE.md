# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

u701 is a BLE proxy developed in Rust and C++ for the Terrain Command controller by Carpe Iter. It runs on an ESP32C3 microcontroller and enables BLE event remapping for iOS device compatibility. The ESP32 functions as both a BLE host and server, receiving events from the Terrain Command controller and remapping them for iOS devices.

## Development Environment Setup

The project uses:
- PlatformIO for C++ and ESP-IDF framework
- Cargo for Rust components
- Just command runner for build automation

Required tools:
```
cargo install cargo-pio espup just
```

## Common Commands

### Setup and Build

```bash
# Initial setup and build
just setup build

# Build specific mode (release is default, debug is alternative)
just build release
just build debug

# CI mode - optimized for build speed, not for performance or size
just build ci

# Clean the build artifacts
just clean
```

### Testing

```bash
# Run Rust tests
just test
```

### Flashing and Monitoring

```bash
# Flash the ESP32
just upload

# Monitor serial output
just monitor

# Flash and start monitoring (combined)
just install
```

### Advanced Commands

```bash
# OTA update
just ota

# Enter ESP-IDF configuration menu
just menuconfig

# Erase the NVS region
just erase

# Update PlatformIO packages
just update
```

## Project Architecture

The project is a mixed C++ and Rust codebase:

1. **Main Components**:
   - C++ code handles the BLE connectivity and Arduino framework integration
   - Rust code handles the event mapping logic
   - FFI interface connects C++ and Rust components

2. **Key Files**:
   - `src/main.cpp`: Core ESP32 setup and BLE connection logic
   - `machine/src/constants.rs`: Button mapping configuration
   - `include/ClientCallbacks.hh`: BLE client callback handlers
   - `include/AdvertisedDeviceCallbacks.hh`: BLE advertisement handlers
   - `machine/src/lib.rs`: State machine for button event handling
   - `src/lib.rs`: Rust core functionality that processes button events
   - `src/ffi.rs`: FFI interface between C++ and Rust

3. **Data Flow**:
   - ESP32 scans for the Terrain Command controller using whitelist filtering
   - Upon connection, it subscribes to specific BLE characteristics
   - When events are received, they're processed through the Rust FFI
   - Remapped events are sent to the connected iOS device

4. **Event Mapping Logic**:
   - Direct mapping: Buttons trigger HID events (Play/Pause, Volume, etc.)
   - Meta key combinations: Red buttons (M1, M2) work as modifiers for other buttons
   - iOS shortcuts integration: Allows complex actions through iOS shortcut system

5. **State Machine**:
   - The project uses a simple state machine to track button press combinations
   - Meta keys (M1, M2) change the state to allow for different actions when other buttons are pressed
   - Actions can result in either direct media control events or shortcut activations

## Hardware Support

- Terrain Command v2 controller
- ESP32C3 microcontroller

For Terrain Command V3 support, update event IDs in `machine/src/constants.rs`.

## Modifying Button Mappings

To customize button mappings:
1. Edit `machine/src/constants.rs` to update button codes and mappings
2. Modify the `EVENT` HashMap for direct button mappings
3. Modify the `META` HashMap for meta key combinations
4. Recompile and flash the ESP32