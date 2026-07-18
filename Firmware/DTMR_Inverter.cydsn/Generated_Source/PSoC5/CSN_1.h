/*******************************************************************************
* File Name: CSN_1.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_CSN_1_H) /* Pins CSN_1_H */
#define CY_PINS_CSN_1_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "CSN_1_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 CSN_1__PORT == 15 && ((CSN_1__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    CSN_1_Write(uint8 value);
void    CSN_1_SetDriveMode(uint8 mode);
uint8   CSN_1_ReadDataReg(void);
uint8   CSN_1_Read(void);
void    CSN_1_SetInterruptMode(uint16 position, uint16 mode);
uint8   CSN_1_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the CSN_1_SetDriveMode() function.
     *  @{
     */
        #define CSN_1_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define CSN_1_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define CSN_1_DM_RES_UP          PIN_DM_RES_UP
        #define CSN_1_DM_RES_DWN         PIN_DM_RES_DWN
        #define CSN_1_DM_OD_LO           PIN_DM_OD_LO
        #define CSN_1_DM_OD_HI           PIN_DM_OD_HI
        #define CSN_1_DM_STRONG          PIN_DM_STRONG
        #define CSN_1_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define CSN_1_MASK               CSN_1__MASK
#define CSN_1_SHIFT              CSN_1__SHIFT
#define CSN_1_WIDTH              1u

/* Interrupt constants */
#if defined(CSN_1__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in CSN_1_SetInterruptMode() function.
     *  @{
     */
        #define CSN_1_INTR_NONE      (uint16)(0x0000u)
        #define CSN_1_INTR_RISING    (uint16)(0x0001u)
        #define CSN_1_INTR_FALLING   (uint16)(0x0002u)
        #define CSN_1_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define CSN_1_INTR_MASK      (0x01u) 
#endif /* (CSN_1__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define CSN_1_PS                     (* (reg8 *) CSN_1__PS)
/* Data Register */
#define CSN_1_DR                     (* (reg8 *) CSN_1__DR)
/* Port Number */
#define CSN_1_PRT_NUM                (* (reg8 *) CSN_1__PRT) 
/* Connect to Analog Globals */                                                  
#define CSN_1_AG                     (* (reg8 *) CSN_1__AG)                       
/* Analog MUX bux enable */
#define CSN_1_AMUX                   (* (reg8 *) CSN_1__AMUX) 
/* Bidirectional Enable */                                                        
#define CSN_1_BIE                    (* (reg8 *) CSN_1__BIE)
/* Bit-mask for Aliased Register Access */
#define CSN_1_BIT_MASK               (* (reg8 *) CSN_1__BIT_MASK)
/* Bypass Enable */
#define CSN_1_BYP                    (* (reg8 *) CSN_1__BYP)
/* Port wide control signals */                                                   
#define CSN_1_CTL                    (* (reg8 *) CSN_1__CTL)
/* Drive Modes */
#define CSN_1_DM0                    (* (reg8 *) CSN_1__DM0) 
#define CSN_1_DM1                    (* (reg8 *) CSN_1__DM1)
#define CSN_1_DM2                    (* (reg8 *) CSN_1__DM2) 
/* Input Buffer Disable Override */
#define CSN_1_INP_DIS                (* (reg8 *) CSN_1__INP_DIS)
/* LCD Common or Segment Drive */
#define CSN_1_LCD_COM_SEG            (* (reg8 *) CSN_1__LCD_COM_SEG)
/* Enable Segment LCD */
#define CSN_1_LCD_EN                 (* (reg8 *) CSN_1__LCD_EN)
/* Slew Rate Control */
#define CSN_1_SLW                    (* (reg8 *) CSN_1__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define CSN_1_PRTDSI__CAPS_SEL       (* (reg8 *) CSN_1__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define CSN_1_PRTDSI__DBL_SYNC_IN    (* (reg8 *) CSN_1__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define CSN_1_PRTDSI__OE_SEL0        (* (reg8 *) CSN_1__PRTDSI__OE_SEL0) 
#define CSN_1_PRTDSI__OE_SEL1        (* (reg8 *) CSN_1__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define CSN_1_PRTDSI__OUT_SEL0       (* (reg8 *) CSN_1__PRTDSI__OUT_SEL0) 
#define CSN_1_PRTDSI__OUT_SEL1       (* (reg8 *) CSN_1__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define CSN_1_PRTDSI__SYNC_OUT       (* (reg8 *) CSN_1__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(CSN_1__SIO_CFG)
    #define CSN_1_SIO_HYST_EN        (* (reg8 *) CSN_1__SIO_HYST_EN)
    #define CSN_1_SIO_REG_HIFREQ     (* (reg8 *) CSN_1__SIO_REG_HIFREQ)
    #define CSN_1_SIO_CFG            (* (reg8 *) CSN_1__SIO_CFG)
    #define CSN_1_SIO_DIFF           (* (reg8 *) CSN_1__SIO_DIFF)
#endif /* (CSN_1__SIO_CFG) */

/* Interrupt Registers */
#if defined(CSN_1__INTSTAT)
    #define CSN_1_INTSTAT            (* (reg8 *) CSN_1__INTSTAT)
    #define CSN_1_SNAP               (* (reg8 *) CSN_1__SNAP)
    
	#define CSN_1_0_INTTYPE_REG 		(* (reg8 *) CSN_1__0__INTTYPE)
#endif /* (CSN_1__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_CSN_1_H */


/* [] END OF FILE */
