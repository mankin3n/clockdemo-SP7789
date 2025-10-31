# Display Reset Sequence Documentation

## Enhanced Reset at Startup

All programs now include a **thorough reset sequence** at the beginning to ensure the display starts in a clean, known state.

---

## 8-Step Reset and Initialization Sequence

### Step 1: Turn Off Backlight
```
Duration: 50ms
Purpose: Prevents flashing or visual artifacts during initialization
```
The backlight is turned off before any reset or configuration occurs, so the user doesn't see garbage on the screen during startup.

### Step 2: Hardware Reset
```
Duration: 210ms total
Sequence:
  - Set RESET pin HIGH (10ms)
  - Set RESET pin LOW (50ms) ← Active reset
  - Set RESET pin HIGH (150ms) ← Recovery time
```
**Why this matters:**
- Hardware reset puts the ST7789 controller into a known state
- Clears all internal registers and settings
- Resets any previous configuration from prior runs
- Longer LOW pulse ensures complete reset
- Recovery time allows internal circuits to stabilize

### Step 3: Software Reset
```
Command: SWRESET (0x01)
Duration: 200ms wait
```
**Why this matters:**
- Double insurance after hardware reset
- Clears all ST7789 registers to default values
- Essential for ST7789 initialization
- Longer wait time ensures reset completes

### Step 4: Wake Up Display
```
Command: SLPOUT (0x11)
Duration: 120ms wait
```
**Why this matters:**
- Brings display out of sleep mode
- Powers up internal circuits
- Required before configuration commands
- Wait time per ST7789 specification

### Step 5: Configure Display
```
Commands:
  - MADCTL (0x36): Set rotation to 90° (landscape)
  - COLMOD (0x3A): Set 16-bit RGB565 color mode
  - NORON (0x13): Normal display mode
  - INVON (0x21): Enable color inversion (ST7789 specific)
```
**Why this matters:**
- Sets the correct orientation (landscape)
- Configures color format for RGB565
- Enables inversion (often needed for ST7789)
- Ensures consistent display behavior

### Step 6: Clear Screen to Black
```
Action: Fill entire display with black pixels (0x0000)
Duration: ~50ms + fill time
```
**Why this matters:**
- Ensures no garbage pixels from previous state
- Provides clean slate for content
- Prevents flashing or artifacts
- User sees clean black screen before content appears

### Step 7: Turn On Display
```
Command: DISPON (0x29)
Duration: 120ms wait
```
**Why this matters:**
- Activates the display output
- Final step before showing content
- Wait ensures display is ready

### Step 8: Turn On Backlight
```
Duration: 50ms wait
```
**Why this matters:**
- Last step ensures everything is configured first
- User sees clean display with correct content
- No flashing or artifacts visible

---

## Total Initialization Time

**Approximately 700-800ms** (less than 1 second)

This is a good tradeoff between thorough initialization and startup speed.

---

## Why Enhanced Reset is Important

### Problems Without Proper Reset:

1. **Garbage on Screen**
   - Random pixels from previous state
   - Artifacts during initialization
   - Flashing colors

2. **Wrong Configuration**
   - Display in wrong orientation
   - Incorrect color mode
   - Previous settings still active

3. **Inconsistent Behavior**
   - Works sometimes, fails other times
   - Different behavior after power cycle
   - Depends on previous state

4. **Communication Errors**
   - Display not responding
   - Commands ignored
   - SPI communication issues

### Benefits of Enhanced Reset:

✅ **Consistent startup every time**
✅ **Clean black screen before content**
✅ **No visual artifacts**
✅ **Proper orientation from the start**
✅ **All registers in known state**
✅ **Reliable SPI communication**
✅ **Professional appearance**

---

## Reset in Different Programs

### clock.cpp
```
Initializing ST7789 display...
  - Turning off backlight...
  - Performing hardware reset...
  - Sending software reset...
  - Waking up display...
  - Configuring display (90° rotation)...
  - Clearing screen to black...
  - Turning on display...
  - Turning on backlight...
Display initialization complete!
```

### test_display.cpp
Same sequence, with verbose output at each step so you can see the progress.

### failsafe.cpp
- Uses same reset in `display_error_screen()` for error recovery
- Uses hardware-only reset in `reset_display_hardware()` between restarts

---

## Timing Details

| Step | Duration | Cumulative |
|------|----------|------------|
| Backlight OFF | 50ms | 50ms |
| Hardware Reset | 210ms | 260ms |
| Software Reset | 200ms | 460ms |
| Sleep Out | 120ms | 580ms |
| Configuration | 30ms | 610ms |
| Clear Screen | 50ms+ | 660ms+ |
| Display ON | 120ms | 780ms+ |
| Backlight ON | 50ms | 830ms+ |

**Total: ~830ms** (may vary slightly based on system)

---

## Technical Specifications

### Hardware Reset Timing
Based on ST7789 datasheet:
- Reset pulse width: Min 10μs (we use 50ms = 50,000μs)
- Reset recovery: Min 5ms (we use 150ms)

### Software Reset Timing
Based on ST7789 datasheet:
- Reset execution: 5ms typical
- Wait time: Min 120ms (we use 200ms for safety)

### Sleep Out Timing
Based on ST7789 datasheet:
- Sleep out time: 120ms typical
- We wait exactly 120ms

---

## Debugging Reset Issues

### If display stays black:
1. Check backlight connection (GPIO 18)
2. Verify reset pin connection (GPIO 24)
3. Check power (3.3V, not 5V!)
4. Run test: `sudo ./test_display`

### If display shows garbage:
1. Reset timing may be too short
2. SPI communication issues
3. Try increasing wait times in code

### If colors are wrong:
1. INVON may need to be disabled
2. MADCTL value may need adjustment
3. Check RGB vs BGR order

---

## Source Code Locations

Reset sequence implemented in:
- **clock.cpp**: Line ~120-193 (`init_display()`)
- **failsafe.cpp**: Line ~139-218 (`display_error_screen()`)
- **test_display.cpp**: Line ~120-171 (`init_display()`)

---

## Customization

### To Change Reset Timing:

Edit the `usleep()` values (in microseconds):
- `usleep(50000)` = 50ms
- `usleep(100000)` = 100ms
- `usleep(200000)` = 200ms

### To Skip Screen Clear:

Comment out Step 6 in `init_display()`:
```cpp
// Step 6: Clear screen to black before turning on
// std::cout << "  - Clearing screen to black..." << std::endl;
// set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
// gpio_write(GPIO_TFT_DATA_CONTROL, 1);
// ...
```

### To Disable Inversion:

Change in `init_display()`:
```cpp
// spi_write_command(ST7789_INVON);  // Comment this out
spi_write_command(ST7789_INVOFF);    // Add this instead
```

---

## Summary

The enhanced reset sequence ensures your ST7789 display:
- ✅ Starts in a clean, predictable state
- ✅ Shows no visual artifacts
- ✅ Has correct orientation (90° landscape)
- ✅ Responds reliably to commands
- ✅ Looks professional at startup

**Total time: Less than 1 second for rock-solid initialization!**
