/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/
    
#ifndef DRV_PWM_H
#define DRV_PWM_H

#include "stdbool.h"
#include "stdint.h"

typedef union
{
    float fData[3];
    struct { float fU; float fV; float fW; } xPh;
} xData_3F;

void vDrvPwm_Init(void);
bool bDrvPwm_Update(xData_3F xDuty);   /* duties em [0..1]; retorna true em erro */
bool bDrvPwm_UpdateInt(uint32_t ui32DutyU, uint32_t ui32DutyV, uint32_t ui32DutyW);

#endif /* DRV_PWM_H */
