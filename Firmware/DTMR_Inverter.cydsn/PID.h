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

#ifndef PID_H_
#define PID_H_

#include <stdint.h>

typedef struct {
    float fKp;           // Proportional gain
    float fKi;           // Integral gain
    float fKd;           // Differential gain
    float fIntegral;     // Integral accumulator
    float fOutMax;       // Output upper limit (clamp)
    float fOutMin;
    float fTs;
    float fEDeadband;
    float fLastError;
    float fDAlphaFilter; // Derivative filter coefficient
    float fDFiltered;
    float fDMax;
    float fMv;
} xPID_Controller;

float fPID_PiControl(xPID_Controller *pxPi, float fError);
float fPID_PidControl(xPID_Controller *pxPid, float fError);
void  vPID_SetDeadband(xPID_Controller *pxPid, float fDeadband);
void  vPID_SetDFilterFc(xPID_Controller *pxPid, float fFc);
void  vPID_SetMaxD(xPID_Controller *pxPid, float fMax);

#endif /* PID_H_ */

/* [] END OF FILE */
