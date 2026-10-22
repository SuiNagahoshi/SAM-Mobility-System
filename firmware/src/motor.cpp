// BD64950 直接PWM制御。
// データシート推奨の「PWM/L」駆動:
//   正転: IN1=PWM, IN2=L
//   逆転: IN1=L,   IN2=PWM
//   停止: IN1=L,   IN2=L (STANDBY)
// 電流制限は基板のVREF/RNFで固定。ファームからは変更しない。

#include "motor.h"

#include "board_pins.h"
#include "config.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

namespace {
uint s_slice = 0;
uint s_chan_a = 0;
uint s_chan_b = 0;
uint16_t s_wrap = 0;

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
}  // namespace

void motor_init() {
    gpio_set_function(PIN_MOTOR_IN1, GPIO_FUNC_PWM);
    gpio_set_function(PIN_MOTOR_IN2, GPIO_FUNC_PWM);

    s_slice = pwm_gpio_to_slice_num(PIN_MOTOR_IN1);
    s_chan_a = pwm_gpio_to_channel(PIN_MOTOR_IN1);
    s_chan_b = pwm_gpio_to_channel(PIN_MOTOR_IN2);
    s_wrap = MOTOR_PWM_WRAP;

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, MOTOR_PWM_CLKDIV);
    pwm_config_set_wrap(&cfg, s_wrap);
    pwm_init(s_slice, &cfg, true);

    motor_stop();
}

void motor_set_duty(float duty) {
    duty = clampf(duty, -1.0f, 1.0f) * static_cast<float>(MOTOR_DIR_SIGN);

    const float magnitude = duty < 0.0f ? -duty : duty;
    uint32_t level = static_cast<uint32_t>(magnitude * (s_wrap + 1) + 0.5f);
    if (level > s_wrap) {
        level = s_wrap;
    }

    if (duty > 0.0f) {
        // 先に反対側をLにしてからPWMを入れる (方向切替時のブレーキを最小化)。
        pwm_set_chan_level(s_slice, s_chan_b, 0);
        pwm_set_chan_level(s_slice, s_chan_a, static_cast<uint16_t>(level));
    } else if (duty < 0.0f) {
        pwm_set_chan_level(s_slice, s_chan_a, 0);
        pwm_set_chan_level(s_slice, s_chan_b, static_cast<uint16_t>(level));
    } else {
        motor_stop();
    }
}

void motor_stop() {
    if (s_wrap == 0) {
        // motor_init() 前の呼び出しではGPIOを直接Lにする。
        gpio_init(PIN_MOTOR_IN1);
        gpio_init(PIN_MOTOR_IN2);
        gpio_set_dir(PIN_MOTOR_IN1, GPIO_OUT);
        gpio_set_dir(PIN_MOTOR_IN2, GPIO_OUT);
        gpio_put(PIN_MOTOR_IN1, 0);
        gpio_put(PIN_MOTOR_IN2, 0);
        return;
    }
    pwm_set_chan_level(s_slice, s_chan_a, 0);
    pwm_set_chan_level(s_slice, s_chan_b, 0);
}
