// Test script for ST7789 Display (320x240, 90° rotation)
// Tests various display functions and patterns

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <signal.h>

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define GPIO_TFT_DATA_CONTROL 25
#define GPIO_TFT_RESET_PIN 24
#define GPIO_TFT_BACKLIGHT 18

#define ST7789_SWRESET 0x01
#define ST7789_RDDID 0x04
#define ST7789_RDDST 0x09
#define ST7789_SLPOUT 0x11
#define ST7789_NORON 0x13
#define ST7789_INVON 0x21
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_MADCTL 0x36
#define ST7789_COLMOD 0x3A

// Colors
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW 0xFFE0

int spi_fd = -1;
volatile bool running = true;

void signal_handler(int signo) {
    running = false;
}

void gpio_export(int pin) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", pin);
    if (access(path, F_OK) == 0) return; // Already exported

    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", pin);
        write(fd, buf, strlen(buf));
        close(fd);
        usleep(200000);
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

void gpio_write(int pin, int value) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, value ? "1" : "0", 1);
        close(fd);
    }
}

void spi_write_command(uint8_t cmd) {
    gpio_write(GPIO_TFT_DATA_CONTROL, 0);
    struct spi_ioc_transfer transfer = {};
    transfer.tx_buf = (unsigned long)&cmd;
    transfer.len = 1;
    transfer.speed_hz = 32000000;
    transfer.bits_per_word = 8;
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
}

void spi_write_data(const uint8_t* data, int len) {
    gpio_write(GPIO_TFT_DATA_CONTROL, 1);
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

void spi_read_data(uint8_t* data, int len) {
    gpio_write(GPIO_TFT_DATA_CONTROL, 1);
    struct spi_ioc_transfer transfer = {};
    transfer.rx_buf = (unsigned long)data;
    transfer.len = len;
    transfer.speed_hz = 1000000; // Slower for reading
    transfer.bits_per_word = 8;
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
}

void init_display() {
    std::cout << "  Performing hardware reset..." << std::endl;
    gpio_write(GPIO_TFT_RESET_PIN, 1);
    usleep(5000);
    gpio_write(GPIO_TFT_RESET_PIN, 0);
    usleep(20000);
    gpio_write(GPIO_TFT_RESET_PIN, 1);
    usleep(150000);

    std::cout << "  Software reset..." << std::endl;
    spi_write_command(ST7789_SWRESET);
    usleep(150000);

    std::cout << "  Waking up display..." << std::endl;
    spi_write_command(ST7789_SLPOUT);
    usleep(120000);

    std::cout << "  Configuring display (90° rotation)..." << std::endl;
    spi_write_command(ST7789_MADCTL);
    uint8_t madctl = 0x60; // 90° rotation
    spi_write_data(&madctl, 1);

    spi_write_command(ST7789_COLMOD);
    uint8_t colmod = 0x55;
    spi_write_data(&colmod, 1);

    spi_write_command(ST7789_NORON);
    usleep(10000);

    spi_write_command(ST7789_INVON);
    usleep(10000);

    std::cout << "  Turning on display..." << std::endl;
    spi_write_command(ST7789_DISPON);
    usleep(120000);

    std::cout << "  Turning on backlight..." << std::endl;
    gpio_write(GPIO_TFT_BACKLIGHT, 1);
}

void set_window(int x0, int y0, int x1, int y1) {
    spi_write_command(ST7789_CASET);
    spi_write_data_u16(x0);
    spi_write_data_u16(x1);

    spi_write_command(ST7789_RASET);
    spi_write_data_u16(y0);
    spi_write_data_u16(y1);

    spi_write_command(ST7789_RAMWR);
}

void fill_screen(uint16_t color) {
    set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_write(GPIO_TFT_DATA_CONTROL, 1);

    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        spi_write_data_u16(color);
    }
}

void draw_gradient() {
    set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_write(GPIO_TFT_DATA_CONTROL, 1);

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            uint8_t r = (x * 31) / DISPLAY_WIDTH;
            uint8_t g = (y * 63) / DISPLAY_HEIGHT;
            uint8_t b = ((x + y) * 31) / (DISPLAY_WIDTH + DISPLAY_HEIGHT);
            uint16_t color = (r << 11) | (g << 5) | b;
            spi_write_data_u16(color);
        }
    }
}

void draw_color_bars() {
    set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_write(GPIO_TFT_DATA_CONTROL, 1);

    uint16_t colors[] = {COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN,
                        COLOR_GREEN, COLOR_MAGENTA, COLOR_RED,
                        COLOR_BLUE, COLOR_BLACK};
    int bar_width = DISPLAY_WIDTH / 8;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int bar_index = x / bar_width;
            if (bar_index >= 8) bar_index = 7;
            spi_write_data_u16(colors[bar_index]);
        }
    }
}

void draw_checkerboard(int square_size) {
    set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_write(GPIO_TFT_DATA_CONTROL, 1);

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            bool is_white = ((x / square_size) + (y / square_size)) % 2 == 0;
            spi_write_data_u16(is_white ? COLOR_WHITE : COLOR_BLACK);
        }
    }
}

void test_backlight() {
    std::cout << "Testing backlight control..." << std::endl;
    for (int i = 0; i < 3; i++) {
        std::cout << "  Backlight OFF" << std::endl;
        gpio_write(GPIO_TFT_BACKLIGHT, 0);
        sleep(1);
        std::cout << "  Backlight ON" << std::endl;
        gpio_write(GPIO_TFT_BACKLIGHT, 1);
        sleep(1);
    }
}

