// Failsafe wrapper for ST7789 Display
// Monitors and recovers from display errors
// Using ST7789_TFT_RPI driver architecture with bcm2835 library

#include <bcm2835.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <fstream>

// GPIO Pin Configuration (Software SPI - matches ST7789_TFT_RPI SW SPI setup)
#define TFT_CS_GPIO    RPI_BPLUS_GPIO_J8_32  // GPIO12 - CS/SS pin
#define TFT_DC_GPIO    RPI_BPLUS_GPIO_J8_18  // GPIO24 - DC pin
#define TFT_RST_GPIO   RPI_BPLUS_GPIO_J8_22  // GPIO25 - RESET pin
#define TFT_SDATA_GPIO RPI_BPLUS_GPIO_J8_35  // GPIO19 - MOSI/SDA pin
#define TFT_SCLK_GPIO  RPI_BPLUS_GPIO_J8_37  // GPIO26 - SCLK pin
// Note: LED/Backlight connected to VCC (always on, no GPIO control needed)

#define TFT_HIGHFREQ_DELAY 0  // Microseconds delay between bit operations

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
uint16_t _highFreqDelay = TFT_HIGHFREQ_DELAY;

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

void spi_write_command(uint8_t cmd) {
    bcm2835_gpio_write(TFT_DC_GPIO, LOW);  // Command mode
    bcm2835_gpio_write(TFT_CS_GPIO, LOW);  // CS low (select)
    spiWriteByte(cmd);
    bcm2835_gpio_write(TFT_CS_GPIO, HIGH); // CS high (deselect)
}

void spi_write_data(const uint8_t* data, int len) {
    bcm2835_gpio_write(TFT_DC_GPIO, HIGH); // Data mode
    bcm2835_gpio_write(TFT_CS_GPIO, LOW);  // CS low (select)
    for (int i = 0; i < len; i++) {
        spiWriteByte(data[i]);
    }
    bcm2835_gpio_write(TFT_CS_GPIO, HIGH); // CS high (deselect)
}

void spi_write_data_u16(uint16_t data) {
    uint8_t bytes[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
    spi_write_data(bytes, 2);
}

void display_error_screen(const char* error_msg) {
    log_message("Displaying error screen with full reset");

    // Note: Backlight is connected to VCC (always on)

    // Thorough hardware reset
    log_message("Performing hardware reset for error screen");
    bcm2835_gpio_write(TFT_RST_GPIO, HIGH);
    bcm2835_delay(10);
    bcm2835_gpio_write(TFT_RST_GPIO, LOW);
    bcm2835_delay(50);
    bcm2835_gpio_write(TFT_RST_GPIO, HIGH);
    bcm2835_delay(150);

    // Software reset
    log_message("Performing software reset for error screen");
    spi_write_command(ST7789_SWRESET);
    bcm2835_delay(200);

    // Wake up display
    spi_write_command(ST7789_SLPOUT);
    bcm2835_delay(120);

    // Configure display
    spi_write_command(ST7789_MADCTL);
    uint8_t madctl = 0x60; // 90° rotation
    spi_write_data(&madctl, 1);

    spi_write_command(ST7789_COLMOD);
    uint8_t colmod = 0x55;
    spi_write_data(&colmod, 1);

    spi_write_command(ST7789_NORON);
    bcm2835_delay(10);

    spi_write_command(ST7789_INVON);
    bcm2835_delay(10);

    // Turn on display
    spi_write_command(ST7789_DISPON);
    bcm2835_delay(120);

    // Fill screen with red (error indicator)
    log_message("Filling error screen with red");
    spi_write_command(ST7789_CASET);
    spi_write_data_u16(0);
    spi_write_data_u16(DISPLAY_WIDTH - 1);

    spi_write_command(ST7789_RASET);
    spi_write_data_u16(0);
    spi_write_data_u16(DISPLAY_HEIGHT - 1);

    spi_write_command(ST7789_RAMWR);

    uint16_t red_pixel = COLOR_RED;
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        spi_write_data_u16(red_pixel);
    }

    log_message("Error screen displayed");
}

void reset_display_hardware() {
    log_message("Performing thorough hardware reset");

    // Note: Backlight is connected to VCC (always on)

    // Thorough reset sequence
    bcm2835_gpio_write(TFT_RST_GPIO, HIGH);
    bcm2835_delay(10);
    bcm2835_gpio_write(TFT_RST_GPIO, LOW);
    bcm2835_delay(50);
    bcm2835_gpio_write(TFT_RST_GPIO, HIGH);
    bcm2835_delay(200);

    log_message("Hardware reset complete");
}

void cleanup_gpio() {
    log_message("Cleaning up GPIO");
    bcm2835_close();
}

bool check_bcm2835_available() {
    // Try to initialize bcm2835 library
    if (!bcm2835_init()) {
        return false;
    }
    return true;
}

void setup_gpio() {
    log_message("Setting up GPIO pins for Software SPI");

    bcm2835_gpio_fsel(TFT_CS_GPIO, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TFT_DC_GPIO, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TFT_RST_GPIO, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TFT_SDATA_GPIO, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TFT_SCLK_GPIO, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_write(TFT_CS_GPIO, HIGH);
    bcm2835_gpio_write(TFT_SCLK_GPIO, LOW);
    bcm2835_gpio_write(TFT_SDATA_GPIO, LOW);

    log_message("GPIO setup complete");
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

    // Check if bcm2835 library is available
    if (!check_bcm2835_available()) {
        log_message("ERROR: bcm2835 library initialization failed");
        log_message("Are you running as root? Try: sudo ./failsafe");
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
