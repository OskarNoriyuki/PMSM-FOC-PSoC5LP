/*******************************************************************************
* File Name: US_CLOCK.h
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

#if !defined(CY_CLOCK_US_CLOCK_H)
#define CY_CLOCK_US_CLOCK_H

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

void US_CLOCK_Start(void) ;
void US_CLOCK_Stop(void) ;

#if(CY_PSOC3 || CY_PSOC5LP)
void US_CLOCK_StopBlock(void) ;
#endif /* (CY_PSOC3 || CY_PSOC5LP) */

void US_CLOCK_StandbyPower(uint8 state) ;
void US_CLOCK_SetDividerRegister(uint16 clkDivider, uint8 restart) 
                                ;
uint16 US_CLOCK_GetDividerRegister(void) ;
void US_CLOCK_SetModeRegister(uint8 modeBitMask) ;
void US_CLOCK_ClearModeRegister(uint8 modeBitMask) ;
uint8 US_CLOCK_GetModeRegister(void) ;
void US_CLOCK_SetSourceRegister(uint8 clkSource) ;
uint8 US_CLOCK_GetSourceRegister(void) ;
#if defined(US_CLOCK__CFG3)
void US_CLOCK_SetPhaseRegister(uint8 clkPhase) ;
uint8 US_CLOCK_GetPhaseRegister(void) ;
#endif /* defined(US_CLOCK__CFG3) */

#define US_CLOCK_Enable()                       US_CLOCK_Start()
#define US_CLOCK_Disable()                      US_CLOCK_Stop()
#define US_CLOCK_SetDivider(clkDivider)         US_CLOCK_SetDividerRegister(clkDivider, 1u)
#define US_CLOCK_SetDividerValue(clkDivider)    US_CLOCK_SetDividerRegister((clkDivider) - 1u, 1u)
#define US_CLOCK_SetMode(clkMode)               US_CLOCK_SetModeRegister(clkMode)
#define US_CLOCK_SetSource(clkSource)           US_CLOCK_SetSourceRegister(clkSource)
#if defined(US_CLOCK__CFG3)
#define US_CLOCK_SetPhase(clkPhase)             US_CLOCK_SetPhaseRegister(clkPhase)
#define US_CLOCK_SetPhaseValue(clkPhase)        US_CLOCK_SetPhaseRegister((clkPhase) + 1u)
#endif /* defined(US_CLOCK__CFG3) */


/***************************************
*             Registers
***************************************/

/* Register to enable or disable the clock */
#define US_CLOCK_CLKEN              (* (reg8 *) US_CLOCK__PM_ACT_CFG)
#define US_CLOCK_CLKEN_PTR          ((reg8 *) US_CLOCK__PM_ACT_CFG)

/* Register to enable or disable the clock */
#define US_CLOCK_CLKSTBY            (* (reg8 *) US_CLOCK__PM_STBY_CFG)
#define US_CLOCK_CLKSTBY_PTR        ((reg8 *) US_CLOCK__PM_STBY_CFG)

/* Clock LSB divider configuration register. */
#define US_CLOCK_DIV_LSB            (* (reg8 *) US_CLOCK__CFG0)
#define US_CLOCK_DIV_LSB_PTR        ((reg8 *) US_CLOCK__CFG0)
#define US_CLOCK_DIV_PTR            ((reg16 *) US_CLOCK__CFG0)

/* Clock MSB divider configuration register. */
#define US_CLOCK_DIV_MSB            (* (reg8 *) US_CLOCK__CFG1)
#define US_CLOCK_DIV_MSB_PTR        ((reg8 *) US_CLOCK__CFG1)

/* Mode and source configuration register */
#define US_CLOCK_MOD_SRC            (* (reg8 *) US_CLOCK__CFG2)
#define US_CLOCK_MOD_SRC_PTR        ((reg8 *) US_CLOCK__CFG2)

#if defined(US_CLOCK__CFG3)
/* Analog clock phase configuration register */
#define US_CLOCK_PHASE              (* (reg8 *) US_CLOCK__CFG3)
#define US_CLOCK_PHASE_PTR          ((reg8 *) US_CLOCK__CFG3)
#endif /* defined(US_CLOCK__CFG3) */


/**************************************
*       Register Constants
**************************************/

/* Power manager register masks */
#define US_CLOCK_CLKEN_MASK         US_CLOCK__PM_ACT_MSK
#define US_CLOCK_CLKSTBY_MASK       US_CLOCK__PM_STBY_MSK

/* CFG2 field masks */
#define US_CLOCK_SRC_SEL_MSK        US_CLOCK__CFG2_SRC_SEL_MASK
#define US_CLOCK_MODE_MASK          (~(US_CLOCK_SRC_SEL_MSK))

#if defined(US_CLOCK__CFG3)
/* CFG3 phase mask */
#define US_CLOCK_PHASE_MASK         US_CLOCK__CFG3_PHASE_DLY_MASK
#endif /* defined(US_CLOCK__CFG3) */

#endif /* CY_CLOCK_US_CLOCK_H */


/* [] END OF FILE */
