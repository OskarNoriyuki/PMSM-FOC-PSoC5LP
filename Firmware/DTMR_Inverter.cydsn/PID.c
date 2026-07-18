/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 * Copyright (c) 2026 sirojudin munir
 *
 * Based on siroju's pid_utils library.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

/*
 * TODO copy MIT license
 */

#include "PID.h"

#ifndef TWO_PI
#define TWO_PI 6.2831853f
#endif

/******************************************************************************
* @brief       PI step with anti-windup (integrator clamping)
* @param[in]   pxPi    Controller instance
* @param[in]   fError  Setpoint - feedback
* @return      float   Output, clamped to [fOutMin, fOutMax]
******************************************************************************/
float fPID_PiControl(xPID_Controller *pxPi, float fError) {
    if (fError >= -pxPi->fEDeadband && fError <= pxPi->fEDeadband) {
        fError = 0.0f;
    }

    float fPTerm = pxPi->fKp * fError;

    float fNewIntegral = pxPi->fIntegral + fError * pxPi->fKi * pxPi->fTs;

    float fOutput = fPTerm + fNewIntegral;

    // Anti-windup with clamping
    if (fOutput > pxPi->fOutMax) {
        fOutput = pxPi->fOutMax;
        pxPi->fIntegral = fOutput - fPTerm;
    }
    else if (fOutput < pxPi->fOutMin) {
        fOutput = pxPi->fOutMin;
        pxPi->fIntegral = fOutput - fPTerm;
    }
    else {
        pxPi->fIntegral = fNewIntegral;
    }

    pxPi->fMv = fOutput;

    return fOutput;
}

/******************************************************************************
* @brief       PID step with filtered/clamped derivative and anti-windup
* @param[in]   pxPid   Controller instance
* @param[in]   fError  Setpoint - feedback
* @return      float   Output, clamped to [fOutMin, fOutMax]
******************************************************************************/
float fPID_PidControl(xPID_Controller *pxPid, float fError) {
    if (fError >= -pxPid->fEDeadband && fError <= pxPid->fEDeadband) {
        fError = 0.0f;
    }

    float fPTerm = pxPid->fKp * fError;

    float fErrorDerivative = (fError - pxPid->fLastError) / pxPid->fTs;
    pxPid->fDFiltered = (1.0f - pxPid->fDAlphaFilter) * pxPid->fDFiltered + pxPid->fDAlphaFilter * fErrorDerivative;
    // clamp derivative
    if (pxPid->fDFiltered > pxPid->fDMax) pxPid->fDFiltered = pxPid->fDMax;
    else if (pxPid->fDFiltered < -pxPid->fDMax) pxPid->fDFiltered = -pxPid->fDMax;
    float fDTerm = pxPid->fDFiltered * pxPid->fKd;
    pxPid->fLastError = fError;

    float fNewIntegral = pxPid->fIntegral + fError * pxPid->fKi * pxPid->fTs;

    float fOutput = fPTerm + fDTerm + fNewIntegral;

    // Anti-windup with clamping
    if (fOutput > pxPid->fOutMax) {
        fOutput = pxPid->fOutMax;
        pxPid->fIntegral = fOutput - (fPTerm + fDTerm);
    }
    else if (fOutput < pxPid->fOutMin) {
        fOutput = pxPid->fOutMin;
        pxPid->fIntegral = fOutput - (fPTerm + fDTerm);
    }
    else {
        pxPid->fIntegral = fNewIntegral;
    }

    return fOutput;
}

/******************************************************************************
* @brief       Sets the symmetric error deadband
* @param[in]   pxPid      Controller instance
* @param[in]   fDeadband  Error band treated as zero
******************************************************************************/
void vPID_SetDeadband(xPID_Controller *pxPid, float fDeadband) {
    pxPid->fEDeadband = fDeadband;
}

/******************************************************************************
* @brief       Sets the derivative low-pass filter cutoff frequency
* @param[in]   pxPid  Controller instance
* @param[in]   fFc    Cutoff frequency [Hz]; requires fTs already set
******************************************************************************/
void vPID_SetDFilterFc(xPID_Controller *pxPid, float fFc) {
    float fTau = 1.0f / (TWO_PI * fFc);
    pxPid->fDAlphaFilter = pxPid->fTs / (fTau + pxPid->fTs);
    if (pxPid->fDAlphaFilter > 1.0f) pxPid->fDAlphaFilter = 1.0f;
}

/******************************************************************************
* @brief       Sets the clamp for the filtered derivative
* @param[in]   pxPid  Controller instance
* @param[in]   fMax   Absolute limit; must be > 0
******************************************************************************/
void vPID_SetMaxD(xPID_Controller *pxPid, float fMax) {
    if (fMax <= 0) return;
    pxPid->fDMax = fMax;
}

/* [] END OF FILE */
