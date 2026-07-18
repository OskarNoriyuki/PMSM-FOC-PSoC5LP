/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#include "Drv_UART.h"
#include "project.h"

/******************************************************************************
* @brief       Starts the UART block
******************************************************************************/
void vDrvUART_Init(void)
{
    UART_1_Start();
}

/******************************************************************************
* @brief       Blocking transmit of a null-terminated string
* @param[in]   pcStr  String to send
******************************************************************************/
void vDrvUART_PutString(const char *pcStr)
{
    UART_1_PutString((const char8 *)pcStr);
}

/******************************************************************************
* @brief       Blocking transmit of a single character
* @param[in]   cCh  Character to send
******************************************************************************/
void vDrvUART_PutChar(char cCh)
{
    UART_1_PutChar((char8)cCh);
}
