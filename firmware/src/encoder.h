#pragma once

#include <stdint.h>

// エンコーダーAPI。
// encoder_init() より前にGPIO設定を行う必要はない (内部で行う)。
void encoder_init();

// ENCODER_DIR_SIGN適用済みの累積カウント (4逓倍) を返す。
int32_t encoder_read();
