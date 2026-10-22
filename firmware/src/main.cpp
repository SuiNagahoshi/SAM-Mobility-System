// Raspberry Pi Pico 2 / RP2350
// CH1 BD64950 DCモーター + JGB520エンコーダー + MG90S
//
// - 出力軸 30-60RPM をサイン波で自動スイープ
// - エンコーダーRPMによるPI速度制御
// - BD64950の電流制限は基板側VREF/RNFで固定
// - MG90Sは中心±30度を約6秒周期で往復
// - I2C LCD (PCF8574 16x2) に目標/実測RPM、duty、サーボ角度を表示
// - USB CDCにテレメトリを出力

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/gpio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "board_pins.h"
#include "config.h"
#include "encoder.h"
#include "encoder_math.h"
#include "lcd_pcf8574.h"
#include "motor.h"
#include "servo.h"

namespace {
uint64_t s_start_ms = 0;
uint64_t s_last_control_ms = 0;
uint64_t s_last_telemetry_ms = 0;
uint64_t s_last_servo_ms = 0;
uint64_t s_last_led_ms = 0;
uint64_t s_last_lcd_ms = 0;

bool s_lcd_present = false;
uint8_t s_lcd_failures = 0;

int32_t s_last_count = 0;
float s_rpm_filtered = 0.0f;
float s_target_rpm = 0.0f;
float s_integral = 0.0f;
float s_duty = 0.0f;

bool s_led_on = false;
bool s_stalled = false;
uint64_t s_stall_since_ms = 0;

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

float approach(float current, float target, float max_delta) {
    if (target > current) {
        const float next = current + max_delta;
        return next > target ? target : next;
    }
    const float next = current - max_delta;
    return next < target ? target : next;
}

void update_target(uint64_t now_ms, float dt_s) {
    const float t_s = static_cast<float>(now_ms - s_start_ms) / 1000.0f;
    const float period_s = static_cast<float>(SPEED_SWEEP_PERIOD_MS) / 1000.0f;
    const float phase = 2.0f * PI_F * t_s / period_s;

    const float mid = 0.5f * (TARGET_RPM_MIN + TARGET_RPM_MAX);
    const float half = 0.5f * (TARGET_RPM_MAX - TARGET_RPM_MIN);
    const float raw_target = mid - half * cosf(phase);

    s_target_rpm =
        approach(s_target_rpm, raw_target, TARGET_SLEW_RPM_PER_S * dt_s);
}

void update_control(uint64_t now_ms) {
    const uint64_t elapsed_ms = now_ms - s_last_control_ms;
    if (elapsed_ms < CONTROL_PERIOD_MS) {
        return;
    }
    s_last_control_ms = now_ms;

    const float dt_s = static_cast<float>(elapsed_ms) / 1000.0f;

    const int32_t count = encoder_read();
    const int32_t delta_count = count - s_last_count;
    s_last_count = count;

    const float rpm = countsToRpm(delta_count, dt_s, COUNTS_PER_REV);
    s_rpm_filtered += RPM_FILTER_ALPHA * (rpm - s_rpm_filtered);

    if (s_stalled) {
        s_target_rpm = 0.0f;
        s_duty = 0.0f;
        s_integral = 0.0f;
        motor_set_duty(0.0f);
        return;
    }

    update_target(now_ms, dt_s);

    const float error = s_target_rpm - s_rpm_filtered;
    const float feedforward = FEEDFORWARD_DUTY_PER_RPM * s_target_rpm;

    // 飽和時は積分を巻き上げない。
    const float duty_unsat = KP * error + s_integral + feedforward;
    const bool saturated_high = duty_unsat >= DUTY_MAX && error > 0.0f;
    const bool saturated_low = duty_unsat <= DUTY_MIN && error < 0.0f;
    if (!saturated_high && !saturated_low) {
        s_integral += KI * error * dt_s;
        s_integral = clampf(s_integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);
    }

    float duty_cmd = KP * error + s_integral + feedforward;
    duty_cmd = clampf(duty_cmd, DUTY_MIN, DUTY_MAX);
    s_duty = approach(s_duty, duty_cmd, DUTY_SLEW_PER_S * dt_s);
    s_duty = clampf(s_duty, DUTY_MIN, DUTY_MAX);

    if (s_target_rpm < 0.1f && fabsf(s_rpm_filtered) < 0.5f) {
        s_integral = 0.0f;
        s_duty = 0.0f;
    }

    // 起動直後の加速期間を除き、duty高止まりでRPMがほぼ0なら停止する。
    const bool past_grace = (now_ms - s_start_ms) >= STALL_GRACE_MS;
    if (past_grace && s_duty >= STALL_DUTY_THRESHOLD &&
        fabsf(s_rpm_filtered) < STALL_RPM_THRESHOLD) {
        if (s_stall_since_ms == 0) {
            s_stall_since_ms = now_ms;
        } else if (now_ms - s_stall_since_ms >= STALL_TIMEOUT_MS) {
            s_stalled = true;
        }
    } else {
        s_stall_since_ms = 0;
    }

    if (s_stalled) {
        s_duty = 0.0f;
        s_integral = 0.0f;
        s_target_rpm = 0.0f;
    }

    motor_set_duty(s_duty);
}

void update_servo(uint64_t now_ms) {
    if (now_ms - s_last_servo_ms < SERVO_UPDATE_PERIOD_MS) {
        return;
    }
    s_last_servo_ms = now_ms;
    servo_update(now_ms);
}

void update_led(uint64_t now_ms) {
    if (now_ms - s_last_led_ms < LED_BLINK_PERIOD_MS) {
        return;
    }
    s_last_led_ms = now_ms;
    s_led_on = !s_led_on;
    gpio_put(PIN_STATUS_LED, s_led_on);
}

void update_lcd(uint64_t now_ms) {
    if (!s_lcd_present) {
        return;
    }
    if (now_ms - s_last_lcd_ms < LCD_UPDATE_PERIOD_MS) {
        return;
    }
    s_last_lcd_ms = now_ms;

    char line0[LCD_COLS + 1];
    char line1[LCD_COLS + 1];

    snprintf(line0, sizeof(line0), "SV:%+4.1f %4uus",
             servo_last_angle_deg(), servo_last_pulse_us());

    if (s_stalled) {
        snprintf(line1, sizeof(line1), "STALL A%3.0f D%2.0f%%",
                 s_rpm_filtered, s_duty * 100.0f);
    } else {
        snprintf(line1, sizeof(line1), "DC T%2.0f A%3.0f %2.0f%%",
                 s_target_rpm, s_rpm_filtered, s_duty * 100.0f);
    }

    if (!lcd_print_line(0, line0) || !lcd_print_line(1, line1)) {
        if (++s_lcd_failures >= LCD_MAX_FAILURES) {
            s_lcd_present = false;
        }
    } else {
        s_lcd_failures = 0;
    }
}

void update_telemetry(uint64_t now_ms) {
    if (now_ms - s_last_telemetry_ms < TELEMETRY_PERIOD_MS) {
        return;
    }
    s_last_telemetry_ms = now_ms;

    if (!stdio_usb_connected()) {
        return;
    }

    printf("target=%5.1f rpm=%6.1f duty=%.2f count=%ld servo=%6.1f%s\n",
           s_target_rpm, s_rpm_filtered, s_duty,
           static_cast<long>(encoder_read()), servo_last_angle_deg(),
           s_stalled ? " STALL" : "");
}
}  // namespace

