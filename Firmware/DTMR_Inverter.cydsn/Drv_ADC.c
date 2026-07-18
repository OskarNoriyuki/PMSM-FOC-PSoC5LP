/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#include "Drv_ADC.h"
#include "project.h"

#define CSENSE_CAL_SAMPLES   (4000u)

#define CSENSE_R_SHUNT       0.01f
#define CSENSE_G_AMC         8.2f
#define CSENSE_G_DIFF        1.0f
#define CSENSE_ADC_VREF      5.0f
#define CSENSE_ADC_COUNTS    4096.0f
#define CSENSE_A_PER_COUNT   ((CSENSE_ADC_VREF / CSENSE_ADC_COUNTS) / (CSENSE_R_SHUNT * CSENSE_G_AMC * CSENSE_G_DIFF))

static uint8_t   ui8CsenseCalibrated = 0u;
static uint16_t  ui16CalCount = 0u;
static uint64_t  ui64AccU = 0u;
static uint64_t  ui64AccW = 0u;
static uint16_t  ui16OffsetU = 2048u;
static uint16_t  ui16OffsetW = 2048u;

static float fIu = 0.0f;
static float fIv = 0.0f;
static float fIw = 0.0f;

/******************************************************************************
* @brief       Current sensing step: averages offsets during calibration,
*              then converts raw counts to phase currents (U + V + W = 0)
* @param[in]   ui16RawU, ui16RawW  Raw ADC samples of phases U and W
* @return      bool                true when calibrated and currents are valid
******************************************************************************/
bool bDrvAdc_RunCurrentSense(uint16_t ui16RawU, uint16_t ui16RawW)
{
    if (!ui8CsenseCalibrated)
    {
        ui64AccU += (uint64_t)ui16RawU;
        ui64AccW += (uint64_t)ui16RawW;
        ui16CalCount++;

        if (ui16CalCount >= CSENSE_CAL_SAMPLES)
        {
            ui16OffsetU = (uint16_t)(ui64AccU / CSENSE_CAL_SAMPLES);
            ui16OffsetW = (uint16_t)(ui64AccW / CSENSE_CAL_SAMPLES);
            ui8CsenseCalibrated = 1u;
        }
        return false;
    }

    fIu = ((float)ui16RawU - (float)ui16OffsetU) * CSENSE_A_PER_COUNT;
    fIw = ((float)ui16RawW - (float)ui16OffsetW) * CSENSE_A_PER_COUNT;
    fIv = -(fIu + fIw);

    return true;
}

/******************************************************************************
* @brief       Last converted phase currents [A]
******************************************************************************/
float fDrvAdc_GetCurrentU(void) { return fIu; }
float fDrvAdc_GetCurrentV(void) { return fIv; }
float fDrvAdc_GetCurrentW(void) { return fIw; }

/* [] END OF FILE */
