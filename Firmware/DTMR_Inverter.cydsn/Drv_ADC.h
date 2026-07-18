/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#ifndef DRV_ADC_H
#define DRV_ADC_H

#include "stdbool.h"
#include "stdint.h"

bool  bDrvAdc_RunCurrentSense(uint16_t ui16RawU, uint16_t ui16RawW);

float fDrvAdc_GetCurrentU(void);
float fDrvAdc_GetCurrentV(void);
float fDrvAdc_GetCurrentW(void);

#endif /* DRV_ADC_H */

/* [] END OF FILE */
