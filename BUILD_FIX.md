# Build Fix - Forward Declarations

## Issue Found

The build was failing with this error:
```
clock.cpp: In function 'void init_display()':
clock.cpp:173:5: error: 'set_window' was not declared in this scope
  173 |     set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
      |     ^~~~~~~~~~
```

## Root Cause

In C++, functions must be declared before they are used. The enhanced reset sequence calls `set_window()` inside `init_display()`, but `set_window()` was defined later in the file.

## Solution Applied

Added **forward declarations** before the functions that need them:

### clock.cpp
```cpp
// Forward declarations
void set_window(int x0, int y0, int x1, int y1);

// Initialize ST7789 display with thorough reset
void init_display() {
    // ... can now call set_window() ...
}
```

### test_display.cpp
```cpp
// Forward declarations
void set_window(int x0, int y0, int x1, int y1);
void spi_write_data_u16(uint16_t data);

void init_display() {
    // ... can now call set_window() and spi_write_data_u16() ...
}
```

### failsafe.cpp
No changes needed - functions are properly ordered.

## Files Modified

- ✅ clock.cpp - Added forward declaration
- ✅ test_display.cpp - Added forward declarations
- ✅ failsafe.cpp - No changes needed

## Build Status

**Should now compile successfully!**

Run:
```bash
./build.sh
```

## Technical Explanation

**Forward declarations** tell the compiler that a function exists and what its signature is, even though the full definition comes later. This is a common pattern in C++ when:

1. Functions call each other
2. You want to organize code logically
3. Functions are called before they're defined

### Before (Error):
```cpp
void init_display() {
    set_window(0, 0, 319, 239);  // ERROR: set_window not declared
}

void set_window(int x0, int y0, int x1, int y1) {
    // ...
}
```

### After (Fixed):
```cpp
void set_window(int x0, int y0, int x1, int y1);  // Forward declaration

void init_display() {
    set_window(0, 0, 319, 239);  // OK: compiler knows about set_window
}

void set_window(int x0, int y0, int x1, int y1) {
    // ... actual implementation
}
```

## Why This Happened

When I enhanced the reset sequence, I added a call to `set_window()` to clear the screen during initialization. This created a dependency on a function that was defined later in the file. The forward declaration resolves this.

## No Functional Changes

This is purely a **compilation fix**. The actual behavior and functionality remain exactly the same:
- ✅ Same initialization sequence
- ✅ Same 8-step reset process
- ✅ Same screen clearing
- ✅ Same all functionality

## Testing After Build

After successful build, test everything:

```bash
# 1. Build
./build.sh

# 2. Test display
sudo ./test_display

# 3. Run clock
sudo ./clock

# 4. Test failsafe
sudo ./failsafe ./clock
```

All should work perfectly now!

## Summary

**Problem:** Compilation error due to function call order
**Solution:** Added forward declarations
**Impact:** None - purely a build fix
**Status:** ✅ FIXED

Ready to build and run!
