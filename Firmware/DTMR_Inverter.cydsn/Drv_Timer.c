/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#include "Drv_Timer.h"
#include "project.h"
#include "stdbool.h"
#include "stdint.h"

static uint16_t ui16TStart = 0u;

/******************************************************************************
* @brief       Starts the hardware timer block
******************************************************************************/
void vDrvTimer_Init(void)
{
    Timer_1_Start();
}

/******************************************************************************
* @brief       Latches the current counter as measurement start
******************************************************************************/
void vDrvTimer_Start(void)
{
    ui16TStart = Timer_1_ReadCounter();
}

/******************************************************************************
* @brief       Elapsed time since vDrvTimer_Start()
* @return      uint32_t  Microseconds (down-counter, 16-bit wrap safe)
******************************************************************************/
uint32_t ui32DrvTimer_GetDiff(void)
{
    uint16_t ui16TEnd = Timer_1_ReadCounter();
    return (uint32_t)(uint16_t)(ui16TStart - ui16TEnd);
}
