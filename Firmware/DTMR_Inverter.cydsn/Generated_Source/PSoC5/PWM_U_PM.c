/*******************************************************************************
* File Name: PWM_U_PM.c
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

#include "PWM_U.h"

static PWM_U_backupStruct PWM_U_backup;


/*******************************************************************************
* Function Name: PWM_U_SaveConfig
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
*  PWM_U_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void PWM_U_SaveConfig(void) 
{

    #if(!PWM_U_UsingFixedFunction)
        #if(!PWM_U_PWMModeIsCenterAligned)
            PWM_U_backup.PWMPeriod = PWM_U_ReadPeriod();
        #endif /* (!PWM_U_PWMModeIsCenterAligned) */
        PWM_U_backup.PWMUdb = PWM_U_ReadCounter();
        #if (PWM_U_UseStatus)
            PWM_U_backup.InterruptMaskValue = PWM_U_STATUS_MASK;
        #endif /* (PWM_U_UseStatus) */

        #if(PWM_U_DeadBandMode == PWM_U__B_PWM__DBM_256_CLOCKS || \
            PWM_U_DeadBandMode == PWM_U__B_PWM__DBM_2_4_CLOCKS)
            PWM_U_backup.PWMdeadBandValue = PWM_U_ReadDeadTime();
        #endif /*  deadband count is either 2-4 clocks or 256 clocks */

        #if(PWM_U_KillModeMinTime)
             PWM_U_backup.PWMKillCounterPeriod = PWM_U_ReadKillTime();
        #endif /* (PWM_U_KillModeMinTime) */

        #if(PWM_U_UseControl)
            PWM_U_backup.PWMControlRegister = PWM_U_ReadControlRegister();
        #endif /* (PWM_U_UseControl) */
    #endif  /* (!PWM_U_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: PWM_U_RestoreConfig
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
*  PWM_U_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void PWM_U_RestoreConfig(void) 
{
        #if(!PWM_U_UsingFixedFunction)
            #if(!PWM_U_PWMModeIsCenterAligned)
                PWM_U_WritePeriod(PWM_U_backup.PWMPeriod);
            #endif /* (!PWM_U_PWMModeIsCenterAligned) */

            PWM_U_WriteCounter(PWM_U_backup.PWMUdb);

            #if (PWM_U_UseStatus)
                PWM_U_STATUS_MASK = PWM_U_backup.InterruptMaskValue;
            #endif /* (PWM_U_UseStatus) */

            #if(PWM_U_DeadBandMode == PWM_U__B_PWM__DBM_256_CLOCKS || \
                PWM_U_DeadBandMode == PWM_U__B_PWM__DBM_2_4_CLOCKS)
                PWM_U_WriteDeadTime(PWM_U_backup.PWMdeadBandValue);
            #endif /* deadband count is either 2-4 clocks or 256 clocks */

            #if(PWM_U_KillModeMinTime)
                PWM_U_WriteKillTime(PWM_U_backup.PWMKillCounterPeriod);
            #endif /* (PWM_U_KillModeMinTime) */

            #if(PWM_U_UseControl)
                PWM_U_WriteControlRegister(PWM_U_backup.PWMControlRegister);
            #endif /* (PWM_U_UseControl) */
        #endif  /* (!PWM_U_UsingFixedFunction) */
    }


/*******************************************************************************
* Function Name: PWM_U_Sleep
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
*  PWM_U_backup.PWMEnableState:  Is modified depending on the enable
*  state of the block before entering sleep mode.
*
*******************************************************************************/
void PWM_U_Sleep(void) 
{
    #if(PWM_U_UseControl)
        if(PWM_U_CTRL_ENABLE == (PWM_U_CONTROL & PWM_U_CTRL_ENABLE))
        {
            /*Component is enabled */
            PWM_U_backup.PWMEnableState = 1u;
        }
        else
        {
            /* Component is disabled */
            PWM_U_backup.PWMEnableState = 0u;
        }
    #endif /* (PWM_U_UseControl) */

    /* Stop component */
    PWM_U_Stop();

    /* Save registers configuration */
    PWM_U_SaveConfig();
}


/*******************************************************************************
* Function Name: PWM_U_Wakeup
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
*  PWM_U_backup.pwmEnable:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void PWM_U_Wakeup(void) 
{
     /* Restore registers values */
    PWM_U_RestoreConfig();

    if(PWM_U_backup.PWMEnableState != 0u)
    {
        /* Enable component's operation */
        PWM_U_Enable();
    } /* Do nothing if component's block was disabled before */

}


/* [] END OF FILE */
