/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/
    
#ifndef DRV_TIMER_H
#define DRV_TIMER_H

#include "stdint.h"

void     vDrvTimer_Init(void);
void     vDrvTimer_Start(void);
uint32_t ui32DrvTimer_GetDiff(void);   /* us decorridos desde vDrvTimer_Start() */

#endif /* DRV_TIMER_H */
