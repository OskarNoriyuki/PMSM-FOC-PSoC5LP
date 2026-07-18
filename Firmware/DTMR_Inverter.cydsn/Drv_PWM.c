/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#include "Drv_PWM.h"
#include "project.h"

#define PWM_PERIOD              2000u   /* MASTER_CLK/2 / 2000 = 16 kHz */

#define REG_PWM_CTRL_B0_RST     (1U << 0)
#define REG_PWM_CTRL_B1_OUTEN   (1U << 1)
#define REG_PWM_CTRL_B2_GD_EN   (1U << 2)

static bool     bPwmInitialized = false;
static uint16_t ui16PwmCtrlReg  = 0u;

/******************************************************************************
* @brief       Starts the three PWM blocks, synchronizes phases and enables
*              gate driver (DRV8302) and outputs
******************************************************************************/
void vDrvPwm_Init(void)
{
    if (!bPwmInitialized)
    {
        PWM_U_Start();
        PWM_V_Start();
        PWM_W_Start();

        ui16PwmCtrlReg |= REG_PWM_CTRL_B0_RST;              /* sincroniza fases */
        ControlReg_PWM_Write(ui16PwmCtrlReg);
        CyDelay(1);
        ui16PwmCtrlReg &= (uint16_t)~REG_PWM_CTRL_B0_RST;
        ControlReg_PWM_Write(ui16PwmCtrlReg);

        ui16PwmCtrlReg |= REG_PWM_CTRL_B2_GD_EN;            /* habilita DRV8302 */
        ControlReg_PWM_Write(ui16PwmCtrlReg);

        ui16PwmCtrlReg |= REG_PWM_CTRL_B1_OUTEN;            /* habilita saidas  */
        ControlReg_PWM_Write(ui16PwmCtrlReg);

        bPwmInitialized = true;
    }
}

/******************************************************************************
* @brief       Converts duty [0..1] to compare counts
* @param[in]   fDuty   Duty cycle, clamped to [0..1]
* @return      uint16  Compare value (0..PWM_PERIOD)
******************************************************************************/
static uint16_t ui16DutyToCmp(float fDuty)
{
    if (fDuty <= 0.0f) { return 0u; }
    if (fDuty >= 1.0f) { return (uint16_t)PWM_PERIOD; }
    return (uint16_t)(fDuty * (float)PWM_PERIOD);
}

/******************************************************************************
* @brief       Updates the three phase duties from normalized values
* @param[in]   xDuty  Duties in [0..1]
* @return      bool   true on error (driver not initialized)
******************************************************************************/
bool bDrvPwm_Update(xData_3F xDuty)
{
    if (!bPwmInitialized) { return true; }

    PWM_U_WriteCompare(ui16DutyToCmp(xDuty.xPh.fU));
    PWM_V_WriteCompare(ui16DutyToCmp(xDuty.xPh.fV));
    PWM_W_WriteCompare(ui16DutyToCmp(xDuty.xPh.fW));
    return false;
}

/******************************************************************************
* @brief       Updates the three phase duties from raw compare counts
* @param[in]   ui32DutyU/V/W  Compare values (0..PWM_PERIOD)
* @return      bool           true on error (driver not initialized)
******************************************************************************/
bool bDrvPwm_UpdateInt(uint32_t ui32DutyU, uint32_t ui32DutyV, uint32_t ui32DutyW)
{
    if (!bPwmInitialized) { return true; }

    PWM_U_WriteCompare((uint16)ui32DutyU);
    PWM_V_WriteCompare((uint16)ui32DutyV);
    PWM_W_WriteCompare((uint16)ui32DutyW);

    return false;
}
