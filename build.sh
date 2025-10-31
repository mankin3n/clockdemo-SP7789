#!/bin/bash

# Build script for ST7789 Digital Clock (90° rotation)
# Checks dependencies, installs if needed, and builds all components

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print functions
print_header() {
    echo -e "\n${BLUE}=====================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}=====================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}!${NC} $1"
}

print_info() {
    echo -e "${BLUE}→${NC} $1"
}

# Check if running on Raspberry Pi
check_raspberry_pi() {
    print_info "Checking if running on Raspberry Pi..."
    if [ ! -f /proc/device-tree/model ]; then
        print_warning "Cannot determine device model"
        return 0
    fi

    model=$(cat /proc/device-tree/model)
    if [[ $model == *"Raspberry Pi"* ]]; then
        print_success "Running on: $model"
        return 0
    else
        print_warning "Not running on Raspberry Pi (detected: $model)"
        print_warning "Build will continue but may not work on this platform"
        return 0
    fi
}

# Check Linux version
check_linux_version() {
    print_info "Checking Linux version..."
    kernel_version=$(uname -r)
    os_info=$(cat /etc/os-release | grep PRETTY_NAME | cut -d= -f2 | tr -d '"')
    print_success "Kernel: $kernel_version"
    print_success "OS: $os_info"
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check and install dependencies
check_dependencies() {
    print_header "Checking Dependencies"

    local needs_update=0
    local missing_packages=()

    # Check for essential build tools
    if ! command_exists g++; then
        print_warning "g++ not found"
        missing_packages+=("g++")
        needs_update=1
    else
        gcc_version=$(g++ --version | head -n1)
        print_success "g++ found: $gcc_version"
    fi

    if ! command_exists make; then
        print_warning "make not found"
        missing_packages+=("make")
        needs_update=1
    else
        make_version=$(make --version | head -n1)
        print_success "make found: $make_version"
    fi

    # Check for required libraries
    print_info "Checking for required development libraries..."

    # Check if linux headers are installed
    if [ ! -d "/usr/include/linux" ]; then
        print_warning "Linux headers not found"
        missing_packages+=("linux-libc-dev")
        needs_update=1
    else
        print_success "Linux headers found"
    fi

    # Install missing packages if needed
    if [ $needs_update -eq 1 ]; then
        print_info "Installing missing packages: ${missing_packages[*]}"

        if [ "$EUID" -ne 0 ]; then
            print_warning "Need sudo privileges to install packages"
            sudo apt-get update
            sudo apt-get install -y build-essential "${missing_packages[@]}"
        else
            apt-get update
            apt-get install -y build-essential "${missing_packages[@]}"
        fi

        print_success "Dependencies installed"
    else
        print_success "All dependencies already installed"
    fi
}

# Check SPI configuration
check_spi() {
    print_header "Checking SPI Configuration"

    # Check if SPI kernel module is loaded
    if lsmod | grep -q spi_bcm2835; then
        print_success "SPI kernel module loaded"
    else
        print_warning "SPI kernel module not loaded"
        print_info "Attempting to load module..."
        if [ "$EUID" -ne 0 ]; then
            sudo modprobe spi_bcm2835 || print_error "Failed to load SPI module"
        else
            modprobe spi_bcm2835 || print_error "Failed to load SPI module"
        fi
    fi

    # Check if SPI device exists
    if [ -e /dev/spidev0.0 ]; then
        print_success "SPI device /dev/spidev0.0 exists"

        # Check permissions
        if [ -r /dev/spidev0.0 ] && [ -w /dev/spidev0.0 ]; then
            print_success "SPI device has correct permissions"
        else
            print_warning "SPI device permissions may need adjustment"
            print_info "You may need to add user to 'spi' group: sudo usermod -a -G spi \$USER"
        fi
    else
        print_error "SPI device /dev/spidev0.0 not found"
        print_info "Enable SPI using: sudo raspi-config"
        print_info "Navigate to: Interface Options -> SPI -> Enable"
        return 1
    fi

    # Check boot config
    if [ -f /boot/config.txt ]; then
        if grep -q "^dtparam=spi=on" /boot/config.txt; then
            print_success "SPI enabled in /boot/config.txt"
        else
            print_warning "SPI may not be enabled in /boot/config.txt"
        fi
    elif [ -f /boot/firmware/config.txt ]; then
        if grep -q "^dtparam=spi=on" /boot/firmware/config.txt; then
            print_success "SPI enabled in /boot/firmware/config.txt"
        else
            print_warning "SPI may not be enabled in /boot/firmware/config.txt"
        fi
    fi
}

# Check GPIO access
check_gpio() {
    print_header "Checking GPIO Access"

    if [ -d /sys/class/gpio ]; then
        print_success "GPIO sysfs interface available"

        # Check if we can access GPIO
        if [ -w /sys/class/gpio/export ]; then
            print_success "GPIO export is writable"
        else
            print_warning "GPIO export not writable, may need sudo for GPIO access"
            print_info "You can add user to 'gpio' group: sudo usermod -a -G gpio \$USER"
        fi
    else
        print_error "GPIO sysfs interface not found"
        return 1
    fi
}

# Clean previous builds
clean_build() {
    print_header "Cleaning Previous Builds"

    if [ -f clock ]; then
        rm -f clock
        print_info "Removed old clock binary"
    fi

    if [ -f failsafe ]; then
        rm -f failsafe
        print_info "Removed old failsafe binary"
    fi

    if [ -f test_display ]; then
        rm -f test_display
        print_info "Removed old test_display binary"
    fi

    print_success "Clean completed"
}

# Build the clock application
build_clock() {
    print_header "Building Clock Application"

    print_info "Compiling clock.cpp..."
    g++ -O3 -Wall -o clock clock.cpp -lpthread

    if [ -f clock ]; then
        print_success "Clock application built successfully"
        chmod +x clock
    else
        print_error "Failed to build clock application"
        return 1
    fi
}

# Build the failsafe wrapper
build_failsafe() {
    print_header "Building Failsafe Wrapper"

    print_info "Compiling failsafe.cpp..."
    g++ -O3 -Wall -o failsafe failsafe.cpp

    if [ -f failsafe ]; then
        print_success "Failsafe wrapper built successfully"
        chmod +x failsafe
    else
        print_error "Failed to build failsafe wrapper"
        return 1
    fi
}

# Build the test application
build_test() {
    print_header "Building Test Application"

    print_info "Compiling test_display.cpp..."
    g++ -O3 -Wall -o test_display test_display.cpp

    if [ -f test_display ]; then
        print_success "Test application built successfully"
        chmod +x test_display
    else
        print_error "Failed to build test application"
        return 1
    fi
}

# Create systemd service file
create_service() {
    print_header "Creating Systemd Service"

    local service_file="/tmp/ili9340-clock.service"
    local current_dir=$(pwd)

    cat > "$service_file" << EOF
[Unit]
Description=ST7789 Digital Clock Display (90° rotation)
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=$current_dir
ExecStart=$current_dir/start.sh
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

    print_success "Service file created at $service_file"
    print_info "To install service:"
    print_info "  sudo cp $service_file /etc/systemd/system/"
    print_info "  sudo systemctl daemon-reload"
    print_info "  sudo systemctl enable ili9340-clock.service"
    print_info "  sudo systemctl start ili9340-clock.service"
}

# Main build process
main() {
    print_header "ST7789 Digital Clock - Build Script (90° Rotation)"

    # System checks
    check_raspberry_pi
    check_linux_version

    # Check and install dependencies
    check_dependencies

    # Hardware checks
    check_spi
    check_gpio

    # Build process
    clean_build
    build_clock || exit 1
    build_failsafe || exit 1
    build_test || exit 1

    # Create service file
    create_service

    # Final summary
    print_header "Build Summary"
    print_success "All components built successfully!"
    echo ""
    print_info "Built binaries:"
    ls -lh clock failsafe test_display | awk '{print "  " $9 " (" $5 ")"}'
    echo ""
    print_info "Next steps:"
    echo "  1. Test the display: sudo ./test_display"
    echo "  2. Run the clock:    sudo ./start.sh"
    echo "  3. Or with failsafe: sudo ./failsafe ./clock"
    echo ""
    print_warning "Note: You may need sudo to access GPIO and SPI"
    print_info "To run without sudo, add your user to 'gpio' and 'spi' groups:"
    echo "  sudo usermod -a -G gpio,spi \$USER"
    echo "  Then log out and log back in"
    echo ""
}

# Run main function
main
