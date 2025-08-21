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

# Clean the build artifacts
just clean
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

3. **Data Flow**:
   - ESP32 scans for the Terrain Command controller using whitelist filtering
   - Upon connection, it subscribes to specific BLE characteristics
   - When events are received, they're processed through the Rust FFI
   - Remapped events are sent to the connected iOS device

4. **Event Mapping Logic**:
   - Direct mapping: Buttons trigger HID events (Play/Pause, Volume, etc.)
   - Meta key combinations: Red buttons (M1, M2) work as modifiers for other buttons
   - iOS shortcuts integration: Allows complex actions through iOS shortcut system

## Hardware Support

- Terrain Command v2 controller
- ESP32C3 microcontroller

For Terrain Command V3 support, update event IDs in `machine/src/constants.rs`.