int main() {
    stdio_init_all();

    // USB CDC接続を最大3秒待つ (未接続でも運転は開始する)。
    const uint64_t boot_ms = to_ms_since_boot(get_absolute_time());
    while (!stdio_usb_connected() &&
           to_ms_since_boot(get_absolute_time()) - boot_ms < 3000) {
        sleep_ms(10);
    }

    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);
    gpio_put(PIN_STATUS_LED, 0);

    s_lcd_present = lcd_init();
    if (s_lcd_present) {
        lcd_print_line(0, "Pico2 Rover CH1");
        lcd_print_line(1, "Starting...");
    }

    encoder_init();
    motor_init();
    servo_init();

    s_start_ms = to_ms_since_boot(get_absolute_time());
    s_last_control_ms = s_start_ms;
    s_last_telemetry_ms = s_start_ms;
    s_last_servo_ms = s_start_ms;
    s_last_led_ms = s_start_ms;
    s_last_lcd_ms = s_start_ms;
    s_last_count = encoder_read();

    if (stdio_usb_connected()) {
        printf("Pico2 rover mobility: CH1 BD64950 + MG90S\n");
        printf("encoder %.0f counts/rev, target %.0f-%.0f rpm, current limit is hardware-fixed\n",
               COUNTS_PER_REV, TARGET_RPM_MIN, TARGET_RPM_MAX);
        if (s_lcd_present) {
            printf("LCD found at 0x%02X\n", lcd_addr());
        } else {
            printf("LCD not found on 0x27 / 0x3F\n");
        }
    }

    while (true) {
        const uint64_t now_ms = to_ms_since_boot(get_absolute_time());
        update_control(now_ms);
        update_servo(now_ms);
        update_led(now_ms);
        update_lcd(now_ms);
        update_telemetry(now_ms);
        tight_loop_contents();
    }
}
