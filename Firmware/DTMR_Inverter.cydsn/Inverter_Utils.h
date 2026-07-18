/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#ifndef INVERTER_UTILS_H
#define INVERTER_UTILS_H

#include "stdint.h"
#include "stdbool.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

#define TEST_AMPLITUDE_TORQUE           12.0f       // Iq
#define TEST_AMPLITUDE_SPEED            1500.0f     // RPM
#define TEST_AMPLITUDE_POSITION         1080.0f     // Degrees
#define TEST_INTERVAL                   1000u      // 1000 ms = 1s Se
#define TEST_CYCLES                     1u

float fInverterUtils_TestSeqTorqueIq(uint64_t ui64Tick, float fKnobInput);

float fInverterUtils_TestSeqSpeedRpm(uint64_t ui64Tick, float fKnobInput);

float fInverterUtils_TestSeqPositionDeg(uint64_t ui64Tick, float fKnobInput);

bool  bInverterUtils_TestSeqIsRunning(void);

#endif /* INVERTER_UTILS_H */

/* [] END OF FILE */
