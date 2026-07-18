/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/
#ifndef DUTYMEAS_H
#define DUTYMEAS_H

#include "stdint.h"
    
// task to measure. select ONE:
//#define MEAS_TASK_FOC_HFT_4K
#define MEAS_TASK_FOC_LFT_1K
    
#if defined(MEAS_TASK_FOC_HFT_4K)
#define HISTOGRAM_NUMBER_OF_SAMPLES     (4000u * 100u) // ~100 seconds for HFT
#elif defined(MEAS_TASK_FOC_LFT_1K)
#define HISTOGRAM_NUMBER_OF_SAMPLES     (1000u * 100u) // ~100 seconds for LFT
#endif

void vDutyMeas_Init(void);
void vDutyMeas_RegisterSample(uint32_t ui32ExecTimeUs);
void vDutyMeas_Process(void);

#endif /* DUTYMEAS_H */

/* [] END OF FILE */

