/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/
    
#ifndef DRV_CYCLE_H
#define DRV_CYCLE_H

#include "stdint.h"

void     vDrvCycle_Init(void);
void     vDrvCycle_Start(void);
uint32_t ui32DrvCycle_GetDiff(void);   /* us decorridos desde vDrvCycle_Start() */

#endif /* DRV_CYCLE_H */
