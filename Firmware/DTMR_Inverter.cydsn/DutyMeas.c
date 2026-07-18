/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#include "DutyMeas.h"
#include "Drv_UART.h"
#include "stdio.h"

#define HIST_HORIZONTAL_RES 2000
#define DUMP_COLS           10u

#if defined(MEAS_TASK_FOC_HFT_4K)
    #define MEAS_TASK_NAME  "HFT"
#elif defined(MEAS_TASK_FOC_LFT_1K)
    #define MEAS_TASK_NAME  "LFT"
#else
    #define MEAS_TASK_NAME  "NONE"
#endif

typedef enum
{
    DM_COLLECTING = 0,
    DM_READY,
    DM_DUMPING,
    DM_DONE
} xDutyMeasState;

static uint32_t ui32HistogramBuf[HIST_HORIZONTAL_RES]; // 20KB, 1us resolution, max 2ms
static volatile uint32_t ui32Wcet = 0;           // worst case execution time
static volatile uint32_t ui32OverflowCount = 0;
static volatile uint32_t ui32SampleCount = 0;
static volatile xDutyMeasState xState = DM_DONE;
static uint16_t ui16DumpIdx = 0;
static uint16_t ui16DumpLimit = 0;

/******************************************************************************
* @brief       Clears the histogram and starts a new collection window
******************************************************************************/
void vDutyMeas_Init(void)
{
    uint16_t ui16I;
    for (ui16I = 0u; ui16I < HIST_HORIZONTAL_RES; ui16I++)
    {
        ui32HistogramBuf[ui16I] = 0u;
    }
    ui32Wcet = 0u;
    ui32OverflowCount = 0u;
    ui32SampleCount = 0u;
    ui16DumpIdx = 0u;
    xState = DM_COLLECTING;
}

/******************************************************************************
* @brief       Registers one execution-time sample into the histogram
*              (ISR context; O(1))
* @param[in]   ui32ExecTimeUs  Measured execution time [us]
******************************************************************************/
void vDutyMeas_RegisterSample(uint32_t ui32ExecTimeUs)
{
    if (xState != DM_COLLECTING)
    {
        return;
    }

    if (ui32ExecTimeUs < HIST_HORIZONTAL_RES)
    {
        ui32HistogramBuf[ui32ExecTimeUs]++;
    }
    else
    {
        ui32OverflowCount++;
    }

    if (ui32ExecTimeUs > ui32Wcet)
    {
        ui32Wcet = ui32ExecTimeUs;
    }

    if (++ui32SampleCount >= HISTOGRAM_NUMBER_OF_SAMPLES)
    {
        xState = DM_READY;
    }
}

/******************************************************************************
* @brief       Background state machine: dumps the histogram over UART in
*              chunks when collection completes (call from main loop)
******************************************************************************/
void vDutyMeas_Process(void)
{
    char cLine[96];
    int  s32Len;
    uint16_t ui16I;

    switch (xState)
    {
        case DM_READY:
        {
            uint32_t ui32Lim = (ui32Wcet < HIST_HORIZONTAL_RES) ? (ui32Wcet + 1u) : (uint32_t)HIST_HORIZONTAL_RES;
            ui32Lim = ((ui32Lim + DUMP_COLS - 1u) / DUMP_COLS) * DUMP_COLS;
            if (ui32Lim > HIST_HORIZONTAL_RES) { ui32Lim = HIST_HORIZONTAL_RES; }
            ui16DumpLimit = (uint16_t)ui32Lim;
            ui16DumpIdx = 0u;
            vDrvUART_PutString("#HIST_BEGIN res_us=1 bins=2000\r\n");
            xState = DM_DUMPING;
            break;
        }

        case DM_DUMPING:
            s32Len = sprintf(cLine, "%4u:", (unsigned int)ui16DumpIdx);
            for (ui16I = 0u; (ui16I < DUMP_COLS) && (ui16DumpIdx < ui16DumpLimit); ui16I++)
            {
                s32Len += sprintf(&cLine[s32Len], " %lu", (unsigned long)ui32HistogramBuf[ui16DumpIdx]);
                ui16DumpIdx++;
            }
            cLine[s32Len]     = '\r';
            cLine[s32Len + 1] = '\n';
            cLine[s32Len + 2] = '\0';
            vDrvUART_PutString(cLine);

            if (ui16DumpIdx >= ui16DumpLimit)
            {
                sprintf(cLine, "#HIST_END task=%s wcet_us=%lu overflow=%lu samples=%lu\r\n",
                        MEAS_TASK_NAME, (unsigned long)ui32Wcet, (unsigned long)ui32OverflowCount,
                        (unsigned long)ui32SampleCount);
                vDrvUART_PutString(cLine);
                xState = DM_DONE;
            }
            break;

        default:
            break;
    }
}

/* [] END OF FILE */
