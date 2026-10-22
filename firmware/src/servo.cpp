// MG90S 制御。
// 1MHz tick / 20ms period (50Hz) で 500-2500us を出力する。
// 角度指令は SERVO_SLEW_DEG_PER_S でスルーリミットし、
// 原点回帰・スイープ時の反力トルク(モーメント)を抑える。

#include "servo.h"

#include "board_pins.h"
#include "config.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include <math.h>

namespace {
float s_cmd_angle_deg = 0.0f;
uint16_t s_last_pulse_us = SERVO_CENTER_US;
uint64_t s_last_update_ms = 0;

// スルーリミット計算で使うdtの上限。長時間ブロック後の初回更新で
// 指令が跳ねるのを防ぐ。
constexpr uint64_t SERVO_SLEW_DT_CAP_MS = 100;

uint16_t clamp_us(float us) {
    if (us < static_cast<float>(SERVO_MIN_US)) {
        return SERVO_MIN_US;
    }
    if (us > static_cast<float>(SERVO_MAX_US)) {
        return SERVO_MAX_US;
    }
    return static_cast<uint16_t>(us + 0.5f);
}

void apply_angle(float deg) {
    s_cmd_angle_deg = deg;
    const float pulse_us =
        static_cast<float>(SERVO_CENTER_US) + deg * SERVO_US_PER_DEG;
    servo_set_pulse_us(clamp_us(pulse_us));
}

float slew_toward(float current, float target, float max_step) {
    const float delta = target - current;
    if (delta > max_step) {
        return current + max_step;
    }
    if (delta < -max_step) {
        return current - max_step;
    }
    return target;
}
}  // namespace

void servo_init() {
    gpio_set_function(PIN_SERVO, GPIO_FUNC_PWM);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 150.0f);  // 150MHz / 150 = 1MHz
    pwm_config_set_wrap(&cfg, 19999);     // 20000us = 50Hz
    pwm_init(pwm_gpio_to_slice_num(PIN_SERVO), &cfg, true);

    // 初回パルスは中心。以降の指令変化はスルーリミットで緩やかに追従する。
    s_cmd_angle_deg = 0.0f;
    s_last_update_ms = to_ms_since_boot(get_absolute_time());
    apply_angle(0.0f);
}

void servo_set_pulse_us(uint16_t pulse_us) {
    s_last_pulse_us = clamp_us(static_cast<float>(pulse_us));
    pwm_set_gpio_level(PIN_SERVO, s_last_pulse_us);
}

void servo_update(uint64_t now_ms) {
    float target_deg = 0.0f;
    if (now_ms >= SERVO_START_DELAY_MS) {
        const float t_s =
            static_cast<float>(now_ms - SERVO_START_DELAY_MS) / 1000.0f;
        const float period_s =
            static_cast<float>(SERVO_SWEEP_PERIOD_MS) / 1000.0f;
        const float phase = 2.0f * PI_F * t_s / period_s;
        target_deg = SERVO_SWEEP_DEG * sinf(phase);
    }

    uint64_t elapsed_ms = now_ms - s_last_update_ms;
    if (elapsed_ms > SERVO_SLEW_DT_CAP_MS) {
        elapsed_ms = SERVO_SLEW_DT_CAP_MS;
    }
    s_last_update_ms = now_ms;

    const float max_step_deg =
        SERVO_SLEW_DEG_PER_S * static_cast<float>(elapsed_ms) / 1000.0f;
    apply_angle(slew_toward(s_cmd_angle_deg, target_deg, max_step_deg));
}

float servo_last_angle_deg() {
    return s_cmd_angle_deg;
}

uint16_t servo_last_pulse_us() {
    return s_last_pulse_us;
}
