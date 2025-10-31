# ST7789 Digital Clock for Raspberry Pi

A simple, reliable digital clock application for 2.8" 320x240 ST7789 displays on Raspberry Pi 4B with 90° rotation (landscape mode), using the same driver approach as [fbcp-ili9341](https://github.com/juj/fbcp-ili9341).

![Display Demo](https://via.placeholder.com/320x240/000000/00FFFF?text=12:34:56)

## Features

- **Simple Digital Clock** with large, readable time display
- **Date Display** with color-coded elements
- **Failsafe Mode** with automatic crash recovery
- **Hardware Acceleration** using Raspberry Pi's SPI interface
- **Low CPU Usage** with efficient update mechanism
- **Auto-build Script** that handles all dependencies
- **Comprehensive Testing** with display test suite
- **Production Ready** with systemd service support

## Quick Start

```bash
# 1. Make scripts executable
chmod +x build.sh start.sh

# 2. Build everything (handles dependencies automatically)
./build.sh

# 3. Test the display
sudo ./test_display

# 4. Run the clock
sudo ./start.sh
```

## What's Included

| File | Description |
|------|-------------|
| `clock.cpp` | Main digital clock application |
| `failsafe.cpp` | Failsafe wrapper with auto-restart and error recovery |
| `test_display.cpp` | Comprehensive display test utility (10 test phases) |
| `build.sh` | **Single-script build system** - checks dependencies, versions, compatibility, and builds everything |
| `start.sh` | **Single-script launcher** - sets up environment and starts the clock with failsafe |
| `SETUP.md` | **Complete setup guide** with GPIO pinout, wiring diagrams, and troubleshooting |

## Hardware Requirements

- **Raspberry Pi 4B** (running Linux/Raspberry Pi OS)
- **2.8" ST7789 Display** (320x240, SPI interface)
- Jumper wires for connections

## GPIO Pin Connections

**Your Display Has 8 Pins in This Order:**

| Display Pin | Pin # | RPi GPIO | RPi Pin # | Function |
|-------------|-------|----------|-----------|----------|
| GND | 1 | GND | Pin 6 | Ground |
| VCC | 2 | 3.3V | Pin 1 | Power |
| SCL | 3 | GPIO 11 | Pin 23 | SPI Clock |
| SDA | 4 | GPIO 10 | Pin 19 | SPI Data (MOSI) |
| RES | 5 | GPIO 24 | Pin 18 | Reset |
| DC/RS | 6 | GPIO 25 | Pin 22 | Data/Command |
| CS | 7 | GPIO 8 | Pin 24 | Chip Select |
| BL | 8 | GPIO 18 | Pin 12 | Backlight |

**⚠️ Important**: Use 3.3V, NOT 5V! The ST7789 operates at 3.3V only.

**📐 Display Orientation**: This code is configured for **90° rotation (landscape mode)**, meaning the display is rotated to show content horizontally. This is ideal for clock displays.

See [SETUP.md](SETUP.md) for complete wiring diagrams and detailed instructions.

## Build Script Features

The `build.sh` script is a **comprehensive build system** that:

- ✅ Detects Raspberry Pi model and Linux version
- ✅ Checks for required dependencies (g++, make, headers)
- ✅ **Automatically installs missing packages** with version compatibility checks
- ✅ **Checks if packages are already installed** (doesn't reinstall unnecessarily)
- ✅ Verifies SPI is enabled and accessible
- ✅ Verifies GPIO access and permissions
- ✅ Compiles all three programs with optimization
- ✅ Creates systemd service file for autostart
- ✅ Provides clear next steps and troubleshooting info

## Start Script Features

The `start.sh` script is a **complete launcher** that:

- ✅ Performs pre-flight checks (SPI, GPIO, permissions)
- ✅ Loads SPI kernel module if needed
- ✅ Sets CPU governor for optimal performance
- ✅ Starts clock with failsafe wrapper
- ✅ Creates timestamped log files
- ✅ Handles clean shutdown on Ctrl+C
- ✅ Turns off display backlight on exit

## Installation Steps

### 1. Enable SPI

```bash
sudo raspi-config
```
Navigate to: `Interface Options` → `SPI` → `Yes` → Reboot

### 2. Wire the Display

Follow the GPIO pin table above. See [SETUP.md](SETUP.md) for visual diagrams.

### 3. Build the Project

```bash
./build.sh
```

The build script handles everything automatically, including:
- Installing build tools (g++, make)
- Installing required libraries
- Checking system compatibility
- Compiling all components

### 4. Test the Display

```bash
sudo ./test_display
```

Runs 10 comprehensive tests:
1. SPI device check
2. GPIO configuration
3. Display initialization
4. SPI communication
5. Backlight control
6. Color fills (red, green, blue, white, black)
7. Color bars
8. Gradient rendering
9. Checkerboard patterns
10. Stress test (performance measurement)

### 5. Run the Clock

```bash
sudo ./start.sh
```

Or run without failsafe:
```bash
sudo ./clock
```

Or run with failsafe manually:
```bash
sudo ./failsafe ./clock
```

## Running Without Sudo

Add your user to the required groups:

```bash
sudo usermod -a -G gpio,spi $USER
```

Log out and log back in, then:
```bash
./start.sh
```

## Autostart on Boot

Install as a systemd service:

```bash
# Install service
sudo cp /tmp/ili9340-clock.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable ili9340-clock.service
sudo systemctl start ili9340-clock.service

# Check status
sudo systemctl status ili9340-clock.service

# View logs
sudo journalctl -u ili9340-clock.service -f
```

## Failsafe Features

The failsafe wrapper provides:

- **Automatic Restart** - Restarts clock if it crashes
- **Rate Limiting** - Prevents restart loops (max 10 restarts per minute)
- **Error Screen** - Displays red screen if too many crashes occur
- **Hardware Reset** - Performs display reset between restarts
- **Logging** - Logs all events to `/tmp/clock_failsafe.log`

## Display Specifications

- Resolution: 320×240 pixels
- Colors: 65,536 (RGB565)
- Interface: SPI
- Controller: ST7789
- Size: 2.8 inches diagonal
- Viewing angle: 160°
- Orientation: 90° rotation (landscape mode)

## Driver Architecture

Based on [fbcp-ili9341](https://github.com/juj/fbcp-ili9341) approach:

- **Direct SPI Access** via `/dev/spidev0.0`
- **Hardware SPI** using Raspberry Pi's SPI0
- **GPIO Control** via sysfs for DC, RESET, backlight
- **RGB565 Color** for efficient rendering
- **Optimized Updates** only when time changes
- **No Dependencies** on heavy graphics libraries

## Troubleshooting

### Display doesn't turn on
- Check wiring matches GPIO table
- Verify 3.3V power connection
- Run `sudo ./test_display` for diagnostics

### "Cannot open SPI device"
- Enable SPI: `sudo raspi-config` → Interface Options → SPI
- Reboot after enabling

### Permission errors
- Run with `sudo` or add user to groups:
  ```bash
  sudo usermod -a -G gpio,spi $USER
  ```

### Display shows red screen
- Failsafe detected errors
- Check logs: `cat /tmp/clock_failsafe.log`
- Verify wiring and power supply

### Garbled display
- Check MOSI and SCLK connections
- Verify power supply is adequate (3A recommended)
- Test with slower SPI speed

See [SETUP.md](SETUP.md) for complete troubleshooting guide.

## File Structure

```
clockdemo/
├── README.md           # This file
├── SETUP.md           # Detailed setup guide with pinouts
├── clock.cpp          # Main clock application
├── failsafe.cpp       # Failsafe wrapper
├── test_display.cpp   # Display test utility
├── build.sh          # Build script (handles everything)
├── start.sh          # Start script (production launcher)
├── clock             # Compiled binary (after build)
├── failsafe          # Compiled binary (after build)
├── test_display      # Compiled binary (after build)
└── logs/             # Runtime logs (created automatically)
```

## Technical Details

### Clock Application
- Renders time in large cyan digits (scale 8x)
- Renders date in smaller yellow digits (scale 3x)
- Updates every second
- Low CPU usage (~1-2%)
- Clean shutdown handling

### Failsafe Wrapper
- Monitors child process
- Hardware reset on crash
- Rate-limited restarts
- Error logging
- Red screen on critical failure

### Test Application
- 10 comprehensive test phases
- Color accuracy validation
- Performance measurement
- Backlight verification
- Pattern rendering tests

## Performance

- **Update Rate**: 1 Hz (once per second)
- **CPU Usage**: 1-2% average
- **Memory**: ~2 MB
- **Startup Time**: ~2 seconds
- **Display Refresh**: ~20ms per full frame

## Dependencies

Auto-installed by `build.sh`:
- g++ (GCC C++ compiler)
- make
- linux-libc-dev (kernel headers)

Runtime requirements:
- Linux kernel with SPI support
- GPIO sysfs interface
- /dev/spidev0.0 device

## References

- [fbcp-ili9341](https://github.com/juj/fbcp-ili9341) - Original driver inspiration
- [ST7789_TFT_RPI](https://github.com/gavinlyonsrepo/ST7789_TFT_RPI) - ST7789 reference implementation
- [ST7789 Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7789.pdf)
- [Raspberry Pi SPI Documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#spi-overview)

## License

Provided as-is for educational and personal use.

## Contributing

This is a demonstration project. Feel free to adapt for your needs.

## Support

For detailed setup instructions, see [SETUP.md](SETUP.md).

For issues:
1. Run `sudo ./test_display` for diagnostics
2. Check logs in `logs/` directory
3. Verify wiring matches GPIO table
4. Ensure SPI is enabled
5. Check power supply (3A recommended)

---

**Made with ❤️ for Raspberry Pi**
