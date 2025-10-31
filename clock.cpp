// Digital Clock for ST7789 Display (320x240, 90° rotation)
// Based on fbcp-ili9341 driver logic, adapted for ST7789

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <signal.h>

// Display specifications (320x240 with 90° rotation = landscape)
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_DRAWABLE_WIDTH 320
#define DISPLAY_DRAWABLE_HEIGHT 240

// SPI Configuration
#define SPI_BUS_CLOCK_DIVISOR 6
#define GPIO_TFT_DATA_CONTROL 25
#define GPIO_TFT_RESET_PIN 24
#define GPIO_TFT_BACKLIGHT 18

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
int spi_fd = -1;
int gpio_fd = -1;
volatile bool running = true;
uint16_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];

// Signal handler for clean shutdown
void signal_handler(int signo) {
    running = false;
}

// GPIO helper functions (simplified version of fbcp-ili9341 logic)
void gpio_write(int pin, int value) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, value ? "1" : "0", 1);
        close(fd);
    }
}

void gpio_export(int pin) {
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", pin);
        write(fd, buf, strlen(buf));
        close(fd);
        usleep(100000); // Wait for export to complete
    }
}

void gpio_set_direction(int pin, const char* direction) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, direction, strlen(direction));
        close(fd);
    }
}

// SPI communication (based on fbcp-ili9341 approach)
void spi_write_command(uint8_t cmd) {
    gpio_write(GPIO_TFT_DATA_CONTROL, 0); // Command mode
    struct spi_ioc_transfer transfer = {};
    transfer.tx_buf = (unsigned long)&cmd;
    transfer.len = 1;
    transfer.speed_hz = 32000000;
    transfer.bits_per_word = 8;
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
}

void spi_write_data(const uint8_t* data, int len) {
    gpio_write(GPIO_TFT_DATA_CONTROL, 1); // Data mode
    struct spi_ioc_transfer transfer = {};
    transfer.tx_buf = (unsigned long)data;
    transfer.len = len;
    transfer.speed_hz = 32000000;
    transfer.bits_per_word = 8;
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
}

void spi_write_data_u16(uint16_t data) {
    uint8_t bytes[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
    spi_write_data(bytes, 2);
}

// Initialize ST7789 display with thorough reset
void init_display() {
    std::cout << "Initializing ST7789 display..." << std::endl;

    // Step 1: Turn off backlight during initialization
    std::cout << "  - Turning off backlight..." << std::endl;
    gpio_write(GPIO_TFT_BACKLIGHT, 0);
    usleep(50000);

    // Step 2: Hardware reset sequence (ensures clean state)
    std::cout << "  - Performing hardware reset..." << std::endl;
    gpio_write(GPIO_TFT_RESET_PIN, 1);
    usleep(10000);  // Hold high
    gpio_write(GPIO_TFT_RESET_PIN, 0);
    usleep(50000);  // Hold low (reset active)
    gpio_write(GPIO_TFT_RESET_PIN, 1);
    usleep(150000); // Wait for reset to complete

    // Step 3: Software reset (ensures all registers are cleared)
    std::cout << "  - Sending software reset..." << std::endl;
    spi_write_command(ST7789_SWRESET);
    usleep(200000); // Wait longer after software reset

    // Step 4: Wake up display
    std::cout << "  - Waking up display..." << std::endl;
    spi_write_command(ST7789_SLPOUT);
    usleep(120000);

    // Step 5: Configure display orientation and format
    std::cout << "  - Configuring display (90° rotation)..." << std::endl;

    // Memory Access Control (90° rotation = landscape mode)
    // MADCTL bits: MY=0, MX=0, MV=1, ML=0, RGB=1, MH=0
    // MV=1 swaps row/column (rotation), RGB=1 for RGB order
    spi_write_command(ST7789_MADCTL);
    uint8_t madctl = 0x60; // 0x60 = 90° rotation, RGB order
    spi_write_data(&madctl, 1);

    // Pixel Format: 16 bits/pixel (RGB565)
    spi_write_command(ST7789_COLMOD);
    uint8_t colmod = 0x55; // 16-bit color
    spi_write_data(&colmod, 1);

    // Normal display mode
    spi_write_command(ST7789_NORON);
    usleep(10000);

    // Inversion on (often needed for ST7789)
    spi_write_command(ST7789_INVON);
    usleep(10000);

    // Step 6: Clear screen to black before turning on
    std::cout << "  - Clearing screen to black..." << std::endl;
    set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_write(GPIO_TFT_DATA_CONTROL, 1); // Data mode

    uint16_t black = COLOR_BLACK;
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        spi_write_data_u16(black);
    }
    usleep(50000);

    // Step 7: Turn on display
    std::cout << "  - Turning on display..." << std::endl;
    spi_write_command(ST7789_DISPON);
    usleep(120000);

    // Step 8: Turn on backlight
    std::cout << "  - Turning on backlight..." << std::endl;
    gpio_write(GPIO_TFT_BACKLIGHT, 1);
    usleep(50000);

    std::cout << "Display initialization complete!" << std::endl;
}

// Set display window
void set_window(int x0, int y0, int x1, int y1) {
    spi_write_command(ST7789_CASET);
    spi_write_data_u16(x0);
    spi_write_data_u16(x1);

    spi_write_command(ST7789_RASET);
    spi_write_data_u16(y0);
    spi_write_data_u16(y1);

    spi_write_command(ST7789_RAMWR);
}

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

// Update display with framebuffer
void update_display() {
    set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

    // Convert framebuffer to byte array and send
    gpio_write(GPIO_TFT_DATA_CONTROL, 1); // Data mode

    // Send in chunks for stability
    const int chunk_size = 4096;
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i += chunk_size) {
        int remaining = DISPLAY_WIDTH * DISPLAY_HEIGHT - i;
        int count = remaining < chunk_size ? remaining : chunk_size;

        struct spi_ioc_transfer transfer = {};
        transfer.tx_buf = (unsigned long)(&framebuffer[i]);
        transfer.len = count * 2;
        transfer.speed_hz = 32000000;
        transfer.bits_per_word = 8;
        ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
    }
}

int main() {
    std::cout << "Digital Clock for ST7789 Display (320x240, 90° rotation)" << std::endl;

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Open SPI device
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        std::cerr << "Error: Cannot open SPI device /dev/spidev0.0" << std::endl;
        std::cerr << "Make sure SPI is enabled in raspi-config" << std::endl;
        return 1;
    }

    // Configure SPI mode
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 32000000;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // Setup GPIO pins
    gpio_export(GPIO_TFT_DATA_CONTROL);
    gpio_export(GPIO_TFT_RESET_PIN);
    gpio_export(GPIO_TFT_BACKLIGHT);

    usleep(200000);

    gpio_set_direction(GPIO_TFT_DATA_CONTROL, "out");
    gpio_set_direction(GPIO_TFT_RESET_PIN, "out");
    gpio_set_direction(GPIO_TFT_BACKLIGHT, "out");

    // Initialize display
    std::cout << "Initializing display..." << std::endl;
    init_display();

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
            update_display();
        }

        usleep(100000); // Sleep 100ms
    }

    // Cleanup
    std::cout << "\nShutting down..." << std::endl;
    gpio_write(GPIO_TFT_BACKLIGHT, 0);
    close(spi_fd);

    return 0;
}
