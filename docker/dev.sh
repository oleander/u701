#!/bin/bash

# u701 Docker Development Helper Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "u701 Docker Development Helper"
    echo ""
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  build         Build the Docker images"
    echo "  dev           Start development environment"
    echo "  shell         Open shell in development container"
    echo "  build-project Build the ESP32 project in container"
    echo "  test          Run Rust tests in container"
    echo "  clean         Clean Docker images and containers"
    echo "  logs          Show container logs"
    echo "  stop          Stop all containers"
    echo "  ota           Start OTA server"
    echo "  help          Show this help message"
    echo ""
    echo "ESP32 Commands (require hardware connection):"
    echo "  upload        Flash ESP32 via USB"
    echo "  monitor       Monitor ESP32 serial output"
    echo "  install       Flash and monitor ESP32"
    echo ""
}

build_images() {
    print_info "Building Docker images..."
    cd "$PROJECT_ROOT"
    docker-compose build u701-dev
    print_success "Docker images built successfully"
}

start_dev() {
    print_info "Starting development environment..."
    cd "$PROJECT_ROOT"
    docker-compose up -d u701-dev
    print_success "Development environment started"
    print_info "Use 'docker/dev.sh shell' to access the container"
}

open_shell() {
    print_info "Opening shell in development container..."
    cd "$PROJECT_ROOT"
    
    # Check if container is running
    if ! docker-compose ps u701-dev | grep -q "Up"; then
        print_warning "Development container not running. Starting it..."
        docker-compose up -d u701-dev
        sleep 2
    fi
    
    docker-compose exec u701-dev /bin/bash
}

build_project() {
    print_info "Building ESP32 project in container..."
    cd "$PROJECT_ROOT"
    docker-compose exec u701-dev bash -c "source \$ESPUP_PATH && just build"
}

run_tests() {
    print_info "Running Rust tests in container..."
    cd "$PROJECT_ROOT"
    docker-compose exec u701-dev bash -c "source \$ESPUP_PATH && just test"
}

upload_firmware() {
    print_info "Uploading firmware to ESP32..."
    cd "$PROJECT_ROOT"
    
    # Check for USB devices
    if ! ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null; then
        print_error "No ESP32 USB devices found. Please connect your ESP32."
        exit 1
    fi
    
    docker-compose exec u701-dev bash -c "source \$ESPUP_PATH && just upload"
}

monitor_serial() {
    print_info "Monitoring ESP32 serial output..."
    cd "$PROJECT_ROOT"
    docker-compose exec u701-dev bash -c "source \$ESPUP_PATH && just monitor"
}

install_firmware() {
    print_info "Installing firmware (upload + monitor)..."
    cd "$PROJECT_ROOT"
    docker-compose exec u701-dev bash -c "source \$ESPUP_PATH && just install"
}

start_ota() {
    print_info "Starting OTA server..."
    cd "$PROJECT_ROOT"
    docker-compose up -d u701-ota
    print_success "OTA server started on port 3232"
}

clean_docker() {
    print_info "Cleaning Docker images and containers..."
    cd "$PROJECT_ROOT"
    docker-compose down -v
    docker system prune -f
    print_success "Docker cleanup completed"
}

show_logs() {
    print_info "Showing container logs..."
    cd "$PROJECT_ROOT"
    docker-compose logs -f u701-dev
}

stop_containers() {
    print_info "Stopping all containers..."
    cd "$PROJECT_ROOT"
    docker-compose down
    print_success "All containers stopped"
}

# Main command dispatcher
case "${1:-help}" in
    build)
        build_images
        ;;
    dev)
        start_dev
        ;;
    shell)
        open_shell
        ;;
    build-project)
        build_project
        ;;
    test)
        run_tests
        ;;
    upload)
        upload_firmware
        ;;
    monitor)
        monitor_serial
        ;;
    install)
        install_firmware
        ;;
    ota)
        start_ota
        ;;
    clean)
        clean_docker
        ;;
    logs)
        show_logs
        ;;
    stop)
        stop_containers
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        echo ""
        show_help
        exit 1
        ;;
esac