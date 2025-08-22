# Docker Setup for u701

This document describes how to use Docker for u701 development, providing a consistent development environment across different platforms.

## Prerequisites

- Docker (version 20.10 or later)
- Docker Compose (version 2.0 or later)
- USB access to ESP32 devices (for hardware flashing)

## Quick Start

1. **Build the Docker images:**
   ```bash
   ./docker/dev.sh build
   # or
   just docker-build
   ```

2. **Start the development environment:**
   ```bash
   ./docker/dev.sh dev
   # or
   just docker-dev
   ```

3. **Open a shell in the development container:**
   ```bash
   ./docker/dev.sh shell
   # or
   just docker-shell
   ```

4. **Build the project inside the container:**
   ```bash
   ./docker/dev.sh build-project
   # or
   just docker-build-project
   ```

## Docker Services

### u701-dev (Development)
The main development container with all tools installed:
- Complete ESP32 Rust toolchain
- PlatformIO with ESP32 platform
- cargo-pio integration
- just command runner
- Development tools (vim, nano, tmux, etc.)

### u701-builder (CI/CD)
Optimized for building the project in CI/CD environments:
- Minimal dependencies
- Faster build times
- No interactive tools

### u701-ota (OTA Server)
Dedicated container for Over-The-Air updates:
- Exposes port 3232 for OTA updates
- Automatically starts OTA server

## Available Commands

### Development Commands
```bash
# Build Docker images
./docker/dev.sh build

# Start development environment
./docker/dev.sh dev

# Open shell in container
./docker/dev.sh shell

# Build the ESP32 project
./docker/dev.sh build-project

# Run Rust tests
./docker/dev.sh test
```

### Hardware Commands (require ESP32 connected via USB)
```bash
# Flash firmware to ESP32
./docker/dev.sh upload

# Monitor serial output
./docker/dev.sh monitor

# Flash and monitor (combined)
./docker/dev.sh install
```

### OTA Commands
```bash
# Start OTA server
./docker/dev.sh ota
```

### Utility Commands
```bash
# Show container logs
./docker/dev.sh logs

# Stop all containers
./docker/dev.sh stop

# Clean up Docker resources
./docker/dev.sh clean
```

## Just Integration

All Docker commands are also available via just:

```bash
just docker-build      # Build images
just docker-dev        # Start development
just docker-shell      # Open shell
just docker-test       # Run tests
just docker-upload     # Flash ESP32
just docker-monitor    # Monitor serial
just docker-install    # Flash and monitor
just docker-ota        # Start OTA server
just docker-clean      # Clean up
```

## USB Device Access

For hardware flashing, the Docker container needs access to USB devices. The setup automatically includes common ESP32 USB device mappings:

- `/dev/ttyUSB0` - USB-to-serial adapters
- `/dev/ttyACM0` - CDC ACM devices

### Linux USB Permissions

On Linux, you may need to add your user to the `dialout` group:
```bash
sudo usermod -a -G dialout $USER
```

Then log out and back in for the changes to take effect.

### macOS USB Access

On macOS, USB devices typically appear as `/dev/cu.usbserial-*` or `/dev/cu.usbmodem-*`. You may need to adjust the device mappings in `docker-compose.yml`.

### Windows USB Access

On Windows with WSL2, USB device access requires additional setup. Consider using USB/IP or a Windows-based development environment.

## Volume Mounts

The Docker setup uses several volume mounts for optimal development experience:

- **Source code**: `.` → `/home/esp32/workspace` (live reload)
- **Cargo registry**: Persistent cache for Rust dependencies
- **Cargo git**: Persistent cache for Git dependencies
- **Target cache**: Persistent build artifacts
- **PlatformIO cache**: Persistent PlatformIO packages

## Environment Variables

The development container sets up the following environment variables:

- `CARGO_HOME=/home/esp32/.cargo`
- `PLATFORMIO_CORE_DIR=/home/esp32/.platformio`
- `ESPUP_PATH=/home/esp32/tmp/espup.sh`

## Troubleshooting

### Container won't start
```bash
# Check Docker daemon status
docker info

# Rebuild images
docker-compose build --no-cache
```

### USB device not found
```bash
# List available USB devices
ls /dev/tty*

# Check udev rules (Linux)
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Build failures
```bash
# Clean all caches
./docker/dev.sh clean

# Rebuild images from scratch
docker-compose build --no-cache u701-dev
```

### Permission issues
```bash
# Fix file ownership (run from host)
sudo chown -R $USER:$USER .

# Check container user
docker-compose exec u701-dev whoami
```

## Advanced Usage

### Custom Build Arguments

You can override build arguments in `docker-compose.yml`:

```yaml
services:
  u701-dev:
    build:
      args:
        RUST_VERSION: 1.76.0
        UBUNTU_VERSION: 24.04
```

### Production Builds

For CI/CD, use the builder service:

```bash
docker-compose run --rm u701-builder bash -c "source \$ESPUP_PATH && just build release"
```

### Multiple ESP32 Devices

Add additional device mappings to `docker-compose.yml`:

```yaml
devices:
  - /dev/ttyUSB0:/dev/ttyUSB0
  - /dev/ttyUSB1:/dev/ttyUSB1
  - /dev/ttyACM0:/dev/ttyACM0
```

## Performance Tips

1. **Use build cache**: Keep Docker images updated but don't rebuild unnecessarily
2. **Persistent volumes**: The setup uses volumes to cache dependencies
3. **Parallel builds**: Use `docker-compose build --parallel` for faster builds
4. **Resource limits**: Adjust Docker resource limits if needed

## Security Considerations

- The container runs with `privileged: true` for USB access
- USB devices are mapped directly into the container
- Consider using Docker secrets for sensitive configuration
- Regularly update base images for security patches