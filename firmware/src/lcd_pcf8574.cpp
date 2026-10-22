#include "lcd_pcf8574.h"

#include "board_pins.h"
#include "config.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

namespace {
constexpr uint8_t LCD_RS = 0x01;
constexpr uint8_t LCD_EN = 0x04;
constexpr uint8_t LCD_BL = 0x08;
constexpr uint8_t LCD_ADDR_CANDIDATES[] = {0x27, 0x3F};

uint8_t s_addr = 0;
bool s_present = false;

bool lcd_write_raw(uint8_t data) {
    const uint8_t value = static_cast<uint8_t>(data | LCD_BL);
    const int ret = i2c_write_timeout_us(i2c0, s_addr, &value, 1, false,
                                         LCD_I2C_TIMEOUT_US);
    return ret == 1;
}

bool lcd_pulse(uint8_t data) {
    if (!lcd_write_raw(static_cast<uint8_t>(data | LCD_EN))) {
        return false;
    }
    sleep_us(2);
    if (!lcd_write_raw(static_cast<uint8_t>(data & ~LCD_EN))) {
        return false;
    }
    sleep_us(60);
    return true;
}

bool lcd_send(uint8_t value, bool rs) {
    uint8_t high = static_cast<uint8_t>(value & 0xF0);
    uint8_t low = static_cast<uint8_t>((value << 4) & 0xF0);
    if (rs) {
        high |= LCD_RS;
        low |= LCD_RS;
    }
    return lcd_pulse(high) && lcd_pulse(low);
}

bool lcd_command(uint8_t cmd) {
    const bool ok = lcd_send(cmd, false);
    if (ok && (cmd == 0x01 || cmd == 0x02)) {
        sleep_ms(3);
    }
    return ok;
}

bool lcd_set_cursor(uint8_t col, uint8_t row) {
    static const uint8_t row_offsets[] = {0x00, 0x40};
    if (row > 1) {
        row = 1;
    }
    if (col >= LCD_COLS) {
        col = LCD_COLS - 1;
    }
    return lcd_command(static_cast<uint8_t>(0x80 | (row_offsets[row] + col)));
}

bool lcd_detect() {
    const uint8_t zero = 0x00;
    for (uint8_t addr : LCD_ADDR_CANDIDATES) {
        if (i2c_write_timeout_us(i2c0, addr, &zero, 1, false,
                                 LCD_I2C_TIMEOUT_US) == 1) {
            s_addr = addr;
            return true;
        }
    }
    return false;
}
}  // namespace

bool lcd_init() {
    i2c_init(i2c0, LCD_I2C_FREQ_HZ);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);

    s_present = false;
    s_addr = 0;
    if (!lcd_detect()) {
        return false;
    }

    s_present = true;
    sleep_ms(50);
    bool ok = lcd_pulse(0x30);
    sleep_ms(5);
    ok = ok && lcd_pulse(0x30);
    sleep_ms(1);
    ok = ok && lcd_pulse(0x30);
    sleep_ms(1);
    ok = ok && lcd_pulse(0x20);
    sleep_ms(1);
    ok = ok && lcd_command(0x28);
    ok = ok && lcd_command(0x0C);
    ok = ok && lcd_command(0x01);
    ok = ok && lcd_command(0x06);

    if (!ok) {
        s_present = false;
        return false;
    }
    return true;
}

uint8_t lcd_addr() { return s_addr; }

bool lcd_print_line(uint8_t row, const char *text) {
    if (!s_present) {
        return false;
    }
    if (!lcd_set_cursor(0, row)) {
        return false;
    }

    bool end = false;
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        char c = ' ';
        if (!end && text[i] != '\0') {
            c = text[i];
        } else {
            end = true;
        }
        if (!lcd_send(static_cast<uint8_t>(c), true)) {
            return false;
        }
    }
    return true;
}
