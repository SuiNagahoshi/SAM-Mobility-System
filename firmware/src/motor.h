#pragma once

// BD64950 CH1 モーター制御API。
void motor_init();

// duty: -1.0 (逆転最大) .. +1.0 (正転最大), 0.0 = STANDBY (両入力L)
void motor_set_duty(float duty);

void motor_stop();
