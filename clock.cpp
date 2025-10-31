// Digital Clock for ST7789 Display (320x240, 90° rotation)
// Using ST7789_TFT_RPI driver architecture with bcm2835 library

#include <bcm2835.h>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <signal.h>
#include <unistd.h>

// Display specifications (320x240 with 90° rotation = landscape)
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_DRAWABLE_WIDTH 320
#define DISPLAY_DRAWABLE_HEIGHT 240

// GPIO Pin Configuration (Software SPI - matches ST7789_TFT_RPI SW SPI setup)
#define TFT_CS_GPIO    RPI_BPLUS_GPIO_J8_32  // GPIO12 - CS/SS pin
#define TFT_DC_GPIO    RPI_BPLUS_GPIO_J8_18  // GPIO24 - DC pin
#define TFT_RST_GPIO   RPI_BPLUS_GPIO_J8_22  // GPIO25 - RESET pin
#define TFT_SDATA_GPIO RPI_BPLUS_GPIO_J8_35  // GPIO19 - MOSI/SDA pin
#define TFT_SCLK_GPIO  RPI_BPLUS_GPIO_J8_37  // GPIO26 - SCLK pin
// Note: LED/Backlight connected to VCC (always on, no GPIO control needed)

// SPI Communication Delay (for software SPI bit-banging)
#define TFT_HIGHFREQ_DELAY 0  // Microseconds delay between bit operations

// ST7789 Commands
#define ST7789_NOP 0x00
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT 0x11
#define ST7789_NORON 0x13
#define ST7789_INVON 0x21
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_MADCTL 0x36
#define ST7789_COLMOD 0x3A

// Colors (RGB565 format)
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW 0xFFE0

// Global variables
volatile bool running = true;
uint16_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];

// Signal handler for clean shutdown
void signal_handler(int signo) {
    running = false;
}

// ST7789 Display Driver Class (following ST7789_TFT_RPI architecture)
class ST7789_Driver {
private:
    uint16_t _highFreqDelay;

    // Software SPI bit-banging (matching ST7789_TFT_RPI implementation)
    void spiWriteByte(uint8_t byte) {
        for (int i = 7; i >= 0; i--) {
            bcm2835_gpio_write(TFT_SCLK_GPIO, LOW);
            if (byte & (1 << i)) {
                bcm2835_gpio_write(TFT_SDATA_GPIO, HIGH);
            } else {
                bcm2835_gpio_write(TFT_SDATA_GPIO, LOW);
            }
            if (_highFreqDelay != 0) {
                bcm2835_delayMicroseconds(_highFreqDelay);
            }
            bcm2835_gpio_write(TFT_SCLK_GPIO, HIGH);
            if (_highFreqDelay != 0) {
                bcm2835_delayMicroseconds(_highFreqDelay);
            }
        }
    }

    void writeCommand(uint8_t cmd) {
        bcm2835_gpio_write(TFT_DC_GPIO, LOW);  // Command mode
        bcm2835_gpio_write(TFT_CS_GPIO, LOW);  // CS low (select)
        spiWriteByte(cmd);
        bcm2835_gpio_write(TFT_CS_GPIO, HIGH); // CS high (deselect)
    }

    void writeData(uint8_t data) {
        bcm2835_gpio_write(TFT_DC_GPIO, HIGH); // Data mode
        bcm2835_gpio_write(TFT_CS_GPIO, LOW);  // CS low (select)
        spiWriteByte(data);
        bcm2835_gpio_write(TFT_CS_GPIO, HIGH); // CS high (deselect)
    }

public:
    ST7789_Driver() : _highFreqDelay(TFT_HIGHFREQ_DELAY) {}

