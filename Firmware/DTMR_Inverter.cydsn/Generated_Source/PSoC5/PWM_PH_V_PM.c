/*******************************************************************************
* File Name: PWM_PH_V_PM.c
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

#include "PWM_PH_V.h"

static PWM_PH_V_backupStruct PWM_PH_V_backup;


/*******************************************************************************
* Function Name: PWM_PH_V_SaveConfig
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
*  PWM_PH_V_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void PWM_PH_V_SaveConfig(void) 
{

    #if(!PWM_PH_V_UsingFixedFunction)
        #if(!PWM_PH_V_PWMModeIsCenterAligned)
            PWM_PH_V_backup.PWMPeriod = PWM_PH_V_ReadPeriod();
        #endif /* (!PWM_PH_V_PWMModeIsCenterAligned) */
        PWM_PH_V_backup.PWMUdb = PWM_PH_V_ReadCounter();
        #if (PWM_PH_V_UseStatus)
            PWM_PH_V_backup.InterruptMaskValue = PWM_PH_V_STATUS_MASK;
        #endif /* (PWM_PH_V_UseStatus) */

        #if(PWM_PH_V_DeadBandMode == PWM_PH_V__B_PWM__DBM_256_CLOCKS || \
            PWM_PH_V_DeadBandMode == PWM_PH_V__B_PWM__DBM_2_4_CLOCKS)
            PWM_PH_V_backup.PWMdeadBandValue = PWM_PH_V_ReadDeadTime();
        #endif /*  deadband count is either 2-4 clocks or 256 clocks */

        #if(PWM_PH_V_KillModeMinTime)
             PWM_PH_V_backup.PWMKillCounterPeriod = PWM_PH_V_ReadKillTime();
        #endif /* (PWM_PH_V_KillModeMinTime) */

        #if(PWM_PH_V_UseControl)
            PWM_PH_V_backup.PWMControlRegister = PWM_PH_V_ReadControlRegister();
        #endif /* (PWM_PH_V_UseControl) */
    #endif  /* (!PWM_PH_V_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: PWM_PH_V_RestoreConfig
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
*  PWM_PH_V_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void PWM_PH_V_RestoreConfig(void) 
{
        #if(!PWM_PH_V_UsingFixedFunction)
            #if(!PWM_PH_V_PWMModeIsCenterAligned)
                PWM_PH_V_WritePeriod(PWM_PH_V_backup.PWMPeriod);
            #endif /* (!PWM_PH_V_PWMModeIsCenterAligned) */

            PWM_PH_V_WriteCounter(PWM_PH_V_backup.PWMUdb);

            #if (PWM_PH_V_UseStatus)
                PWM_PH_V_STATUS_MASK = PWM_PH_V_backup.InterruptMaskValue;
            #endif /* (PWM_PH_V_UseStatus) */

            #if(PWM_PH_V_DeadBandMode == PWM_PH_V__B_PWM__DBM_256_CLOCKS || \
                PWM_PH_V_DeadBandMode == PWM_PH_V__B_PWM__DBM_2_4_CLOCKS)
                PWM_PH_V_WriteDeadTime(PWM_PH_V_backup.PWMdeadBandValue);
            #endif /* deadband count is either 2-4 clocks or 256 clocks */

            #if(PWM_PH_V_KillModeMinTime)
                PWM_PH_V_WriteKillTime(PWM_PH_V_backup.PWMKillCounterPeriod);
            #endif /* (PWM_PH_V_KillModeMinTime) */

            #if(PWM_PH_V_UseControl)
                PWM_PH_V_WriteControlRegister(PWM_PH_V_backup.PWMControlRegister);
            #endif /* (PWM_PH_V_UseControl) */
        #endif  /* (!PWM_PH_V_UsingFixedFunction) */
    }


/*******************************************************************************
* Function Name: PWM_PH_V_Sleep
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
*  PWM_PH_V_backup.PWMEnableState:  Is modified depending on the enable
*  state of the block before entering sleep mode.
*
*******************************************************************************/
void PWM_PH_V_Sleep(void) 
{
    #if(PWM_PH_V_UseControl)
        if(PWM_PH_V_CTRL_ENABLE == (PWM_PH_V_CONTROL & PWM_PH_V_CTRL_ENABLE))
        {
            /*Component is enabled */
            PWM_PH_V_backup.PWMEnableState = 1u;
        }
        else
        {
            /* Component is disabled */
            PWM_PH_V_backup.PWMEnableState = 0u;
        }
    #endif /* (PWM_PH_V_UseControl) */

    /* Stop component */
    PWM_PH_V_Stop();

    /* Save registers configuration */
    PWM_PH_V_SaveConfig();
}


/*******************************************************************************
* Function Name: PWM_PH_V_Wakeup
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
*  PWM_PH_V_backup.pwmEnable:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void PWM_PH_V_Wakeup(void) 
{
     /* Restore registers values */
    PWM_PH_V_RestoreConfig();

    if(PWM_PH_V_backup.PWMEnableState != 0u)
    {
        /* Enable component's operation */
        PWM_PH_V_Enable();
    } /* Do nothing if component's block was disabled before */

}


/* [] END OF FILE */
