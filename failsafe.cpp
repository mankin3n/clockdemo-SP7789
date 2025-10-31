// Failsafe wrapper for ST7789 Display
// Monitors and recovers from display errors

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <fstream>

#define GPIO_TFT_DATA_CONTROL 24  // DC pin
#define GPIO_TFT_RESET_PIN 25     // RESET pin
// Note: LED/Backlight connected to VCC (always on, no GPIO control needed)

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

#define COLOR_RED 0xF800
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

volatile bool running = true;
pid_t child_pid = 0;

void signal_handler(int signo) {
    running = false;
    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
    }
}

void log_message(const char* message) {
    time_t now = time(nullptr);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    std::cout << "[" << timestamp << "] " << message << std::endl;

    // Also log to file
    std::ofstream log_file("/tmp/clock_failsafe.log", std::ios::app);
    if (log_file.is_open()) {
        log_file << "[" << timestamp << "] " << message << std::endl;
        log_file.close();
    }
}

bool gpio_exists(int pin) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", pin);
    return access(path, F_OK) == 0;
}

void gpio_export(int pin) {
    if (gpio_exists(pin)) {
        return; // Already exported
    }

    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", pin);
        write(fd, buf, strlen(buf));
        close(fd);
        usleep(200000);
    }
}

void gpio_unexport(int pin) {
    if (!gpio_exists(pin)) {
        return;
    }

    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd >= 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", pin);
        write(fd, buf, strlen(buf));
        close(fd);
        usleep(100000);
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

void gpio_set_direction(int pin, const char* direction) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, direction, strlen(direction));
        close(fd);
    }
}

void spi_write_command(int spi_fd, uint8_t cmd) {
    gpio_write(GPIO_TFT_DATA_CONTROL, 0);
    struct spi_ioc_transfer transfer = {};
    transfer.tx_buf = (unsigned long)&cmd;
    transfer.len = 1;
    transfer.speed_hz = 32000000;
    transfer.bits_per_word = 8;
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
}

void spi_write_data(int spi_fd, const uint8_t* data, int len) {
    gpio_write(GPIO_TFT_DATA_CONTROL, 1);
    struct spi_ioc_transfer transfer = {};
    transfer.tx_buf = (unsigned long)data;
    transfer.len = len;
    transfer.speed_hz = 32000000;
    transfer.bits_per_word = 8;
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
}

void spi_write_data_u16(int spi_fd, uint16_t data) {
    uint8_t bytes[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
    spi_write_data(spi_fd, bytes, 2);
}

void display_error_screen(const char* error_msg) {
    log_message("Displaying error screen with full reset");

    int spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        log_message("ERROR: Cannot open SPI device for error screen");
        return;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 32000000;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // Note: Backlight is connected to VCC (always on)

    // Thorough hardware reset
    log_message("Performing hardware reset for error screen");
    gpio_write(GPIO_TFT_RESET_PIN, 1);
    usleep(10000);
    gpio_write(GPIO_TFT_RESET_PIN, 0);
    usleep(50000);  // Hold reset
    gpio_write(GPIO_TFT_RESET_PIN, 1);
    usleep(150000);

    // Software reset
    log_message("Performing software reset for error screen");
    spi_write_command(spi_fd, ST7789_SWRESET);
    usleep(200000);

    // Wake up display
    spi_write_command(spi_fd, ST7789_SLPOUT);
    usleep(120000);

    // Configure display
    spi_write_command(spi_fd, ST7789_MADCTL);
    uint8_t madctl = 0x60; // 90° rotation
    spi_write_data(spi_fd, &madctl, 1);

    spi_write_command(spi_fd, ST7789_COLMOD);
    uint8_t colmod = 0x55;
    spi_write_data(spi_fd, &colmod, 1);

    spi_write_command(spi_fd, ST7789_NORON);
    usleep(10000);

    spi_write_command(spi_fd, ST7789_INVON);
    usleep(10000);

    // Turn on display
    spi_write_command(spi_fd, ST7789_DISPON);
    usleep(120000);

    // Fill screen with red (error indicator)
    log_message("Filling error screen with red");
    spi_write_command(spi_fd, ST7789_CASET);
    spi_write_data_u16(spi_fd, 0);
    spi_write_data_u16(spi_fd, DISPLAY_WIDTH - 1);

    spi_write_command(spi_fd, ST7789_RASET);
    spi_write_data_u16(spi_fd, 0);
    spi_write_data_u16(spi_fd, DISPLAY_HEIGHT - 1);

    spi_write_command(spi_fd, ST7789_RAMWR);

    gpio_write(GPIO_TFT_DATA_CONTROL, 1);
    uint16_t red_pixel = COLOR_RED;
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        spi_write_data_u16(spi_fd, red_pixel);
    }

    close(spi_fd);
    log_message("Error screen displayed");
}

