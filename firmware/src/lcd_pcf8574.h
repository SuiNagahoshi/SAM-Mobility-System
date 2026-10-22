#pragma once

#include <stdint.h>

// PCF8574経由 16x2 LCD (4bitモード) の最小ドライバ。
// I2C0 (GP4/5) の初期化、LCD検出、LCD初期化まで行う。

bool lcd_init();
uint8_t lcd_addr();
bool lcd_print_line(uint8_t row, const char *text);
