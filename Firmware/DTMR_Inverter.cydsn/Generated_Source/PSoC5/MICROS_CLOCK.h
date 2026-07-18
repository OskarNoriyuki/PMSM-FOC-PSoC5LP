/*******************************************************************************
* File Name: MICROS_CLOCK.h
* Version 2.20
*
*  Description:
*   Provides the function and constant definitions for the clock component.
*
*  Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_CLOCK_MICROS_CLOCK_H)
#define CY_CLOCK_MICROS_CLOCK_H

#include <cytypes.h>
#include <cyfitter.h>


/***************************************
* Conditional Compilation Parameters
***************************************/

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component cy_clock_v2_20 requires cy_boot v3.0 or later
#endif /* (CY_PSOC5LP) */


/***************************************
*        Function Prototypes
***************************************/

void MICROS_CLOCK_Start(void) ;
void MICROS_CLOCK_Stop(void) ;

#if(CY_PSOC3 || CY_PSOC5LP)
void MICROS_CLOCK_StopBlock(void) ;
#endif /* (CY_PSOC3 || CY_PSOC5LP) */

void MICROS_CLOCK_StandbyPower(uint8 state) ;
void MICROS_CLOCK_SetDividerRegister(uint16 clkDivider, uint8 restart) 
                                ;
uint16 MICROS_CLOCK_GetDividerRegister(void) ;
void MICROS_CLOCK_SetModeRegister(uint8 modeBitMask) ;
void MICROS_CLOCK_ClearModeRegister(uint8 modeBitMask) ;
uint8 MICROS_CLOCK_GetModeRegister(void) ;
void MICROS_CLOCK_SetSourceRegister(uint8 clkSource) ;
uint8 MICROS_CLOCK_GetSourceRegister(void) ;
#if defined(MICROS_CLOCK__CFG3)
void MICROS_CLOCK_SetPhaseRegister(uint8 clkPhase) ;
uint8 MICROS_CLOCK_GetPhaseRegister(void) ;
#endif /* defined(MICROS_CLOCK__CFG3) */

#define MICROS_CLOCK_Enable()                       MICROS_CLOCK_Start()
#define MICROS_CLOCK_Disable()                      MICROS_CLOCK_Stop()
#define MICROS_CLOCK_SetDivider(clkDivider)         MICROS_CLOCK_SetDividerRegister(clkDivider, 1u)
#define MICROS_CLOCK_SetDividerValue(clkDivider)    MICROS_CLOCK_SetDividerRegister((clkDivider) - 1u, 1u)
#define MICROS_CLOCK_SetMode(clkMode)               MICROS_CLOCK_SetModeRegister(clkMode)
#define MICROS_CLOCK_SetSource(clkSource)           MICROS_CLOCK_SetSourceRegister(clkSource)
#if defined(MICROS_CLOCK__CFG3)
#define MICROS_CLOCK_SetPhase(clkPhase)             MICROS_CLOCK_SetPhaseRegister(clkPhase)
#define MICROS_CLOCK_SetPhaseValue(clkPhase)        MICROS_CLOCK_SetPhaseRegister((clkPhase) + 1u)
#endif /* defined(MICROS_CLOCK__CFG3) */


/***************************************
*             Registers
***************************************/

/* Register to enable or disable the clock */
#define MICROS_CLOCK_CLKEN              (* (reg8 *) MICROS_CLOCK__PM_ACT_CFG)
#define MICROS_CLOCK_CLKEN_PTR          ((reg8 *) MICROS_CLOCK__PM_ACT_CFG)

/* Register to enable or disable the clock */
#define MICROS_CLOCK_CLKSTBY            (* (reg8 *) MICROS_CLOCK__PM_STBY_CFG)
#define MICROS_CLOCK_CLKSTBY_PTR        ((reg8 *) MICROS_CLOCK__PM_STBY_CFG)

/* Clock LSB divider configuration register. */
#define MICROS_CLOCK_DIV_LSB            (* (reg8 *) MICROS_CLOCK__CFG0)
#define MICROS_CLOCK_DIV_LSB_PTR        ((reg8 *) MICROS_CLOCK__CFG0)
#define MICROS_CLOCK_DIV_PTR            ((reg16 *) MICROS_CLOCK__CFG0)

/* Clock MSB divider configuration register. */
#define MICROS_CLOCK_DIV_MSB            (* (reg8 *) MICROS_CLOCK__CFG1)
#define MICROS_CLOCK_DIV_MSB_PTR        ((reg8 *) MICROS_CLOCK__CFG1)

/* Mode and source configuration register */
#define MICROS_CLOCK_MOD_SRC            (* (reg8 *) MICROS_CLOCK__CFG2)
#define MICROS_CLOCK_MOD_SRC_PTR        ((reg8 *) MICROS_CLOCK__CFG2)

#if defined(MICROS_CLOCK__CFG3)
/* Analog clock phase configuration register */
#define MICROS_CLOCK_PHASE              (* (reg8 *) MICROS_CLOCK__CFG3)
#define MICROS_CLOCK_PHASE_PTR          ((reg8 *) MICROS_CLOCK__CFG3)
#endif /* defined(MICROS_CLOCK__CFG3) */


/**************************************
*       Register Constants
**************************************/

/* Power manager register masks */
#define MICROS_CLOCK_CLKEN_MASK         MICROS_CLOCK__PM_ACT_MSK
#define MICROS_CLOCK_CLKSTBY_MASK       MICROS_CLOCK__PM_STBY_MSK

/* CFG2 field masks */
#define MICROS_CLOCK_SRC_SEL_MSK        MICROS_CLOCK__CFG2_SRC_SEL_MASK
#define MICROS_CLOCK_MODE_MASK          (~(MICROS_CLOCK_SRC_SEL_MSK))

#if defined(MICROS_CLOCK__CFG3)
/* CFG3 phase mask */
#define MICROS_CLOCK_PHASE_MASK         MICROS_CLOCK__CFG3_PHASE_DLY_MASK
#endif /* defined(MICROS_CLOCK__CFG3) */

#endif /* CY_CLOCK_MICROS_CLOCK_H */


/* [] END OF FILE */
