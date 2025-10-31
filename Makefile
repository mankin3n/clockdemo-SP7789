# Makefile for ST7789 Digital Clock (90° rotation)
# Alternative to build.sh for those who prefer make

CXX = g++
CXXFLAGS = -O3 -Wall -std=c++11
LDFLAGS = -lpthread

# Targets
TARGETS = clock failsafe test_display

# Default target
all: $(TARGETS)
	@echo ""
	@echo "======================================"
	@echo "Build completed successfully!"
	@echo "======================================"
	@echo ""
	@echo "Next steps:"
	@echo "  1. Test display: sudo ./test_display"
	@echo "  2. Run clock:    sudo ./start.sh"
	@echo ""

# Build clock application
clock: clock.cpp
	@echo "Compiling clock..."
	$(CXX) $(CXXFLAGS) -o clock clock.cpp $(LDFLAGS)

# Build failsafe wrapper
failsafe: failsafe.cpp
	@echo "Compiling failsafe..."
	$(CXX) $(CXXFLAGS) -o failsafe failsafe.cpp

# Build test display
test_display: test_display.cpp
	@echo "Compiling test_display..."
	$(CXX) $(CXXFLAGS) -o test_display test_display.cpp

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGETS)
	rm -f *.o
	rm -rf logs/
	@echo "Clean completed"

# Install systemd service (requires sudo)
install-service:
	@echo "Installing systemd service..."
	@if [ ! -f /tmp/ili9340-clock.service ]; then \
		echo "Error: Service file not found. Run ./build.sh first."; \
		exit 1; \
	fi
	sudo cp /tmp/ili9340-clock.service /etc/systemd/system/
	sudo systemctl daemon-reload
	@echo "Service installed. Enable with: sudo systemctl enable ili9340-clock.service"

# Uninstall systemd service
uninstall-service:
	@echo "Uninstalling systemd service..."
	sudo systemctl stop ili9340-clock.service || true
	sudo systemctl disable ili9340-clock.service || true
	sudo rm -f /etc/systemd/system/ili9340-clock.service
	sudo systemctl daemon-reload
	@echo "Service uninstalled"

# Quick test
test: test_display
	@echo "Running display test (requires sudo)..."
	sudo ./test_display

# Run the clock
run: clock failsafe
	@echo "Starting clock (requires sudo)..."
	sudo ./start.sh

# Help target
help:
	@echo "Available targets:"
	@echo "  make              - Build all components (default)"
	@echo "  make clean        - Remove built binaries"
	@echo "  make test         - Build and run display test"
	@echo "  make run          - Build and run the clock"
	@echo "  make install-service - Install systemd service"
	@echo "  make uninstall-service - Uninstall systemd service"
	@echo "  make help         - Show this help message"

# Phony targets
.PHONY: all clean test run install-service uninstall-service help
