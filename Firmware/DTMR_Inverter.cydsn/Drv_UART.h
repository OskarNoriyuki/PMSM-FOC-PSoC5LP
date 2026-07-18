/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/
    
#ifndef DRV_UART_H
#define DRV_UART_H

void vDrvUART_Init(void);
void vDrvUART_PutString(const char *pcStr);
void vDrvUART_PutChar(char cCh);

#endif /* DRV_UART_H */
