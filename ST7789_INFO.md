# ST7789 Display Configuration

## ✅ Repository Updated for ST7789

All code has been modified to work with **ST7789** controller with **90° rotation (landscape mode)**.

---

## 🔌 Pin Connections

The **GPIO pin connections remain exactly the same** as before. Your display has 8 pins:

```
Display Pin Layout (from left to right):

Pin 1: GND    → Raspberry Pi Pin 6  (GND)
Pin 2: VCC    → Raspberry Pi Pin 1  (3.3V) ⚠️ NOT 5V!
Pin 3: SCL    → Raspberry Pi Pin 23 (GPIO 11 - SPI Clock)
Pin 4: SDA    → Raspberry Pi Pin 19 (GPIO 10 - SPI MOSI)
Pin 5: RES    → Raspberry Pi Pin 18 (GPIO 24 - Reset)
Pin 6: DC/RS  → Raspberry Pi Pin 22 (GPIO 25 - Data/Command)
Pin 7: CS     → Raspberry Pi Pin 24 (GPIO 8 - Chip Select)
Pin 8: BL     → Raspberry Pi Pin 12 (GPIO 18 - Backlight)
```

### Quick Reference

| Display | Function | RPi GPIO | RPi Pin |
|---------|----------|----------|---------|
| Pin 1 (GND) | Ground | GND | Pin 6 |
| Pin 2 (VCC) | Power | 3.3V | Pin 1 |
| Pin 3 (SCL) | SPI Clock | GPIO 11 | Pin 23 |
| Pin 4 (SDA) | SPI Data | GPIO 10 | Pin 19 |
| Pin 5 (RES) | Reset | GPIO 24 | Pin 18 |
| Pin 6 (DC/RS) | Data/Command | GPIO 25 | Pin 22 |
| Pin 7 (CS) | Chip Select | GPIO 8 | Pin 24 |
| Pin 8 (BL) | Backlight | GPIO 18 | Pin 12 |

**⚠️ CRITICAL**: Connect VCC to 3.3V (Pin 1), **NOT** 5V! Using 5V will damage the display!

---

## 🔄 What Changed

### Code Changes

**All three C++ files updated for ST7789:**

1. **clock.cpp** - Main clock application
   - Changed from ILI9340 commands to ST7789 commands
   - Updated initialization sequence
   - Set MADCTL to 0x60 for 90° rotation (landscape)
   - Added INVON command (inversion on, often needed for ST7789)

2. **failsafe.cpp** - Failsafe wrapper
   - Updated all display commands to ST7789
   - Updated initialization to match ST7789 requirements
   - Set 90° rotation for error screen

3. **test_display.cpp** - Display test suite
   - Updated all commands to ST7789
   - Updated initialization sequence
   - Added 90° rotation configuration

### Key Technical Differences: ST7789 vs ILI9340

| Feature | ST7789 | ILI9340 (Old) |
|---------|--------|---------------|
| Row/Column Set | RASET (0x2B) | PASET (0x2B) |
| Initialization | Requires NORON, INVON | Simpler init |
| MADCTL for 90° | 0x60 | 0x48 |
| Sleep out delay | 120ms | 500ms |
| Inversion | Usually ON | Usually OFF |

### Display Orientation

**90° Rotation (Landscape Mode)**
- Display is rotated 90 degrees from default
- Perfect for horizontal clock display
- MADCTL value: 0x60
  - MV bit (bit 5) = 1: Row/column exchange (rotation)
  - RGB bit (bit 3) = 1: RGB color order

If you want to change rotation in the future:
- 0° (Portrait): MADCTL = 0x00
- 90° (Landscape): MADCTL = 0x60 ✓ (Current)
- 180° (Portrait flipped): MADCTL = 0xC0
- 270° (Landscape flipped): MADCTL = 0xA0

---

## 📄 Documentation Updated

All documentation files updated to reflect ST7789:

- ✅ **README.md** - Main project description
- ✅ **SETUP.md** - Complete setup guide
- ✅ **GPIO_REFERENCE.txt** - Pin reference guide
- ✅ **QUICK_WIRING.txt** - Quick wiring guide
- ✅ **build.sh** - Build script headers
- ✅ **start.sh** - Start script headers
- ✅ **Makefile** - Build file comments

---

## 🚀 Quick Start (Same as Before!)

The usage is exactly the same:

```bash
# 1. Enable SPI
sudo raspi-config
# Interface Options → SPI → Yes

# 2. Reboot
sudo reboot

# 3. Build
./build.sh

# 4. Test display
sudo ./test_display

# 5. Run clock
sudo ./start.sh
```

---

## 🔍 Verification

To verify your display is ST7789, the test script will:
1. Read the display ID (should show ST7789-specific values)
2. Test the initialization sequence
3. Display test patterns in landscape mode

---

## 🎯 What You Should See

When the clock runs:
- Display should be in **landscape orientation** (wider than tall)
- Time displayed **horizontally** in large cyan digits
- Date displayed below in smaller yellow digits
- Backlight should turn on automatically
- Display should update every second

---

## 📚 Technical References

- [ST7789 Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7789.pdf)
- [ST7789_TFT_RPI Reference](https://github.com/gavinlyonsrepo/ST7789_TFT_RPI)
- Original inspiration: [fbcp-ili9341](https://github.com/juj/fbcp-ili9341)

---

## 🔧 Troubleshooting ST7789

### Display shows nothing
- Check VCC is connected to 3.3V (Pin 1), not 5V
- Verify backlight connection (BL → GPIO 18)
- Run test: `sudo ./test_display`

### Colors are inverted
- ST7789 often needs inversion ON (already configured)
- If still wrong, check MADCTL settings

### Display rotated wrong way
- Current setting: 90° (landscape) - MADCTL = 0x60
- To change, modify MADCTL value in init_display() function
- See rotation table above

### Garbled display
- Check SDA (Pin 4) and SCL (Pin 3) connections
- Verify SPI is enabled: `ls /dev/spi*`
- Check power supply is adequate (3A recommended)

---

## ✨ Summary

Your repository is now fully configured for:
- ✅ ST7789 controller
- ✅ 320×240 resolution
- ✅ 90° rotation (landscape mode)
- ✅ Same GPIO pin connections
- ✅ Same build and run process

**No hardware changes needed - just different initialization code for ST7789!**

Enjoy your landscape-mode digital clock! 🕐
