#pragma once

#include <stdint.h>

// =====================================================================
// エンコーダー仕様 (JGB520想定)
// =====================================================================
constexpr int32_t ENCODER_PPR = 11;   // 1相あたりのパルス数 / モーター軸1回転
constexpr int32_t QUAD_MULT = 4;      // 4逓倍
constexpr float GEAR_RATIO = 56.0f;   // 178rpmモデル
constexpr float COUNTS_PER_REV =
    static_cast<float>(ENCODER_PPR * QUAD_MULT) * GEAR_RATIO;  // 2464

constexpr int8_t ENCODER_DIR_SIGN = 1;  // 逆なら -1
constexpr int8_t MOTOR_DIR_SIGN = 1;    // 逆なら -1

// =====================================================================
// 速度制御
// =====================================================================
constexpr uint32_t CONTROL_PERIOD_MS = 50;
constexpr uint32_t TELEMETRY_PERIOD_MS = 100;
constexpr float RPM_FILTER_ALPHA = 0.35f;

constexpr float TARGET_RPM_MIN = 30.0f;
constexpr float TARGET_RPM_MAX = 60.0f;
constexpr uint32_t SPEED_SWEEP_PERIOD_MS = 20000;
constexpr float TARGET_SLEW_RPM_PER_S = 10.0f;

constexpr float KP = 0.008f;  // duty / RPM
constexpr float KI = 0.020f;  // duty / (RPM * s)
constexpr float FEEDFORWARD_DUTY_PER_RPM = 0.006f;
constexpr float INTEGRAL_LIMIT = 0.40f;
constexpr float DUTY_MIN = 0.0f;
constexpr float DUTY_MAX = 0.98f;
constexpr float DUTY_SLEW_PER_S = 1.5f;

// =====================================================================
// ストール検出
// =====================================================================
constexpr uint32_t STALL_GRACE_MS = 3000;
constexpr float STALL_DUTY_THRESHOLD = 0.90f;
constexpr float STALL_RPM_THRESHOLD = 2.0f;
constexpr uint32_t STALL_TIMEOUT_MS = 1000;

// =====================================================================
// モーターPWM (GP6/GP7, slice 3)
// =====================================================================
constexpr float MOTOR_PWM_CLKDIV = 1.0f;
constexpr uint16_t MOTOR_PWM_WRAP = 7499;  // 150MHz / 7500 = 20kHz

// =====================================================================
// MG90S (GP2, slice 1A, 1us tick / 50Hz)
// =====================================================================
constexpr uint16_t SERVO_MIN_US = 500;
constexpr uint16_t SERVO_MAX_US = 2500;
constexpr uint16_t SERVO_CENTER_US = 1500;
constexpr float SERVO_SWEEP_DEG = 30.0f;
constexpr float SERVO_US_PER_DEG =
    static_cast<float>(SERVO_MAX_US - SERVO_MIN_US) / 180.0f;
constexpr uint32_t SERVO_SWEEP_PERIOD_MS = 6000;
constexpr uint32_t SERVO_UPDATE_PERIOD_MS = 20;
constexpr uint32_t SERVO_START_DELAY_MS = 1000;
// 角度指令のスルーレート。小さいほど反力トルクが弱く、
// 仮置き台座などが倒れにくい。20deg/s で6秒周期スイープは
// 実質三角波(振幅±30度)になる。
constexpr float SERVO_SLEW_DEG_PER_S = 20.0f;

// =====================================================================
// I2C LCD (PCF8574, 16x2)
// =====================================================================
constexpr uint8_t LCD_COLS = 16;
constexpr uint32_t LCD_I2C_FREQ_HZ = 100000;
constexpr uint32_t LCD_I2C_TIMEOUT_US = 5000;
constexpr uint32_t LCD_UPDATE_PERIOD_MS = 500;
constexpr uint8_t LCD_MAX_FAILURES = 5;

// =====================================================================
// LED
// =====================================================================
constexpr uint32_t LED_BLINK_PERIOD_MS = 500;

constexpr float PI_F = 3.14159265358979323846f;
