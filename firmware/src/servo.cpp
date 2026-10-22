// MG90S 制御。
// 1MHz tick / 20ms period (50Hz) で 500-2500us を出力する。

#include "servo.h"

#include "board_pins.h"
#include "config.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include <math.h>

namespace {
float s_last_angle_deg = 0.0f;
uint16_t s_last_pulse_us = SERVO_CENTER_US;

uint16_t clamp_us(float us) {
    if (us < static_cast<float>(SERVO_MIN_US)) {
        return SERVO_MIN_US;
    }
    if (us > static_cast<float>(SERVO_MAX_US)) {
        return SERVO_MAX_US;
    }
    return static_cast<uint16_t>(us + 0.5f);
}
}  // namespace

void servo_init() {
    gpio_set_function(PIN_SERVO, GPIO_FUNC_PWM);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 150.0f);  // 150MHz / 150 = 1MHz
    pwm_config_set_wrap(&cfg, 19999);     // 20000us = 50Hz
    pwm_init(pwm_gpio_to_slice_num(PIN_SERVO), &cfg, true);

    s_last_angle_deg = 0.0f;
    servo_set_pulse_us(SERVO_CENTER_US);
}

void servo_set_pulse_us(uint16_t pulse_us) {
    s_last_pulse_us = clamp_us(static_cast<float>(pulse_us));
    pwm_set_gpio_level(PIN_SERVO, s_last_pulse_us);
}

void servo_update(uint64_t now_ms) {
    if (now_ms < SERVO_START_DELAY_MS) {
        s_last_angle_deg = 0.0f;
        servo_set_pulse_us(SERVO_CENTER_US);
        return;
    }

    const float t_s =
        static_cast<float>(now_ms - SERVO_START_DELAY_MS) / 1000.0f;
    const float period_s = static_cast<float>(SERVO_SWEEP_PERIOD_MS) / 1000.0f;
    const float phase = 2.0f * PI_F * t_s / period_s;

    s_last_angle_deg = SERVO_SWEEP_DEG * sinf(phase);

    const float pulse_us =
        static_cast<float>(SERVO_CENTER_US) +
        s_last_angle_deg * SERVO_US_PER_DEG;
    servo_set_pulse_us(clamp_us(pulse_us));
}

float servo_last_angle_deg() {
    return s_last_angle_deg;
}

uint16_t servo_last_pulse_us() {
    return s_last_pulse_us;
}
