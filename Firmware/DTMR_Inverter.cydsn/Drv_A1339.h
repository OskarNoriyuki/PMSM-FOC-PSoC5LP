/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

/* ========================================
 *
 * Driver minimo para o encoder Allegro A1339
 * Interface SPI primaria (registradores seriais)
 *
 * SPI: Modo 3 (CPOL=1, CPHA=1), MSB-first, palavra de 16 bits.
 *      O A1339 usa leitura "out-of-frame": cada leitura precisa de
 *      2 frames -> frame 1 envia o endereco, frame 2 recebe o dado.
 *
 * ========================================
*/

#ifndef _DRV_A1339_H_
#define _DRV_A1339_H_

#include "cytypes.h"
#include <stdint.h>

/* ---- Registradores seriais primarios do A1339 ---- */
#define A1339_REG_NULL          (0x00u)  /* le sempre 0x0000                   */
#define A1339_REG_ANGLE_12BIT   (0x20u)  /* ANG: angulo de 12 bits + status    */
#define A1339_REG_ANGLE_15BIT   (0x32u)  /* angulo de 15 bits (alta resolucao) */
#define A1339_REG_TEMPERATURE   (0x28u)  /* TSEN: temperatura (Kelvin codif.)  */
#define A1339_REG_FIELD         (0x2Au)  /* FIELD: intensidade de campo (gauss)*/

/* Monta o comando de LEITURA de 16 bits para a interface serial primaria.
 *   bit15      = 0
 *   bit14      = R/W  (0 = leitura, 1 = escrita)
 *   bits[13:8] = endereco de 6 bits
 *   bits[7:0]  = dado (0 na leitura)
 * Ex.: ler 0x20  ->  (0x20 << 8) = 0x2000                                     */
#define A1339_READ_CMD(addr)    ((uint16)(((uint16)(addr) & 0x3Fu) << 8u))

/* ---- Campos retornados no pacote do registrador 0x20 (12 bits) ----
 *   bit15      = 0
 *   bit14      = EF  (flag de erro geral)
 *   bit13      = UV  (subtensao)
 *   bit12      = P   (paridade)
 *   bits[11:0] = ANGLE[11:0]                                                   */
#define A1339_ANG_EF_MASK       (0x4000u)
#define A1339_ANG_UV_MASK       (0x2000u)
#define A1339_ANG_PARITY_MASK   (0x1000u)
#define A1339_ANG_ANGLE_MASK    (0x0FFFu)

#define A1339_ANGLE_FULLSCALE   (4096u)   /* 12 bits -> 360 graus              */

/* ---- Campos comuns aos registradores TSEN (0x28) e FIELD (0x2A) ----
 * bits[15:12] = codigo identificador do registrador (so leitura/validacao)
 * bits[11:0]  = dado (Kelvin codificado para TSEN, gauss para FIELD)          */
#define A1339_DATA_MASK         (0x0FFFu) /* bits[11:0]: dado                  */
#define A1339_ID_MASK           (0xF000u) /* bits[15:12]: identificador        */
#define A1339_ID_TEMPERATURE    (0xF000u) /* TSEN  -> 1111                     */
#define A1339_ID_FIELD          (0xE000u) /* FIELD -> 1110                     */

/* Conversão */
#define A1339_STATOR_OFFSET_RAW (288U)

/* Inicializa o periferico SPI e aguarda o tempo de power-on (tPO) do sensor. */
void     vDrvA1339_Init(void);

/* Leitura generica de um registrador serial primario (16 bits). */
uint16_t ui16DrvA1339_ReadRegister(uint8_t ui8Addr);

/* Retorna o angulo bruto de 12 bits (0 .. 4095). */
uint16_t ui16DrvA1339_ReadRawAngle(void);

/* ---- Diagnostico (nao interfere na leitura de angulo) ----
 * Cada funcao e uma transacao SPI completa e independente. */

/* Temperatura interna do sensor, em graus Celsius. */
float    fDrvA1339_ReadTemperatureC(void);

/* Intensidade absoluta do campo magnetico, em gauss (1 G = 0.1 mT). */
uint16_t ui16DrvA1339_ReadFieldGauss(void);

uint16_t ui16DrvA1339_GetRawRotorAngle(void);

void     vDrvA1339_UpdateMotion(uint16_t ui16Raw);

int32_t  s32DrvA1339_GetPositionLsb(void);
float    fDrvA1339_GetPositionRad(void);
float    fDrvA1339_GetPositionDeg(void);

float    fDrvA1339_GetSpeedLsbps(void);
float    fDrvA1339_GetSpeedRadps(void);
float    fDrvA1339_GetSpeedRpm(void);

#endif /* _DRV_A1339_H_ */

/* [] END OF FILE */
