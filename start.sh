#!/bin/bash

# Start script for ST7789 Digital Clock (90° rotation)
# Handles environment setup and launches the clock with failsafe wrapper

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_success() { echo -e "${GREEN}✓${NC} $1"; }
print_error() { echo -e "${RED}✗${NC} $1"; }
print_warning() { echo -e "${YELLOW}!${NC} $1"; }
print_info() { echo -e "${BLUE}→${NC} $1"; }

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "======================================"
echo "ST7789 Digital Clock - Startup"
echo "======================================"
echo ""

# Check if binaries exist
if [ ! -f "$SCRIPT_DIR/clock" ]; then
    print_error "Clock binary not found!"
    print_info "Run ./build.sh first to build the project"
    exit 1
fi

if [ ! -f "$SCRIPT_DIR/failsafe" ]; then
    print_warning "Failsafe wrapper not found, running without failsafe"
    USE_FAILSAFE=0
else
    USE_FAILSAFE=1
fi

# Check for SPI device
if [ ! -e /dev/spidev0.0 ]; then
    print_error "SPI device not found!"
    print_info "Enable SPI using: sudo raspi-config"
    print_info "Navigate to: Interface Options -> SPI -> Enable"
    exit 1
fi

# Check SPI permissions
if [ ! -r /dev/spidev0.0 ] || [ ! -w /dev/spidev0.0 ]; then
    print_warning "SPI device permissions issue"
    if [ "$EUID" -ne 0 ]; then
        print_info "Try running with sudo, or add user to spi group:"
        print_info "  sudo usermod -a -G spi $USER"
        exit 1
    fi
fi

# Check GPIO access
if [ ! -w /sys/class/gpio/export ]; then
    print_warning "Cannot write to GPIO export"
    if [ "$EUID" -ne 0 ]; then
        print_info "Try running with sudo, or add user to gpio group:"
        print_info "  sudo usermod -a -G gpio $USER"
        exit 1
    fi
fi

# Load SPI module if not loaded
if ! lsmod | grep -q spi_bcm2835; then
    print_info "Loading SPI kernel module..."
    if [ "$EUID" -ne 0 ]; then
        sudo modprobe spi_bcm2835 || {
            print_error "Failed to load SPI module"
            exit 1
        }
    else
        modprobe spi_bcm2835 || {
            print_error "Failed to load SPI module"
            exit 1
        }
    fi
fi

# Set CPU governor to performance for better responsiveness
if [ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]; then
    print_info "Setting CPU governor to performance..."
    if [ "$EUID" -ne 0 ]; then
        echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null 2>&1 || true
    else
        echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null 2>&1 || true
    fi
fi

# Create log directory
LOG_DIR="$SCRIPT_DIR/logs"
if [ ! -d "$LOG_DIR" ]; then
    mkdir -p "$LOG_DIR"
fi

LOG_FILE="$LOG_DIR/clock_$(date +%Y%m%d_%H%M%S).log"

print_success "Pre-flight checks passed"
echo ""

# Cleanup function
cleanup() {
    echo ""
    print_info "Shutting down..."

    # Kill child processes
    if [ ! -z "$CLOCK_PID" ]; then
        kill $CLOCK_PID 2>/dev/null || true
    fi

    # Wait a moment for clean shutdown
    sleep 1

    # Turn off backlight via GPIO
    if [ -d /sys/class/gpio/gpio18 ]; then
        echo 0 > /sys/class/gpio/gpio18/value 2>/dev/null || true
    fi

    print_success "Shutdown complete"
    exit 0
}

trap cleanup SIGINT SIGTERM

# Start the clock
if [ $USE_FAILSAFE -eq 1 ]; then
    print_info "Starting clock with failsafe wrapper..."
    echo ""
    "$SCRIPT_DIR/failsafe" "$SCRIPT_DIR/clock" 2>&1 | tee "$LOG_FILE" &
    CLOCK_PID=$!
else
    print_info "Starting clock..."
    echo ""
    "$SCRIPT_DIR/clock" 2>&1 | tee "$LOG_FILE" &
    CLOCK_PID=$!
fi

# Wait for the clock process
wait $CLOCK_PID

# If we get here, the clock exited
EXIT_CODE=$?
echo ""
if [ $EXIT_CODE -eq 0 ]; then
    print_success "Clock exited cleanly"
else
    print_error "Clock exited with error code: $EXIT_CODE"
fi

exit $EXIT_CODE
