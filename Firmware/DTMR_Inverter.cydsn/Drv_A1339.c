/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

/* ========================================
 *
 * Driver minimo para o encoder Allegro A1339 (interface SPI primaria).
 * Ver Drv_A1339.h para o formato do protocolo.
 *
 * Observacoes de hardware:
 *  - O CS (SS) e gerado automaticamente pelo componente SPIM a cada
 *    palavra escrita com SPIM_1_WriteTxData(): CS desce, 16 clocks, CS sobe.
 *    Isso enquadra cada frame de 16 bits, como o A1339 exige.
 *  - O SPIM deve estar configurado em Modo 3 (CPOL=1, CPHA=1), MSB-first,
 *    16 bits de dado. Caso contrario o dado recebido vira corrompido.
 *
 * ========================================
*/

#include "Drv_A1339.h"
#include "project.h"
#include "FOC.h"

/******************************************************************************
* @brief       Executes ONE 16-bit SPI frame and returns the word received in
*              the same frame. Keeps the RX FIFO drained so the data read
*              always matches the frame just sent.
* @param[in]   ui16TxWord  Word to transmit
* @return      uint16_t    Word received in the same frame
******************************************************************************/
static uint16_t ui16DrvA1339_Transfer(uint16_t ui16TxWord)
{
    /* descarta qualquer dado antigo que tenha sobrado no FIFO de RX */
    while (0u != (SPIM_1_ReadRxStatus() & SPIM_1_STS_RX_FIFO_NOT_EMPTY))
    {
        (void)SPIM_1_ReadRxData();
    }

    SPIM_1_WriteTxData(ui16TxWord);

    /* espera o frame terminar: o byte de resposta chega ao FIFO de RX */
    while (0u == (SPIM_1_ReadRxStatus() & SPIM_1_STS_RX_FIFO_NOT_EMPTY))
    {
        /* aguardando */
    }

    return SPIM_1_ReadRxData();
}

/******************************************************************************
* @brief       Starts the SPI master and waits the sensor power-on time (tPO)
******************************************************************************/
void vDrvA1339_Init(void)
{
    SPIM_1_Start();

    /* tPO: o angulo so e valido apos o power-on completo do sensor
     * (~15-20 ms tipico, ate ~50 ms com auto-teste). Aguarda por seguranca. */
    CyDelay(50u);

    SPIM_1_ClearRxBuffer();
    SPIM_1_ClearTxBuffer();
}

/******************************************************************************
* @brief       Out-of-frame register read: frame 1 sends the address,
*              frame 2 clocks the content out
* @param[in]   ui8Addr   6-bit primary serial register address
* @return      uint16_t  Register content
******************************************************************************/
uint16_t ui16DrvA1339_ReadRegister(uint8_t ui8Addr)
{
    /* frame 1: requisicao (o MISO deste frame e "don't care") */
    (void)ui16DrvA1339_Transfer(A1339_READ_CMD(ui8Addr));

    /* frame 2: clocka a resposta para fora; o MISO traz o conteudo de addr.
     * Enviamos um read do registrador NULL (0x0000), que nao altera o sensor. */
    return ui16DrvA1339_Transfer(A1339_READ_CMD(A1339_REG_NULL));
}

/******************************************************************************
* @brief       Raw 12-bit mechanical angle (0..4095)
* @return      uint16_t  Angle field of register 0x20
******************************************************************************/
uint16_t ui16DrvA1339_ReadRawAngle(void)
{
    uint16_t ui16Reg = ui16DrvA1339_ReadRegister(A1339_REG_ANGLE_12BIT);
    return (uint16_t)(ui16Reg & A1339_ANG_ANGLE_MASK);
}

/* ----------------------------------------------------------------------------
 * Diagnostico. Cada funcao faz sua propria leitura out-of-frame completa
 * (ui16DrvA1339_ReadRegister), de modo que nao deixa estado pendente que possa
 * alterar a proxima leitura de angulo.
 * --------------------------------------------------------------------------*/

