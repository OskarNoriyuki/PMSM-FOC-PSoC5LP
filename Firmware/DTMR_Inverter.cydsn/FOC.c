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

#include "FOC.h"
#include "math.h"

static float fSinLut[LUT_SIZE];
static float fCosLut[LUT_SIZE];

/******************************************************************************
* @brief       Fills the sin/cos lookup tables (call once at startup)
******************************************************************************/
void vFOC_InitTrigLut(void) {
    for (int i = 0; i < LUT_SIZE; ++i) {
        float fAngle = i * LUT_STEP;
        fSinLut[i] = sinf(fAngle);
        fCosLut[i] = cosf(fAngle);
    }
}

/******************************************************************************
* @brief       Wraps an angle into [0, TWO_PI)
* @param[out]  pfTheta  Angle [rad], normalized in place
******************************************************************************/
static void vFOC_NormAngleRad(float *pfTheta) {
    while (*pfTheta < 0) *pfTheta += TWO_PI;
    while (*pfTheta >= TWO_PI) *pfTheta -= TWO_PI;
}

/******************************************************************************
* @brief       LUT-based sine with linear interpolation
* @param[in]   fTheta  Angle [rad], any range
* @return      float   sin(fTheta)
******************************************************************************/
float fFOC_FastSin(float fTheta) {
    vFOC_NormAngleRad(&fTheta);
    float fIndexF = fTheta / LUT_STEP;
    int iIndex = (int)fIndexF;
    float fFrac = fIndexF - iIndex;

    int iNextIndex = (iIndex + 1) % LUT_SIZE;

    return fSinLut[iIndex] * (1.0f - fFrac) + fSinLut[iNextIndex] * fFrac;
}

/******************************************************************************
* @brief       LUT-based cosine with linear interpolation
* @param[in]   fTheta  Angle [rad], any range
* @return      float   cos(fTheta)
******************************************************************************/
float fFOC_FastCos(float fTheta) {
    vFOC_NormAngleRad(&fTheta);
    float fIndexF = fTheta / LUT_STEP;
    int iIndex = (int)fIndexF;
    float fFrac = fIndexF - iIndex;

    int iNextIndex = (iIndex + 1) % LUT_SIZE;

    return fCosLut[iIndex] * (1.0f - fFrac) + fCosLut[iNextIndex] * fFrac;
}

/******************************************************************************
* @brief       Newton-Raphson square root (5 iterations, no FPU/sqrtf)
* @param[in]   fX     Operand; values <= 0 return 0
* @return      float  sqrt(fX)
******************************************************************************/
float fFOC_FastSqrt(float fX) {
    if (fX <= 0.0f) return 0.0f;

    float fGuess = fX;

    for (int i = 0; i < 5; i++) {
        fGuess = 0.5f * (fGuess + fX / fGuess);
    }

    return fGuess;
}

/******************************************************************************
* @brief       Electrical sin/cos from raw mechanical angle (4 pole pairs)
* @param[in]   ui16MechRaw  Encoder angle, 12 bits (0..4095)
* @param[out]  pfSinTheta   sin of electrical angle
* @param[out]  pfCosTheta   cos of electrical angle
******************************************************************************/
void vFOC_RotorRawToSinCos(uint16_t ui16MechRaw, float *pfSinTheta, float *pfCosTheta) {
    uint16_t ui16EIdx = (uint16_t)((ui16MechRaw << 2) & (LUT_SIZE - 1u));
    *pfSinTheta = fSinLut[ui16EIdx];
    *pfCosTheta = fCosLut[ui16EIdx];
}

