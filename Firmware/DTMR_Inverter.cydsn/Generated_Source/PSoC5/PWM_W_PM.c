/*******************************************************************************
* File Name: PWM_W_PM.c
* Version 3.30
*
* Description:
*  This file provides the power management source code to API for the
*  PWM.
*
* Note:
*
********************************************************************************
* Copyright 2008-2014, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "PWM_W.h"

static PWM_W_backupStruct PWM_W_backup;


/*******************************************************************************
* Function Name: PWM_W_SaveConfig
********************************************************************************
*
* Summary:
*  Saves the current user configuration of the component.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  PWM_W_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void PWM_W_SaveConfig(void) 
{

    #if(!PWM_W_UsingFixedFunction)
        #if(!PWM_W_PWMModeIsCenterAligned)
            PWM_W_backup.PWMPeriod = PWM_W_ReadPeriod();
        #endif /* (!PWM_W_PWMModeIsCenterAligned) */
        PWM_W_backup.PWMUdb = PWM_W_ReadCounter();
        #if (PWM_W_UseStatus)
            PWM_W_backup.InterruptMaskValue = PWM_W_STATUS_MASK;
        #endif /* (PWM_W_UseStatus) */

        #if(PWM_W_DeadBandMode == PWM_W__B_PWM__DBM_256_CLOCKS || \
            PWM_W_DeadBandMode == PWM_W__B_PWM__DBM_2_4_CLOCKS)
            PWM_W_backup.PWMdeadBandValue = PWM_W_ReadDeadTime();
        #endif /*  deadband count is either 2-4 clocks or 256 clocks */

        #if(PWM_W_KillModeMinTime)
             PWM_W_backup.PWMKillCounterPeriod = PWM_W_ReadKillTime();
        #endif /* (PWM_W_KillModeMinTime) */

        #if(PWM_W_UseControl)
            PWM_W_backup.PWMControlRegister = PWM_W_ReadControlRegister();
        #endif /* (PWM_W_UseControl) */
    #endif  /* (!PWM_W_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: PWM_W_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration of the component.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  PWM_W_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void PWM_W_RestoreConfig(void) 
{
        #if(!PWM_W_UsingFixedFunction)
            #if(!PWM_W_PWMModeIsCenterAligned)
                PWM_W_WritePeriod(PWM_W_backup.PWMPeriod);
            #endif /* (!PWM_W_PWMModeIsCenterAligned) */

            PWM_W_WriteCounter(PWM_W_backup.PWMUdb);

            #if (PWM_W_UseStatus)
                PWM_W_STATUS_MASK = PWM_W_backup.InterruptMaskValue;
            #endif /* (PWM_W_UseStatus) */

            #if(PWM_W_DeadBandMode == PWM_W__B_PWM__DBM_256_CLOCKS || \
                PWM_W_DeadBandMode == PWM_W__B_PWM__DBM_2_4_CLOCKS)
                PWM_W_WriteDeadTime(PWM_W_backup.PWMdeadBandValue);
            #endif /* deadband count is either 2-4 clocks or 256 clocks */

            #if(PWM_W_KillModeMinTime)
                PWM_W_WriteKillTime(PWM_W_backup.PWMKillCounterPeriod);
            #endif /* (PWM_W_KillModeMinTime) */

            #if(PWM_W_UseControl)
                PWM_W_WriteControlRegister(PWM_W_backup.PWMControlRegister);
            #endif /* (PWM_W_UseControl) */
        #endif  /* (!PWM_W_UsingFixedFunction) */
    }


/*******************************************************************************
* Function Name: PWM_W_Sleep
********************************************************************************
*
* Summary:
*  Disables block's operation and saves the user configuration. Should be called
*  just prior to entering sleep.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  PWM_W_backup.PWMEnableState:  Is modified depending on the enable
*  state of the block before entering sleep mode.
*
*******************************************************************************/
void PWM_W_Sleep(void) 
{
    #if(PWM_W_UseControl)
        if(PWM_W_CTRL_ENABLE == (PWM_W_CONTROL & PWM_W_CTRL_ENABLE))
        {
            /*Component is enabled */
            PWM_W_backup.PWMEnableState = 1u;
        }
        else
        {
            /* Component is disabled */
            PWM_W_backup.PWMEnableState = 0u;
        }
    #endif /* (PWM_W_UseControl) */

    /* Stop component */
    PWM_W_Stop();

    /* Save registers configuration */
    PWM_W_SaveConfig();
}


/*******************************************************************************
* Function Name: PWM_W_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration. Should be called just after
*  awaking from sleep.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  PWM_W_backup.pwmEnable:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void PWM_W_Wakeup(void) 
{
     /* Restore registers values */
    PWM_W_RestoreConfig();

    if(PWM_W_backup.PWMEnableState != 0u)
    {
        /* Enable component's operation */
        PWM_W_Enable();
    } /* Do nothing if component's block was disabled before */

}


/* [] END OF FILE */