void reset_display_hardware() {
    log_message("Performing thorough hardware reset");

    // Note: Backlight is connected to VCC (always on)

    // Thorough reset sequence
    gpio_write(GPIO_TFT_RESET_PIN, 1);
    usleep(10000);
    gpio_write(GPIO_TFT_RESET_PIN, 0);
    usleep(50000);  // Hold reset longer
    gpio_write(GPIO_TFT_RESET_PIN, 1);
    usleep(200000); // Wait for reset to complete

    log_message("Hardware reset complete");
}

void cleanup_gpio() {
    log_message("Cleaning up GPIO");

    gpio_unexport(GPIO_TFT_DATA_CONTROL);
    gpio_unexport(GPIO_TFT_RESET_PIN);
}

bool check_spi_available() {
    int fd = open("/dev/spidev0.0", O_RDWR);
    if (fd < 0) {
        return false;
    }
    close(fd);
    return true;
}

void setup_gpio() {
    gpio_export(GPIO_TFT_DATA_CONTROL);
    gpio_export(GPIO_TFT_RESET_PIN);

    usleep(200000);

    gpio_set_direction(GPIO_TFT_DATA_CONTROL, "out");
    gpio_set_direction(GPIO_TFT_RESET_PIN, "out");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program_to_run> [args...]" << std::endl;
        std::cerr << "Example: " << argv[0] << " ./clock" << std::endl;
        return 1;
    }

    log_message("========== Failsafe Monitor Started ==========");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Check if SPI is available
    if (!check_spi_available()) {
        log_message("ERROR: SPI device not available. Enable SPI in raspi-config");
        return 1;
    }

    // Setup GPIO
    setup_gpio();

    int restart_count = 0;
    const int max_restarts = 10;
    const int restart_window = 60; // seconds
    time_t first_restart_time = 0;

    while (running) {
        log_message("Starting child process");

        child_pid = fork();

        if (child_pid == 0) {
            // Child process - execute the target program
            execvp(argv[1], &argv[1]);
            // If execvp returns, there was an error
            log_message("ERROR: Failed to execute program");
            exit(1);
        } else if (child_pid > 0) {
            // Parent process - monitor child
            int status;
            pid_t result = waitpid(child_pid, &status, 0);

            if (!running) {
                log_message("Shutdown requested");
                break;
            }

            if (result == -1) {
                log_message("ERROR: waitpid failed");
                break;
            }

            // Check exit status
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                char msg[128];
                snprintf(msg, sizeof(msg), "Child exited with code %d", exit_code);
                log_message(msg);

                if (exit_code == 0) {
                    log_message("Clean exit, shutting down");
                    break;
                }
            } else if (WIFSIGNALED(status)) {
                int signal = WTERMSIG(status);
                char msg[128];
                snprintf(msg, sizeof(msg), "Child killed by signal %d", signal);
                log_message(msg);
            }

            // Track restart rate
            time_t now = time(nullptr);
            if (first_restart_time == 0 || (now - first_restart_time) > restart_window) {
                first_restart_time = now;
                restart_count = 0;
            }

            restart_count++;

            if (restart_count > max_restarts) {
                log_message("ERROR: Too many restarts in short period. Giving up.");
                display_error_screen("Too many crashes");
                sleep(10);
                break;
            }

            log_message("Attempting recovery...");
            reset_display_hardware();
            sleep(2);

        } else {
            log_message("ERROR: fork failed");
            break;
        }
    }

    cleanup_gpio();
    log_message("========== Failsafe Monitor Stopped ==========");

    return 0;
}
