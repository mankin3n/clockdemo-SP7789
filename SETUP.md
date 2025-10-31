# ST7789 Digital Clock - Setup Guide

Complete setup guide for running a digital clock on a 2.8" 320x240 ST7789 display with Raspberry Pi 4B (90° rotation - landscape mode).

## Table of Contents
1. [Hardware Requirements](#hardware-requirements)
2. [GPIO Pin Connections](#gpio-pin-connections)
3. [Hardware Setup](#hardware-setup)
4. [Software Installation](#software-installation)
5. [Building the Project](#building-the-project)
6. [Running the Clock](#running-the-clock)
7. [Troubleshooting](#troubleshooting)
8. [Autostart Configuration](#autostart-configuration)

---

## Hardware Requirements

- **Raspberry Pi 4B** running Raspberry Pi OS (Linux)
- **2.8" ST7789 Display** (320x240 resolution, SPI interface)
- **Jumper wires** (Female-to-Female recommended)
- **Power supply** for Raspberry Pi (5V, 3A recommended)

**Display Configuration**: This setup uses 90° rotation (landscape mode) for optimal clock display.

---

## GPIO Pin Connections

### Wiring Diagram

Connect the ILI9340 display to your Raspberry Pi 4B using the following GPIO pins:

**Your Display Pin Order (8 pins):**

| Display Pin | Pin # | Function         | RPi GPIO | RPi Pin # | Notes                    |
|-------------|-------|------------------|----------|-----------|--------------------------|
| GND         | 1     | Ground           | GND      | Pin 6     | Ground connection        |
| VCC         | 2     | Power            | 3.3V     | Pin 1     | 3.3V power supply        |
| SCL         | 3     | SPI Clock        | GPIO 11  | Pin 23    | SPI0 SCLK                |
| SDA         | 4     | SPI Data         | GPIO 10  | Pin 19    | SPI0 MOSI                |
| RES         | 5     | Reset            | GPIO 24  | Pin 18    | Hardware reset           |
| DC/RS       | 6     | Data/Command     | GPIO 25  | Pin 22    | Data/Command select      |
| CS          | 7     | Chip Select      | GPIO 8   | Pin 24    | SPI0 CE0                 |
| BL          | 8     | Backlight        | GPIO 18  | Pin 12    | Backlight control (PWM)  |

### Pin Configuration Summary

**Control Pins (as defined in the code):**
```
GPIO 25 - DC (Data/Command)
GPIO 24 - RESET
GPIO 18 - Backlight
```

**SPI Pins (Hardware SPI0):**
```
GPIO 8  - CE0 (Chip Select)
GPIO 10 - MOSI (Master Out Slave In)
GPIO 11 - SCLK (Clock)
GPIO 9  - MISO (Master In Slave Out) - Optional for this display
```

### Visual Pin Layout (Raspberry Pi 4B)

```
    3V3  (1) (2)  5V        <- Display Pin 2 (VCC)
  GPIO2  (3) (4)  5V
  GPIO3  (5) (6)  GND       <- Display Pin 1 (GND)
  GPIO4  (7) (8)  GPIO14
    GND  (9) (10) GPIO15
 GPIO17 (11) (12) GPIO18    <- Display Pin 8 (BL - Backlight)
 GPIO27 (13) (14) GND
 GPIO22 (15) (16) GPIO23
    3V3 (17) (18) GPIO24    <- Display Pin 5 (RES - Reset)
 GPIO10 (19) (20) GND       <- Display Pin 4 (SDA - MOSI)
  GPIO9 (21) (22) GPIO25    <- Display Pin 6 (DC/RS)
 GPIO11 (23) (24) GPIO8     <- Display Pin 3 (SCL)   <- Display Pin 7 (CS)
    GND (25) (26) GPIO7
  ...
```

### Important Notes on Wiring

1. **Voltage**: The ST7789 operates at 3.3V. DO NOT connect to 5V as it may damage the display.
2. **Ground**: Ensure a solid ground connection between the Pi and display.
3. **SPI**: Uses hardware SPI0 (/dev/spidev0.0).
4. **Rotation**: Code is configured for 90° rotation (landscape mode).
5. **Double-check connections** before powering on to avoid damage.

---

## Hardware Setup

### Step 1: Prepare the Raspberry Pi

1. Install **Raspberry Pi OS** (Bullseye or newer recommended)
   - Download from: https://www.raspberrypi.com/software/
   - Use Raspberry Pi Imager to flash to SD card
   - Enable SSH if you plan to access remotely

2. Boot up the Raspberry Pi and log in

### Step 2: Enable SPI Interface

SPI must be enabled for the display to work.

**Method 1: Using raspi-config (Recommended)**
```bash
sudo raspi-config
```
- Navigate to: `Interface Options` → `SPI` → `Yes`
- Reboot when prompted

**Method 2: Manual Configuration**
```bash
# Edit the boot configuration
sudo nano /boot/config.txt

# Add or uncomment this line:
dtparam=spi=on

# Save and reboot
sudo reboot
```

### Step 3: Verify SPI is Enabled

After reboot, check if SPI device exists:
```bash
ls /dev/spi*
```

You should see: `/dev/spidev0.0` and `/dev/spidev0.1`

### Step 4: Wire the Display

1. **Power off** the Raspberry Pi completely
2. Connect the display following the [GPIO Pin Connections](#gpio-pin-connections) table
3. **Double-check all connections**
4. Power on the Raspberry Pi

---

## Software Installation

### Step 1: Clone or Download the Project

```bash
# Create a directory for the project
mkdir -p ~/projects
cd ~/projects

# If you have the files, navigate to that directory
# Or copy all files (clock.cpp, failsafe.cpp, test_display.cpp, build.sh, start.sh)
```

### Step 2: Set Permissions

Make the scripts executable:
```bash
chmod +x build.sh start.sh
```

---

## Building the Project

The `build.sh` script handles everything automatically:

```bash
./build.sh
```

This script will:
- ✓ Check if running on Raspberry Pi
- ✓ Detect Linux version and kernel
- ✓ Check for required dependencies (g++, make, etc.)
- ✓ Install missing packages if needed (requires sudo)
- ✓ Verify SPI is enabled and accessible
- ✓ Verify GPIO access
- ✓ Compile all three programs:
  - `clock` - Main clock application
  - `failsafe` - Failsafe wrapper
  - `test_display` - Display test utility
- ✓ Create a systemd service file template

### Build Output

If successful, you'll see:
```
====================================
Build Summary
====================================
✓ All components built successfully!

Built binaries:
  clock (42K)
  failsafe (38K)
  test_display (40K)
```

---

## Running the Clock

### Option 1: Test the Display First (Recommended)

Before running the clock, test that the display works:

```bash
sudo ./test_display
```

This will run through 10 test phases:
1. SPI device check
2. GPIO setup
3. Display initialization
4. SPI communication test
5. Backlight test (flashes 3 times)
6. Color fills (red, green, blue, white, black)
7. Color bars
8. Gradient pattern
9. Checkerboard patterns
10. Stress test (rapid color changes)

### Option 2: Run the Clock Directly

```bash
sudo ./clock
```

### Option 3: Run with Failsafe Wrapper (Recommended for Production)

The failsafe wrapper automatically restarts the clock if it crashes:

```bash
sudo ./failsafe ./clock
```

### Option 4: Use the Start Script (Best Option)

The `start.sh` script handles environment setup and uses failsafe:

```bash
sudo ./start.sh
```

This script:
- Checks all prerequisites
- Loads SPI module if needed
- Sets CPU governor to performance
- Starts the clock with failsafe
- Logs output to `logs/` directory

---

## Permissions (Running Without Sudo)

To run without `sudo`, add your user to the required groups:

```bash
# Add user to gpio and spi groups
sudo usermod -a -G gpio,spi $USER

# Log out and log back in for changes to take effect
```

After logging back in, you can run:
```bash
./start.sh
```

---

## Autostart Configuration

### Option 1: Systemd Service (Recommended)

The build script creates a service file template. To install it:

```bash
# Copy the service file
sudo cp /tmp/ili9340-clock.service /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable the service to start on boot
sudo systemctl enable ili9340-clock.service

# Start the service now
sudo systemctl start ili9340-clock.service

# Check status
sudo systemctl status ili9340-clock.service
```

**Service Management:**
```bash
# Stop the clock
sudo systemctl stop ili9340-clock.service

# Restart the clock
sudo systemctl restart ili9340-clock.service

# Disable autostart
sudo systemctl disable ili9340-clock.service

# View logs
sudo journalctl -u ili9340-clock.service -f
```

### Option 2: Cron (Alternative)

```bash
# Edit crontab
crontab -e

# Add this line to run at boot:
@reboot sleep 30 && /home/pi/projects/clockdemo/start.sh
```

---

## Troubleshooting

### Display Shows Nothing

1. **Check Power**
   - Verify 3.3V connection
   - Check ground connection

2. **Check SPI**
   ```bash
   ls -l /dev/spidev0.0
   lsmod | grep spi
   ```

3. **Check Wiring**
   - Verify all pins match the table
   - Check for loose connections

4. **Run Test**
   ```bash
   sudo ./test_display
   ```

### "Cannot open SPI device" Error

```bash
# Enable SPI
sudo raspi-config
# Interface Options → SPI → Yes

# Reboot
sudo reboot
```

### Display Shows Red Screen

Red screen indicates the failsafe detected an error. Check logs:
```bash
cat /tmp/clock_failsafe.log
```

### Clock Crashes or Restarts

1. **Check Power Supply**: Ensure you have a good 3A power supply
2. **Check Connections**: Verify all wiring is secure
3. **View Logs**:
   ```bash
   # If using systemd:
   sudo journalctl -u ili9340-clock.service -n 50

   # If using start.sh:
   ls -lrt logs/
   tail -f logs/clock_*.log
   ```

### Wrong Colors or Garbled Display

1. **Check SPI Speed**: May need to reduce speed in code
2. **Check Wiring**: Loose MOSI or SCLK connections
3. **Power Issues**: Insufficient power or voltage drop

### GPIO Permission Errors

```bash
# Add to gpio group
sudo usermod -a -G gpio $USER

# Or run with sudo
sudo ./start.sh
```

### Backlight Not Working

Check GPIO 18 connection:
```bash
# Test manually
echo 18 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio18/direction
echo 1 > /sys/class/gpio/gpio18/value  # Backlight ON
echo 0 > /sys/class/gpio/gpio18/value  # Backlight OFF
```

---

## Display Specifications

- **Model**: ST7789
- **Size**: 2.8 inches
- **Resolution**: 320x240 pixels
- **Interface**: SPI
- **Color**: 65K colors (RGB565)
- **Viewing Angle**: 160°
- **Operating Voltage**: 3.3V
- **Backlight**: LED (controllable via GPIO)
- **Orientation**: 90° rotation (landscape mode)

---

## Clock Features

- **Large Digital Time Display** (HH:MM:SS)
- **Date Display** (YYYY-MM-DD)
- **Color-coded** (Cyan time, Yellow date)
- **Automatic Updates** (every second)
- **Clean Shutdown** (Ctrl+C to exit gracefully)
- **Failsafe Recovery** (automatic restart on crash)
- **Low CPU Usage** (sleeps between updates)

---

## Driver Logic

This implementation uses the same approach as [fbcp-ili9341](https://github.com/juj/fbcp-ili9341), adapted for ST7789:

1. **Direct SPI Communication** - Uses `/dev/spidev0.0` for fast SPI access
2. **Hardware SPI** - Leverages Raspberry Pi's hardware SPI (SPI0)
3. **GPIO Control** - Direct GPIO control via sysfs for DC, RESET, and backlight
4. **DMA-free** - Simple ioctl-based SPI transfers (no DMA complexity)
5. **RGB565 Format** - Efficient 16-bit color representation
6. **Optimized Updates** - Only updates when time changes
7. **90° Rotation** - MADCTL configured for landscape orientation (0x60)

---

## File Structure

```
clockdemo/
├── clock.cpp           # Main clock application
├── failsafe.cpp        # Failsafe wrapper with auto-restart
├── test_display.cpp    # Display test utility
├── build.sh           # Build and dependency management script
├── start.sh           # Startup script with environment setup
├── SETUP.md           # This file
├── clock              # Compiled clock binary (after build)
├── failsafe           # Compiled failsafe binary (after build)
├── test_display       # Compiled test binary (after build)
└── logs/              # Log files directory (created at runtime)
```

---

## References

- [fbcp-ili9341 GitHub](https://github.com/juj/fbcp-ili9341) - Original driver inspiration
- [ST7789_TFT_RPI](https://github.com/gavinlyonsrepo/ST7789_TFT_RPI) - ST7789 reference implementation
- [ST7789 Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7789.pdf)
- [Raspberry Pi GPIO](https://www.raspberrypi.com/documentation/computers/os.html#gpio)
- [Raspberry Pi SPI](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#serial-peripheral-interface-spi)

---

## License

This project is provided as-is for educational and personal use.

---

## Support

If you encounter issues:

1. Run the test program: `sudo ./test_display`
2. Check logs in `logs/` directory
3. Verify wiring matches the GPIO table
4. Ensure SPI is enabled in raspi-config
5. Check power supply is adequate (3A for Pi 4)

For hardware issues, consult your display's datasheet and verify compatibility.