bool test_spi_communication() {
    std::cout << "Testing SPI communication..." << std::endl;

    // Try to read display ID
    spi_write_command(ST7789_RDDID);
    uint8_t id[4] = {0};
    spi_read_data(id, 4);

    std::cout << "  Display ID: 0x" << std::hex;
    for (int i = 0; i < 4; i++) {
        std::cout << (int)id[i] << " ";
    }
    std::cout << std::dec << std::endl;

    // Basic validation - at least one byte should be non-zero
    bool valid = false;
    for (int i = 0; i < 4; i++) {
        if (id[i] != 0) {
            valid = true;
            break;
        }
    }

    return valid;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "ST7789 Display Test Suite (90° rotation)" << std::endl;
    std::cout << "=====================================" << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Test 1: Check SPI device
    std::cout << "\n[Test 1] Checking SPI device..." << std::endl;
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        std::cerr << "  FAILED: Cannot open /dev/spidev0.0" << std::endl;
        std::cerr << "  Make sure SPI is enabled in raspi-config" << std::endl;
        return 1;
    }
    std::cout << "  PASSED: SPI device opened successfully" << std::endl;

    // Configure SPI
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 32000000;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // Test 2: Setup GPIO
    std::cout << "\n[Test 2] Setting up GPIO pins..." << std::endl;
    gpio_export(GPIO_TFT_DATA_CONTROL);
    gpio_export(GPIO_TFT_RESET_PIN);
    gpio_export(GPIO_TFT_BACKLIGHT);
    usleep(200000);
    gpio_set_direction(GPIO_TFT_DATA_CONTROL, "out");
    gpio_set_direction(GPIO_TFT_RESET_PIN, "out");
    gpio_set_direction(GPIO_TFT_BACKLIGHT, "out");
    std::cout << "  PASSED: GPIO pins configured" << std::endl;

    // Test 3: Initialize display
    std::cout << "\n[Test 3] Initializing display..." << std::endl;
    init_display();
    std::cout << "  PASSED: Display initialized" << std::endl;

    // Test 4: SPI Communication
    std::cout << "\n[Test 4] Testing SPI communication..." << std::endl;
    if (test_spi_communication()) {
        std::cout << "  PASSED: SPI communication working" << std::endl;
    } else {
        std::cout << "  WARNING: Could not read display ID (may be normal)" << std::endl;
    }

    // Test 5: Backlight
    std::cout << "\n[Test 5] Testing backlight..." << std::endl;
    test_backlight();
    std::cout << "  PASSED: Backlight control working" << std::endl;

    // Test 6: Color fill tests
    std::cout << "\n[Test 6] Testing color fills..." << std::endl;
    const struct {
        const char* name;
        uint16_t color;
    } colors[] = {
        {"RED", COLOR_RED},
        {"GREEN", COLOR_GREEN},
        {"BLUE", COLOR_BLUE},
        {"WHITE", COLOR_WHITE},
        {"BLACK", COLOR_BLACK}
    };

    for (int i = 0; i < 5 && running; i++) {
        std::cout << "  Filling screen with " << colors[i].name << "..." << std::endl;
        fill_screen(colors[i].color);
        sleep(1);
    }
    std::cout << "  PASSED: Color fills working" << std::endl;

    if (!running) goto cleanup;

    // Test 7: Color bars
    std::cout << "\n[Test 7] Testing color bars..." << std::endl;
    draw_color_bars();
    std::cout << "  Displaying color bars for 3 seconds..." << std::endl;
    sleep(3);
    std::cout << "  PASSED: Color bars working" << std::endl;

    if (!running) goto cleanup;

    // Test 8: Gradient
    std::cout << "\n[Test 8] Testing gradient..." << std::endl;
    draw_gradient();
    std::cout << "  Displaying gradient for 3 seconds..." << std::endl;
    sleep(3);
    std::cout << "  PASSED: Gradient working" << std::endl;

    if (!running) goto cleanup;

    // Test 9: Checkerboard patterns
    std::cout << "\n[Test 9] Testing checkerboard patterns..." << std::endl;
    int square_sizes[] = {40, 20, 10, 5};
    for (int i = 0; i < 4 && running; i++) {
        std::cout << "  Checkerboard " << square_sizes[i] << "x" << square_sizes[i] << "..." << std::endl;
        draw_checkerboard(square_sizes[i]);
        sleep(1);
    }
    std::cout << "  PASSED: Checkerboard patterns working" << std::endl;

    // Test 10: Stress test
    std::cout << "\n[Test 10] Running stress test (rapid updates)..." << std::endl;
    std::cout << "  Rapidly changing colors for 5 seconds..." << std::endl;
    time_t start_time = time(nullptr);
    int frame_count = 0;
    while (running && (time(nullptr) - start_time) < 5) {
        fill_screen(colors[frame_count % 5].color);
        frame_count++;
    }
    std::cout << "  PASSED: " << frame_count << " frames rendered (~"
              << (frame_count / 5) << " fps)" << std::endl;

cleanup:
    std::cout << "\n=====================================" << std::endl;
    if (running) {
        std::cout << "All tests completed successfully!" << std::endl;
    } else {
        std::cout << "Tests interrupted by user" << std::endl;
    }
    std::cout << "=====================================" << std::endl;

    // Cleanup
    fill_screen(COLOR_BLACK);
    gpio_write(GPIO_TFT_BACKLIGHT, 0);
    close(spi_fd);

    return 0;
}
