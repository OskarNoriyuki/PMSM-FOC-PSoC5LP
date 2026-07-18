/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 * Copyright (c) 2026 sirojudin munir
 *
 * Minimal FOC implementation, based on siroju's FOC_math library.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#ifndef FOC_INC_FOC_H_
#define FOC_INC_FOC_H_

#include <stdint.h>
#include "PID.h"

#define LUT_SIZE (1024*4)
#define LUT_STEP (TWO_PI / (float)LUT_SIZE)

#define TWO_BY_SQRT3 1.15470053838f
#define ONE_BY_SQRT3 0.57735026919f
#define SQRT3_BY_TWO 0.86602540378f
#define SQRT3 1.73205080757f
#define TWO_PI 6.2831853f
#define PI 3.1415926f

// current loop
#define FOC_PI_TS        (1.0f / 4000.0f)
#define FOC_PI_KP        0.2f
#define FOC_PI_KI        800.0f
#define FOC_PI_VMAX      5.0f // voltage
// #define FOC_IQ_MAX_A     8.0f // safe current for torque mode
#define FOC_IQ_MAX_A     12.0f

// speed loop
#define SPD_PI_TS        (1.0f / 1000.0f)
#define SPD_PI_KP        0.008f
#define SPD_PI_KI        0.150f
#define SPD_PI_KD        0.00002f
#define SPD_D_FC_HZ      30.0f
#define SPD_D_MAX        1000000.0f
#define SPD_KNOB_RANGE_RPM 1000.0f

// position loop
/*
#define POS_PD_TS        (1.0f / 1000.0f)
#define POS_PD_KP        0.20f
#define POS_PD_KI        1.0f
#define POS_PD_KD        0.002f
#define POS_D_FC_HZ      30.0f
#define POS_D_MAX        50000.0f
#define POS_DEADBAND_DEG 0.2f
*/

#define POS_PD_TS        (1.0f / 1000.0f)
#define POS_PD_KP        0.75f
#define POS_PD_KI        16.0f
#define POS_PD_KD        0.002f
#define POS_D_FC_HZ      40.0f
#define POS_D_MAX        50000.0f
#define POS_DEADBAND_DEG 0.2f
#define POS_KNOB_RANGE_DEG 360.0f

#define CONSTRAIN(val, min, max) \
    ((val) <= (min) ? (min) : ((val) >= (max) ? (max) : (val)))

#define RAD_TO_DEG(rad) ((rad) * 57.29577951308232f)  // 180/π

#define DEG_TO_RAD(deg) ((deg) * 0.017453292519943f)  // π/180

/* Low-Side -> Inline current sensing compatibility */
#define A1339_OFFSET_ELECTRICAL_60DEG (171)

typedef struct {
    float fK;
    float fY;
} xFOC_IirLpf;

// FOC state: sensing, cascaded loops and modulator outputs
typedef struct {
    // phase currents [A]
    float fIu;
    float fIv;
    float fIw;
    // stationary (alpha-beta) and rotor (d-q) frame currents
    float fIalpha;
    float fIbeta;
    float fId;
    float fIq;
    // rotor electrical angle: voltage frame and current-sense frame (-60 deg)
    float fSin;
    float fCos;
    float fSinM60;
    float fCosM60;
    // current loop
    float fIqRef;
    xPID_Controller xPiId;
    xPID_Controller xPiIq;
    float fVd;
    float fVq;
    float fVmag;
    float fValpha;
    float fVbeta;
    // SVPWM outputs [counts]
    uint32_t ui32PwmU;
    uint32_t ui32PwmV;
    uint32_t ui32PwmW;
    // speed loop
    float fSpeedTargetRpm;
    float fSpeedMeasRpm;
    float fSpeedIqRef;
    xPID_Controller xPiSpeed;
    // position loop
    float fPosTargetDeg;
    float fPosMeasDeg;
    float fPosIqRef;
    xPID_Controller xPdPos;
} xFOC_Ctrl;

void  vFOC_InitTrigLut(void);
float fFOC_FastSin(float fTheta);
float fFOC_FastCos(float fTheta);
float fFOC_FastSqrt(float fX);
void  vFOC_RotorRawToSinCos(uint16_t ui16MechRaw, float *pfSinTheta, float *pfCosTheta);
void  vFOC_RotorRawToSinCosM60(uint16_t ui16MechRaw, float *pfSinTheta, float *pfCosTheta);
float fFOC_IirLpfUpdate(xFOC_IirLpf *pxF, float fX);

void  vFOC_ClarkeTransform(float fIa, float fIb, float *pfIalpha, float *pfIbeta);
void  vFOC_ParkTransform(float fIalpha, float fIbeta, float fSinTheta, float fCosTheta, float *pfId, float *pfIq);
void  vFOC_InverseParkTransform(float fVd, float fVq, float fSinTheta, float fCosTheta, float *pfValpha, float *pfVbeta);
void  vFOC_Svpwm(float fValpha, float fVbeta, float fVbus, uint32_t ui32PwmPeriod,
                 uint32_t *pui32PwmU, uint32_t *pui32PwmV, uint32_t *pui32PwmW);

#endif

/* [] END OF FILE */
