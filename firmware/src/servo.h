#pragma once

#include <stdint.h>

// MG90S 制御API (50Hz, 500-2500us)。
void servo_init();

// パルス幅を直接設定する (500-2500usにクランプ)。
void servo_set_pulse_us(uint16_t pulse_us);

// 起動ディレイ後、±SERVO_SWEEP_DEG をサイン波で往復させる。
// 角度指令は SERVO_SLEW_DEG_PER_S のスルーリミットで緩やかに追従する。
// now_ms は to_ms_since_boot(get_absolute_time()) の値。
void servo_update(uint64_t now_ms);

// テレメトリ用。直近に指令した角度 [deg] とパルス幅 [us]。
float servo_last_angle_deg();
uint16_t servo_last_pulse_us();
