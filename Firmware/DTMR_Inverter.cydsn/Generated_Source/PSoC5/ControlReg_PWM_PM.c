/*******************************************************************************
* File Name: ControlReg_PWM_PM.c
* Version 1.80
*
* Description:
*  This file contains the setup, control, and status commands to support 
*  the component operation in the low power mode. 
*
* Note:
*
********************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "ControlReg_PWM.h"

/* Check for removal by optimization */
#if !defined(ControlReg_PWM_Sync_ctrl_reg__REMOVED)

static ControlReg_PWM_BACKUP_STRUCT  ControlReg_PWM_backup = {0u};

    
/*******************************************************************************
* Function Name: ControlReg_PWM_SaveConfig
********************************************************************************
*
* Summary:
*  Saves the control register value.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void ControlReg_PWM_SaveConfig(void) 
{
    ControlReg_PWM_backup.controlState = ControlReg_PWM_Control;
}


/*******************************************************************************
* Function Name: ControlReg_PWM_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the control register value.
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
void ControlReg_PWM_RestoreConfig(void) 
{
     ControlReg_PWM_Control = ControlReg_PWM_backup.controlState;
}


/*******************************************************************************
* Function Name: ControlReg_PWM_Sleep
********************************************************************************
*
* Summary:
*  Prepares the component for entering the low power mode.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void ControlReg_PWM_Sleep(void) 
{
    ControlReg_PWM_SaveConfig();
}


/*******************************************************************************
* Function Name: ControlReg_PWM_Wakeup
********************************************************************************
*
* Summary:
*  Restores the component after waking up from the low power mode.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void ControlReg_PWM_Wakeup(void)  
{
    ControlReg_PWM_RestoreConfig();
}

#endif /* End check for removal by optimization */


/* [] END OF FILE */