/******************************************************************************
* @brief       Sensor internal temperature [Celsius]
* @return      float  Temperature in degrees C
******************************************************************************/
float fDrvA1339_ReadTemperatureC(void)
{
    uint16_t ui16Reg = ui16DrvA1339_ReadRegister(A1339_REG_TEMPERATURE);
    uint16_t ui16N   = (uint16_t)(ui16Reg & A1339_DATA_MASK);  /* Kelvin codificado */
    /* Datasheet (familia A1335): n * (1/8) = Kelvin ; Celsius = K - 273.15.
     * Neste A1339 a saida vem com escala 10x em relacao ao valor real, por
     * isso o fator 0.1 (ex.: 238 -> 23.8 C, conferido em bancada).         */
    return ( ( (float)ui16N / 8.0f) - 273.15f ) * 0.1f;
}

/******************************************************************************
* @brief       Absolute magnetic field magnitude [gauss]
* @return      uint16_t  Field in G (1 G = 0.1 mT)
******************************************************************************/
uint16_t ui16DrvA1339_ReadFieldGauss(void)
{
    uint16_t ui16Reg = ui16DrvA1339_ReadRegister(A1339_REG_FIELD);
    return (uint16_t)(ui16Reg & A1339_DATA_MASK);  /* magnitude direta em G */
}

/******************************************************************************
* @brief       Raw rotor angle referenced to the stator (offset applied)
* @return      uint16_t  Angle, 12 bits (0..4095)
******************************************************************************/
uint16_t ui16DrvA1339_GetRawRotorAngle(void)
{
    uint16_t ui16Angle = ui16DrvA1339_ReadRawAngle();
    return (uint16_t)((ui16Angle + A1339_STATOR_OFFSET_RAW) & A1339_ANG_ANGLE_MASK);
}

#define A1339_CPR           4096
#define A1339_HALF_CPR      2048
#define A1339_SAMPLE_HZ     4000.0f
#define A1339_LSB_TO_RAD    (TWO_PI / 4096.0f)
#define A1339_LSB_TO_DEG    (360.0f / 4096.0f)
#define A1339_LSB_TO_RPM    (60.0f / 4096.0f)
#define A1339_SPEED_LPF_K   0.5f

static uint8_t     ui8MotionInit = 0u;
static uint16_t    ui16LastRaw   = 0u;
static int32_t     s32Position   = 0;
static xFOC_IirLpf xSpeedFilt    = { A1339_SPEED_LPF_K, 0.0f };

/******************************************************************************
* @brief       Multi-turn position/speed tracking step (call at A1339_SAMPLE_HZ)
* @param[in]   ui16Raw  Raw rotor angle, 12 bits
******************************************************************************/
void vDrvA1339_UpdateMotion(uint16_t ui16Raw)
{
    int32_t s32Delta;

    if (!ui8MotionInit)
    {
        ui16LastRaw = ui16Raw;
        ui8MotionInit = 1u;
        return;
    }

    s32Delta = (int32_t)ui16Raw - (int32_t)ui16LastRaw;
    if (s32Delta > A1339_HALF_CPR)        { s32Delta -= A1339_CPR; }
    else if (s32Delta < -A1339_HALF_CPR)  { s32Delta += A1339_CPR; }

    s32Position += s32Delta;
    ui16LastRaw = ui16Raw;

    (void)fFOC_IirLpfUpdate(&xSpeedFilt, (float)s32Delta * A1339_SAMPLE_HZ);
}

/******************************************************************************
* @brief       Multi-turn position accessors (LSB, rad, deg)
******************************************************************************/
int32_t s32DrvA1339_GetPositionLsb(void)
{
    return s32Position;
}

float fDrvA1339_GetPositionRad(void)
{
    return (float)s32Position * A1339_LSB_TO_RAD;
}

float fDrvA1339_GetPositionDeg(void)
{
    return (float)s32Position * A1339_LSB_TO_DEG;
}

/******************************************************************************
* @brief       Filtered speed accessors (LSB/s, rad/s, rpm)
******************************************************************************/
float fDrvA1339_GetSpeedLsbps(void)
{
    return xSpeedFilt.fY;
}

float fDrvA1339_GetSpeedRadps(void)
{
    return xSpeedFilt.fY * A1339_LSB_TO_RAD;
}

float fDrvA1339_GetSpeedRpm(void)
{
    return xSpeedFilt.fY * A1339_LSB_TO_RPM;
}

/* [] END OF FILE */