    // Setup GPIO pins for Software SPI (matching TFTSetupGPIO for SW SPI)
    bool setupGPIO() {
        std::cout << "Initializing bcm2835 library..." << std::endl;
        if (!bcm2835_init()) {
            std::cerr << "Error: bcm2835_init failed. Are you running as root?" << std::endl;
            return false;
        }

        // Set GPIO pin modes to output
        bcm2835_gpio_fsel(TFT_CS_GPIO, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_fsel(TFT_DC_GPIO, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_fsel(TFT_RST_GPIO, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_fsel(TFT_SDATA_GPIO, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_fsel(TFT_SCLK_GPIO, BCM2835_GPIO_FSEL_OUTP);

        // Initialize pin states
        bcm2835_gpio_write(TFT_CS_GPIO, HIGH);    // CS high (deselected)
        bcm2835_gpio_write(TFT_SCLK_GPIO, LOW);   // Clock low
        bcm2835_gpio_write(TFT_SDATA_GPIO, LOW);  // Data low

        std::cout << "GPIO Setup Complete - Software SPI Mode" << std::endl;
        std::cout << "  CS:    GPIO12 (Pin 32)" << std::endl;
        std::cout << "  DC:    GPIO24 (Pin 18)" << std::endl;
        std::cout << "  RESET: GPIO25 (Pin 22)" << std::endl;
        std::cout << "  MOSI:  GPIO19 (Pin 35)" << std::endl;
        std::cout << "  SCLK:  GPIO26 (Pin 37)" << std::endl;

        return true;
    }

    // Initialize display (matching ST7789_TFT_RPI initialization sequence)
    void initDisplay() {
        std::cout << "Initializing ST7789 display..." << std::endl;

        // Hardware reset sequence
        std::cout << "  - Performing hardware reset..." << std::endl;
        bcm2835_gpio_write(TFT_RST_GPIO, HIGH);
        bcm2835_delay(10);
        bcm2835_gpio_write(TFT_RST_GPIO, LOW);
        bcm2835_delay(50);
        bcm2835_gpio_write(TFT_RST_GPIO, HIGH);
        bcm2835_delay(150);

        // Software reset
        std::cout << "  - Sending software reset..." << std::endl;
        writeCommand(ST7789_SWRESET);
        bcm2835_delay(200);

        // Sleep out
        std::cout << "  - Waking up display..." << std::endl;
        writeCommand(ST7789_SLPOUT);
        bcm2835_delay(120);

        // Configure display orientation and format
        std::cout << "  - Configuring display (90° rotation)..." << std::endl;

        // Memory Access Control (90° rotation)
        writeCommand(ST7789_MADCTL);
        writeData(0x60);  // 90° rotation, RGB order

        // Pixel Format: 16 bits/pixel (RGB565)
        writeCommand(ST7789_COLMOD);
        writeData(0x55);  // 16-bit color

        // Normal display mode
        writeCommand(ST7789_NORON);
        bcm2835_delay(10);

        // Inversion on
        writeCommand(ST7789_INVON);
        bcm2835_delay(10);

        // Display on
        std::cout << "  - Turning on display..." << std::endl;
        writeCommand(ST7789_DISPON);
        bcm2835_delay(120);

        std::cout << "Display initialization complete!" << std::endl;
    }

    // Set address window
    void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
        writeCommand(ST7789_CASET);
        writeData(x0 >> 8);
        writeData(x0 & 0xFF);
        writeData(x1 >> 8);
        writeData(x1 & 0xFF);

        writeCommand(ST7789_RASET);
        writeData(y0 >> 8);
        writeData(y0 & 0xFF);
        writeData(y1 >> 8);
        writeData(y1 & 0xFF);

        writeCommand(ST7789_RAMWR);
    }

    // Push pixel data
    void pushPixel(uint16_t color) {
        bcm2835_gpio_write(TFT_DC_GPIO, HIGH); // Data mode
        bcm2835_gpio_write(TFT_CS_GPIO, LOW);  // CS low
        spiWriteByte(color >> 8);
        spiWriteByte(color & 0xFF);
        bcm2835_gpio_write(TFT_CS_GPIO, HIGH); // CS high
    }

    // Push framebuffer to display
    void pushFramebuffer(uint16_t* framebuffer, int width, int height) {
        setAddrWindow(0, 0, width - 1, height - 1);

        bcm2835_gpio_write(TFT_DC_GPIO, HIGH); // Data mode
        bcm2835_gpio_write(TFT_CS_GPIO, LOW);  // CS low

        for (int i = 0; i < width * height; i++) {
            uint16_t pixel = framebuffer[i];
            spiWriteByte(pixel >> 8);
            spiWriteByte(pixel & 0xFF);
        }

        bcm2835_gpio_write(TFT_CS_GPIO, HIGH); // CS high
    }

    // Cleanup
    void powerDown() {
        bcm2835_close();
    }
};

// Draw a filled rectangle
void draw_rect(int x, int y, int w, int h, uint16_t color) {
    for (int j = y; j < y + h && j < DISPLAY_HEIGHT; j++) {
        for (int i = x; i < x + w && i < DISPLAY_WIDTH; i++) {
            framebuffer[j * DISPLAY_WIDTH + i] = color;
        }
    }
}

// Simple 5x7 font drawing (digits only)
const uint8_t font_5x7[10][7] = {
    {0x1F, 0x11, 0x11, 0x11, 0x1F}, // 0
    {0x00, 0x00, 0x1F, 0x00, 0x00}, // 1
    {0x1D, 0x15, 0x15, 0x15, 0x17}, // 2
    {0x11, 0x15, 0x15, 0x15, 0x1F}, // 3
    {0x07, 0x04, 0x04, 0x1F, 0x04}, // 4
    {0x17, 0x15, 0x15, 0x15, 0x1D}, // 5
    {0x1F, 0x15, 0x15, 0x15, 0x1D}, // 6
    {0x01, 0x01, 0x01, 0x01, 0x1F}, // 7
    {0x1F, 0x15, 0x15, 0x15, 0x1F}, // 8
    {0x17, 0x15, 0x15, 0x15, 0x1F}  // 9
};

void draw_char(int x, int y, char c, uint16_t color, int scale) {
    if (c < '0' || c > '9') {
        if (c == ':') {
            // Draw colon as two dots
            for (int dy = 0; dy < scale; dy++) {
                for (int dx = 0; dx < scale; dx++) {
                    framebuffer[(y + 1 * scale + dy) * DISPLAY_WIDTH + (x + 2 * scale + dx)] = color;
                    framebuffer[(y + 5 * scale + dy) * DISPLAY_WIDTH + (x + 2 * scale + dx)] = color;
                }
            }
        }
        return;
    }

    int digit = c - '0';
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (font_5x7[digit][row] & (1 << col)) {
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        int px = x + col * scale + dx;
                        int py = y + row * scale + dy;
                        if (px < DISPLAY_WIDTH && py < DISPLAY_HEIGHT) {
                            framebuffer[py * DISPLAY_WIDTH + px] = color;
                        }
                    }
                }
            }
        }
    }
}