/******************************************************************************
* @brief       Same as vFOC_RotorRawToSinCos, shifted -60 deg electrical
*              (low-side -> inline current sensing compatibility)
* @param[in]   ui16MechRaw  Encoder angle, 12 bits (0..4095)
* @param[out]  pfSinTheta   sin of electrical angle
* @param[out]  pfCosTheta   cos of electrical angle
******************************************************************************/
void vFOC_RotorRawToSinCosM60(uint16_t ui16MechRaw, float *pfSinTheta, float *pfCosTheta) {
    uint16_t ui16M = (uint16_t)((ui16MechRaw - A1339_OFFSET_ELECTRICAL_60DEG) & (LUT_SIZE - 1u));
    uint16_t ui16EIdx = (uint16_t)((ui16M << 2) & (LUT_SIZE - 1u));
    *pfSinTheta = fSinLut[ui16EIdx];
    *pfCosTheta = fCosLut[ui16EIdx];
}

/******************************************************************************
* @brief       First-order IIR low-pass step: y += k*(x - y)
* @param[in]   pxF    Filter instance
* @param[in]   fX     New input sample
* @return      float  Filtered output
******************************************************************************/
float fFOC_IirLpfUpdate(xFOC_IirLpf *pxF, float fX) {
    pxF->fY += pxF->fK * (fX - pxF->fY);
    return pxF->fY;
}

/******************************************************************************
* @brief       Clarke transform (2 phases, amplitude invariant)
* @param[in]   fIa, fIb              Phase currents
* @param[out]  pfIalpha, pfIbeta     Stationary frame currents
******************************************************************************/
void vFOC_ClarkeTransform(float fIa, float fIb, float *pfIalpha, float *pfIbeta) {
    // Clarke transform
    *pfIalpha = fIa;
    *pfIbeta  = ONE_BY_SQRT3 * fIa + TWO_BY_SQRT3 * fIb;
}

/******************************************************************************
* @brief       Park transform (stationary -> rotor frame)
* @param[in]   fIalpha, fIbeta       Stationary frame currents
* @param[in]   fSinTheta, fCosTheta  Rotor electrical angle
* @param[out]  pfId, pfIq            Rotor frame currents
******************************************************************************/
void vFOC_ParkTransform(float fIalpha, float fIbeta, float fSinTheta, float fCosTheta, float *pfId, float *pfIq) {
    // Park transform
    *pfId = fIalpha * fCosTheta + fIbeta * fSinTheta;
    *pfIq = fIbeta * fCosTheta - fIalpha * fSinTheta;
}

/******************************************************************************
* @brief       Inverse Park transform (rotor -> stationary frame)
* @param[in]   fVd, fVq              Rotor frame voltages
* @param[in]   fSinTheta, fCosTheta  Rotor electrical angle
* @param[out]  pfValpha, pfVbeta     Stationary frame voltages
******************************************************************************/
void vFOC_InverseParkTransform(float fVd, float fVq, float fSinTheta, float fCosTheta, float *pfValpha, float *pfVbeta) {
    *pfValpha = fVd * fCosTheta - fVq * fSinTheta;
    *pfVbeta  = fVd * fSinTheta + fVq * fCosTheta;
}

