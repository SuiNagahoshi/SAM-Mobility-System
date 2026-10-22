#pragma once

#include <stdint.h>

// =====================================================================
// Raspberry Pi Pico 2 (RP2350) ピン定義
//
// 選択方針:
// - GP4/5 : I2C0 (PCF8574 LCD)
// - GP8-15: SPI1用として温存
// - GP16-19: SPI0 (W5500) 用として温存
// - GP23/24/25 のうち GP25 は内蔵LED
// =====================================================================
constexpr uint8_t PIN_ENC_A = 20;       // CH1_Phase_A
constexpr uint8_t PIN_ENC_B = 21;       // CH1_Phase_B
constexpr uint8_t PIN_MOTOR_IN1 = 6;    // CH1_IN1 (BD64950 pin3)
constexpr uint8_t PIN_MOTOR_IN2 = 7;    // CH1_IN2 (BD64950 pin2)
constexpr uint8_t PIN_SERVO = 2;        // MG90S signal
constexpr uint8_t PIN_I2C_SDA = 4;      // PCF8574 SDA (I2C0)
constexpr uint8_t PIN_I2C_SCL = 5;      // PCF8574 SCL (I2C0)
constexpr uint8_t PIN_STATUS_LED = 25;  // Pico 2 onboard LED
