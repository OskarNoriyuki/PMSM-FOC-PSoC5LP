/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#include "Drv_Cycle.h"
#include "project.h"

#define CYCLES_PER_US   (BCLK__BUS_CLK__MHZ)   /* 64 -> divisao vira shift */

static uint32_t ui32CStart = 0u;

/******************************************************************************
* @brief       Enables the Cortex-M3 DWT cycle counter
******************************************************************************/
void vDrvCycle_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

/******************************************************************************
* @brief       Latches the current cycle count as measurement start
******************************************************************************/
void vDrvCycle_Start(void)
{
    ui32CStart = DWT->CYCCNT;
}

/******************************************************************************
* @brief       Elapsed time since vDrvCycle_Start()
* @return      uint32_t  Microseconds
******************************************************************************/
uint32_t ui32DrvCycle_GetDiff(void)
{
    uint32_t ui32Elapsed = DWT->CYCCNT - ui32CStart;
    return ui32Elapsed / CYCLES_PER_US;
}