/******************************************************************************
* @brief       Space Vector PWM modulation (sector based)
* @param[in]   fValpha, fVbeta  Stationary frame voltage request
* @param[in]   fVbus            DC bus voltage
* @param[in]   ui32PwmPeriod    Full PWM period value [counts]
* @param[out]  pui32PwmU/V/W    Duty cycles (0 to ui32PwmPeriod)
******************************************************************************/
void vFOC_Svpwm(float fValpha, float fVbeta, float fVbus, uint32_t ui32PwmPeriod,
                uint32_t *pui32PwmU, uint32_t *pui32PwmV, uint32_t *pui32PwmW)
{
    // 1. Normalize voltages by vbus
    float fAlpha = fValpha / fVbus;
    float fBeta = fVbeta / fVbus;

    // 2. Sector determination
    uint8_t ui8Sector;
    if (fBeta >= 0.0f) {
        if (fAlpha >= 0.0f) {
            ui8Sector = (ONE_BY_SQRT3 * fBeta > fAlpha) ? 2 : 1;  // 1/sqrt(3) ≈ 0.577
        } else {
            ui8Sector = (-ONE_BY_SQRT3 * fBeta > fAlpha) ? 3 : 2;
        }
    } else {
        if (fAlpha >= 0.0f) {
            ui8Sector = (-ONE_BY_SQRT3 * fBeta > fAlpha) ? 5 : 6;
        } else {
            ui8Sector = (ONE_BY_SQRT3 * fBeta > fAlpha) ? 4 : 5;
        }
    }

    // 3. Calculate active vector times
    int32_t s32T1, s32T2;
    switch(ui8Sector) {
        case 1:
            s32T1 = (int32_t)((fAlpha - ONE_BY_SQRT3 * fBeta) * ui32PwmPeriod);
            s32T2 = (int32_t)(TWO_BY_SQRT3 * fBeta * ui32PwmPeriod);
            *pui32PwmU = (ui32PwmPeriod + s32T1 + s32T2) / 2;
            *pui32PwmV = *pui32PwmU - s32T1;
            *pui32PwmW = *pui32PwmV - s32T2;
            break;

        case 2:
            s32T1 = (int32_t)((fAlpha + ONE_BY_SQRT3 * fBeta) * ui32PwmPeriod);
            s32T2 = (int32_t)((-fAlpha + ONE_BY_SQRT3 * fBeta) * ui32PwmPeriod);
            *pui32PwmV = (ui32PwmPeriod + s32T1 + s32T2) / 2;
            *pui32PwmU = *pui32PwmV - s32T2;
            *pui32PwmW = *pui32PwmU - s32T1;
            break;

        case 3:
            s32T1 = (int32_t)(TWO_BY_SQRT3 * fBeta * ui32PwmPeriod);
            s32T2 = (int32_t)((-fAlpha - ONE_BY_SQRT3 * fBeta) * ui32PwmPeriod);
            *pui32PwmV = (ui32PwmPeriod + s32T1 + s32T2) / 2;
            *pui32PwmW = *pui32PwmV - s32T1;
            *pui32PwmU = *pui32PwmW - s32T2;
            break;

        case 4:
            s32T1 = (int32_t)((-fAlpha + ONE_BY_SQRT3 * fBeta) * ui32PwmPeriod);
            s32T2 = (int32_t)(-TWO_BY_SQRT3 * fBeta * ui32PwmPeriod);
            *pui32PwmW = (ui32PwmPeriod + s32T1 + s32T2) / 2;
            *pui32PwmV = *pui32PwmW - s32T2;
            *pui32PwmU = *pui32PwmV - s32T1;
            break;

        case 5:
            s32T1 = (int32_t)((-fAlpha - ONE_BY_SQRT3 * fBeta) * ui32PwmPeriod);
            s32T2 = (int32_t)((fAlpha - ONE_BY_SQRT3 * fBeta) * ui32PwmPeriod);
            *pui32PwmW = (ui32PwmPeriod + s32T1 + s32T2) / 2;
            *pui32PwmU = *pui32PwmW - s32T1;
            *pui32PwmV = *pui32PwmU - s32T2;
            break;

        case 6:
            s32T1 = (int32_t)(-TWO_BY_SQRT3 * fBeta * ui32PwmPeriod);
            s32T2 = (int32_t)((fAlpha + ONE_BY_SQRT3 * fBeta) * ui32PwmPeriod);
            *pui32PwmU = (ui32PwmPeriod + s32T1 + s32T2) / 2;
            *pui32PwmW = *pui32PwmU - s32T2;
            *pui32PwmV = *pui32PwmW - s32T1;
            break;
    }

    // 4. Clamp outputs to valid range
    *pui32PwmU = (*pui32PwmU > ui32PwmPeriod) ? ui32PwmPeriod : *pui32PwmU;
    *pui32PwmV = (*pui32PwmV > ui32PwmPeriod) ? ui32PwmPeriod : *pui32PwmV;
    *pui32PwmW = (*pui32PwmW > ui32PwmPeriod) ? ui32PwmPeriod : *pui32PwmW;
}

/* [] END OF FILE */
