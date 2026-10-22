// Raspberry Pi Pico 2 (RP2350) 用エンコーダー実装。
// 両相CHANGE割り込みによる4逓倍。

#include "encoder.h"

#include "board_pins.h"
#include "config.h"
#include "encoder_math.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

namespace {
volatile int32_t s_count = 0;
volatile uint8_t s_prev = 0;

inline uint8_t read_ab() {
    return static_cast<uint8_t>((gpio_get(PIN_ENC_A) ? 2u : 0u) |
                                (gpio_get(PIN_ENC_B) ? 1u : 0u));
}

void encoder_irq(uint gpio, uint32_t events) {
    (void)gpio;
    (void)events;
    const uint8_t curr = read_ab();
    s_count += quadratureDelta(s_prev, curr);
    s_prev = curr;
}
}  // namespace

void encoder_init() {
    gpio_init(PIN_ENC_A);
    gpio_init(PIN_ENC_B);
    gpio_set_dir(PIN_ENC_A, GPIO_IN);
    gpio_set_dir(PIN_ENC_B, GPIO_IN);
    gpio_pull_up(PIN_ENC_A);
    gpio_pull_up(PIN_ENC_B);

    s_count = 0;
    s_prev = read_ab();

    gpio_set_irq_enabled_with_callback(
        PIN_ENC_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &encoder_irq);
    gpio_set_irq_enabled(
        PIN_ENC_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}

int32_t encoder_read() {
    const uint32_t status = save_and_disable_interrupts();
    const int32_t raw = s_count;
    restore_interrupts(status);
    return raw * ENCODER_DIR_SIGN;
}
