#pragma once

#include <math.h>
#include <stdint.h>

#include "config.h"

// ホスト上でも単体テスト可能な純粋関数群。Arduino/Pico SDK非依存。

inline float countsToAngleDeg(int32_t count, float countsPerRev) {
    float angle = fmodf(static_cast<float>(count) * 360.0f / countsPerRev, 360.0f);
    if (angle < 0) {
        angle += 360.0f;
    }
    return angle;
}

// RPM = (差分カウント / 1回転あたりのカウント) * (60秒 / 経過秒)
inline float countsToRpm(int32_t deltaCount, float dtSec, float countsPerRev) {
    if (dtSec <= 0.0f || countsPerRev <= 0.0f) {
        return 0.0f;
    }
    return (static_cast<float>(deltaCount) / countsPerRev) * (60.0f / dtSec);
}

inline float defaultCountsToAngleDeg(int32_t count) {
    return countsToAngleDeg(count, COUNTS_PER_REV);
}

inline float defaultCountsToRpm(int32_t deltaCount, float dtSec) {
    return countsToRpm(deltaCount, dtSec, COUNTS_PER_REV);
}

// A/B相の直交デコード。直前/今回のAB状態 (2bit: A<<1|B) から
// 1ステップの増分 (-1/0/+1) を返す。
//
// インデックスは prev<<2 | curr の16通り。逆転やチャタリングなどの
// 不正な遷移は0 (カウントしない)。1電気周期 = 4カウント (4逓倍)。
inline int8_t quadratureDelta(uint8_t prev, uint8_t curr) {
    static const int8_t kQuadTable[16] = {
        0, -1, 1,  0,  // prev=00 -> curr: 00,01,10,11
        1, 0,  0,  -1, // prev=01 -> curr: 00,01,10,11
        -1, 0, 0,  1,  // prev=10 -> curr: 00,01,10,11
        0, 1,  -1, 0,  // prev=11 -> curr: 00,01,10,11
    };
    return kQuadTable[(static_cast<uint8_t>(prev) << 2) | (curr & 0x03)];
}
