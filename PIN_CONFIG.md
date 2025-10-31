# ST7789 Pin Configuration

## ✅ Updated Pin Configuration (from ST7789_TFT_RPI)

All code has been updated to match the pin configuration from the reference repository [ST7789_TFT_RPI](https://github.com/gavinlyonsrepo/ST7789_TFT_RPI).

---

## 📌 Correct Pin Mapping

### Display Pin Order (8 pins on your display)

| TFT Pin # | Pin Description | RPI HW SPI 0  | RPi Pin # | Notes |
|-----------|-----------------|---------------|-----------|-------|
| 1 | LED (Backlight) | VCC | 3.3V (Pin 1 or 17) | **Connected to VCC (always on)** |
| 2 | SS/CS | GPIO 8 (SPI_CE0) | Pin 24 | Chip Select |
| 3 | DC | GPIO 24 | Pin 18 | Data/Command select |
| 4 | RESET | GPIO 25 | Pin 18 | Hardware Reset |
| 5 | SDA (MOSI) | GPIO 10 (SPI_MOSI) | Pin 19 | SPI Data |
| 6 | SCLK | GPIO 11 (SPI_CLK) | Pin 23 | SPI Clock |
| 7 | VCC | VCC | 3.3V (Pin 1 or 17) | Power |
| 8 | GND | GND | Pin 6, 9, 14, etc | Ground |

---

## 🔄 What Changed

### Old (Incorrect) Pin Configuration:
```cpp
#define GPIO_TFT_DATA_CONTROL 25  // ❌ WRONG
#define GPIO_TFT_RESET_PIN 24     // ❌ WRONG
#define GPIO_TFT_BACKLIGHT 18     // ❌ Not used in this setup
```

### New (Correct) Pin Configuration:
```cpp
#define GPIO_TFT_DATA_CONTROL 24  // ✅ DC pin
#define GPIO_TFT_RESET_PIN 25     // ✅ RESET pin
// Note: LED/Backlight connected to VCC (always on)
```

**Key Changes:**
1. **DC (Data/Command)**: Changed from GPIO 25 → **GPIO 24**
2. **RESET**: Changed from GPIO 24 → **GPIO 25**
3. **Backlight**: No longer controlled by GPIO 18 - now connected to **VCC (always on)**

---

## 🔌 Wiring Instructions

### Physical Connections

```
Display Pin 1 (LED)   → RPi Pin 1 or 17 (3.3V)  ⚠️ Always on
Display Pin 2 (CS)    → RPi Pin 24 (GPIO 8)
Display Pin 3 (DC)    → RPi Pin 18 (GPIO 24)
Display Pin 4 (RESET) → RPi Pin 18 (GPIO 25)    ⚠️ Note: Different physical pin!
Display Pin 5 (SDA)   → RPi Pin 19 (GPIO 10)
Display Pin 6 (SCLK)  → RPi Pin 23 (GPIO 11)
Display Pin 7 (VCC)   → RPi Pin 1 or 17 (3.3V)
Display Pin 8 (GND)   → RPi Pin 6, 9, 14, 20, 25, 30, 34, or 39 (GND)
```

### Important Notes

1. **Backlight (LED) Always On**
   - Connected directly to 3.3V (VCC)
   - No GPIO control
   - Display backlight is always active when powered

2. **Voltage Warning**
   - Use **3.3V only**, NOT 5V!
   - 5V will damage the display

3. **Physical Pin Confusion**
   - GPIO 24 is on **Physical Pin 18**
   - GPIO 25 is on **Physical Pin 22**
   - Don't confuse GPIO numbers with physical pin numbers!

---

## 🗺️ Raspberry Pi GPIO Layout

```
         3.3V [●1    2●] 5V ◄── VCC (Display Pin 1 & 7)
              [●3    4●] 5V
              [●5    6●] GND ◄── GND (Display Pin 8)
              [●7    8●]
              [●9   10●]
              [●11  12●]
              [●13  14●]
              [●15  16●]
         3.3V [●17  18●] GPIO24 ◄── DC (Display Pin 3)
      GPIO10  [●19  20●]         ◄── SDA (Display Pin 5)
              [●21  22●] GPIO25  ◄── RESET (Display Pin 4)
      GPIO11  [●23  24●] GPIO8   ◄── SCLK (Display Pin 6)  ◄── CS (Display Pin 2)
              [●25  26●]
```

---

## 💻 Code Changes Summary

### Files Modified:
1. ✅ **clock.cpp** - Updated GPIO pins, removed backlight control
2. ✅ **failsafe.cpp** - Updated GPIO pins, removed backlight control
3. ✅ **test_display.cpp** - Updated GPIO pins, modified backlight test

### Specific Changes:

#### GPIO Definitions
All three files updated:
```cpp
// Before
#define GPIO_TFT_DATA_CONTROL 25
#define GPIO_TFT_RESET_PIN 24
#define GPIO_TFT_BACKLIGHT 18

// After
#define GPIO_TFT_DATA_CONTROL 24  // DC pin
#define GPIO_TFT_RESET_PIN 25     // RESET pin
// Note: LED/Backlight connected to VCC (always on)
```

#### GPIO Setup
Removed backlight GPIO export and direction setting:
```cpp
// Before
gpio_export(GPIO_TFT_DATA_CONTROL);
gpio_export(GPIO_TFT_RESET_PIN);
gpio_export(GPIO_TFT_BACKLIGHT);  // ❌ Removed

// After
gpio_export(GPIO_TFT_DATA_CONTROL);
gpio_export(GPIO_TFT_RESET_PIN);
```

#### Initialization
Removed backlight control from init_display():
```cpp
// Before - had these lines:
gpio_write(GPIO_TFT_BACKLIGHT, 0);  // ❌ Removed
gpio_write(GPIO_TFT_BACKLIGHT, 1);  // ❌ Removed

// After - backlight is always on via VCC connection
// No GPIO control needed
```

#### Test Updates
Modified backlight test in test_display.cpp:
```cpp
// Before - tested backlight on/off
void test_backlight() {
    // Toggled backlight...
}

// After - skips test with explanation
void test_backlight() {
    std::cout << "Skipping backlight control test..." << std::endl;
    std::cout << "  Note: Backlight is connected to VCC (always on)" << std::endl;
}
```

---

## ✅ Benefits of This Configuration

### 1. Simpler Wiring
- One less GPIO pin to manage
- Backlight directly powered

### 2. Always-On Display
- No need to control backlight in code
- Display immediately visible when powered

### 3. Matches Reference Implementation
- Compatible with ST7789_TFT_RPI examples
- Standard pin configuration

### 4. Fewer GPIO Operations
- Faster initialization
- Less code complexity
- Fewer potential failure points

---

## 🔍 Verification

### After wiring, verify your connections:

1. **Power Off** Raspberry Pi
2. **Check Each Wire** against the table above
3. **Double-Check**:
   - ✅ Display Pin 1 (LED) → 3.3V (not 5V!)
   - ✅ Display Pin 3 (DC) → GPIO 24 (Physical Pin 18)
   - ✅ Display Pin 4 (RESET) → GPIO 25 (Physical Pin 22)
   - ✅ Display Pin 7 (VCC) → 3.3V (not 5V!)
   - ✅ Display Pin 8 (GND) → GND
4. **Power On** and test

---

## 🚀 Testing

After updating the wiring:

```bash
# Rebuild with new pin configuration
./build.sh

# Test display (should work with new pins)
sudo ./test_display

# Run clock
sudo ./clock
```

### Expected Behavior:
- ✅ Display backlight is immediately on
- ✅ No backlight flashing during init
- ✅ Display shows test patterns correctly
- ✅ Clock displays time and date

---

## ❗ Troubleshooting

### Display stays black
**Possible causes:**
- Backlight not connected to VCC
- Wrong DC or RESET pin
- No power to display

**Fix:**
- Check LED pin is connected to 3.3V
- Verify GPIO 24 (DC) and GPIO 25 (RESET) connections
- Measure voltage at VCC pins (should be 3.3V)

### Display shows garbage
**Possible causes:**
- Wrong DC pin
- SPI pins incorrect

**Fix:**
- Verify DC is on GPIO 24 (Physical Pin 18)
- Check SDA → GPIO 10, SCLK → GPIO 11

### Display dims or flickers
**Possible causes:**
- Insufficient power
- Poor connection

**Fix:**
- Use adequate power supply (3A for Pi 4)
- Check all connections are tight
- Connect VCC to multiple 3.3V pins if needed

---

## 📋 Quick Checklist

Before running:
- [ ] DC connected to GPIO 24 (Physical Pin 18)
- [ ] RESET connected to GPIO 25 (Physical Pin 22)
- [ ] LED connected to 3.3V (always on)
- [ ] VCC connected to 3.3V (NOT 5V!)
- [ ] GND connected properly
- [ ] SDA → GPIO 10, SCLK → GPIO 11, CS → GPIO 8
- [ ] Code rebuilt with `./build.sh`
- [ ] SPI enabled in raspi-config

---

## 📚 Reference

Based on pin configuration from:
- [ST7789_TFT_RPI main.cpp](https://github.com/gavinlyonsrepo/ST7789_TFT_RPI)
- Hardware SPI 0 mode
- Standard Raspberry Pi 4B pinout

---

## Summary

**Key Changes:**
- ✅ DC: GPIO 25 → GPIO 24
- ✅ RESET: GPIO 24 → GPIO 25
- ✅ Backlight: GPIO 18 control → VCC (always on)

**No Functional Loss:**
- Display works the same
- Simpler code
- Matches reference implementation
- Standard configuration

**Ready to use!** 🎉