void draw_text(int x, int y, const char* text, uint16_t color, int scale) {
    int cursor_x = x;
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(cursor_x, y, text[i], color, scale);
        cursor_x += (text[i] == ':' ? 4 : 6) * scale;
    }
}

int main() {
    std::cout << "==============================================\n";
    std::cout << "Digital Clock for ST7789 Display\n";
    std::cout << "Using ST7789_TFT_RPI driver architecture\n";
    std::cout << "==============================================\n" << std::endl;

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create driver instance
    ST7789_Driver display;

    // Setup GPIO and initialize display
    if (!display.setupGPIO()) {
        return 1;
    }

    display.initDisplay();

    std::cout << "Display initialized. Starting clock..." << std::endl;

    time_t last_second = 0;

    while (running) {
        time_t now = time(nullptr);

        // Update only when second changes
        if (now != last_second) {
            last_second = now;

            struct tm* timeinfo = localtime(&now);
            char time_str[16];
            snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                     timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

            char date_str[32];
            snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d",
                     timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);

            // Clear framebuffer
            draw_rect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, COLOR_BLACK);

            // Draw time (large, centered)
            int time_scale = 8;
            int time_width = strlen(time_str) * 6 * time_scale;
            int time_x = (DISPLAY_WIDTH - time_width) / 2;
            int time_y = 60;
            draw_text(time_x, time_y, time_str, COLOR_CYAN, time_scale);

            // Draw date (smaller, centered below)
            int date_scale = 3;
            int date_width = strlen(date_str) * 6 * date_scale;
            int date_x = (DISPLAY_WIDTH - date_width) / 2;
            int date_y = 160;
            draw_text(date_x, date_y, date_str, COLOR_YELLOW, date_scale);

            // Update display
            display.pushFramebuffer(framebuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        }

        usleep(100000); // Sleep 100ms
    }

    // Cleanup
    std::cout << "\nShutting down..." << std::endl;
    display.powerDown();

    return 0;
}
