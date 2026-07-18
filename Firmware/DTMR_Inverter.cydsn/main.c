/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

// Generated
#include "project.h"
// Std C
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
// Application libraries
#include "FOC.h"
#include "PID.h"
#include "Inverter_Utils.h"
#include "DutyMeas.h"
// Devices and peripheral drivers
#include "Drv_PWM.h"
#include "Drv_A1339.h"
#include "Drv_ADC.h"
#include "Drv_Timer.h"
#include "Drv_UART.h"

#define FOC_VBUS            12.0f
#define FOC_VQ_RANGE        12.0f
#define FOC_VQ_LIN          (FOC_VBUS * 0.80f)
#define POT_ADC_FULLSCALE   4095.0f
#define PWM_PERIOD_CNT      2000u
#define POT_LPF_K           0.20f

// ------- CONTROL MODE (select one) ------- //
//#define CONTROL_MODE_TORQUE
//#define CONTROL_MODE_SPEED
//#define CONTROL_MODE_POSITION
//#define TEST_MODE_TORQUE
//#define TEST_MODE_SPEED
//#define TEST_MODE_POSITION

static const uint8_t ui8Lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

/******************************************************************************
* @brief       Bit-reverses a byte via nibble lookup
* @param[in]   ui8N     Input byte
* @return      uint8_t  Bit-reversed byte
******************************************************************************/
uint8_t ui8Main_Reverse(uint8_t ui8N) {
   // Reverse the top and bottom nibble then swap them.
   return (ui8Lookup[ui8N & 0b1111] << 4) | ui8Lookup[ui8N >> 4];
}

// https://community.infineon.com/t5/PSOC-5-3-1/How-to-configure-and-use-SysTick-Timer-Interrupt-in-PSoC5/td-p/283211
static volatile uint32_t ui32Systick = 0;
CY_ISR(vMain_SysTick_ISR)
{
    ui32Systick++;
}

static volatile uint64_t ui64HftTick = 0;
static volatile uint64_t ui64LftTick = 0;

static int16 s16Ain5Raw;
static int16 s16Ain6Raw = 2048;
static int16 s16Ain7Raw = 2048;
static float fThrottlePotA = 0;
static float fThrottlePotB = 0;
static xFOC_IirLpf xPotFilt = { .fK = POT_LPF_K, .fY = 0.0f };

// current sensing
static int16 s16Ain0Raw = 0;
static int16 s16Ain1Raw = 0;
static int16 s16Ain2Raw = 0;
static bool bCurrentSenseReady = false;

// FOC state: currents, cascaded loops (current/speed/position) and PWM outputs
static xFOC_Ctrl xFoc =
{
    .xPiId    = { .fKp = FOC_PI_KP, .fKi = FOC_PI_KI, .fTs = FOC_PI_TS, .fOutMax = FOC_PI_VMAX, .fOutMin = -FOC_PI_VMAX },
    .xPiIq    = { .fKp = FOC_PI_KP, .fKi = FOC_PI_KI, .fTs = FOC_PI_TS, .fOutMax = FOC_PI_VMAX, .fOutMin = -FOC_PI_VMAX },
    .xPiSpeed = { .fKp = SPD_PI_KP, .fKi = SPD_PI_KI, .fKd = SPD_PI_KD, .fTs = SPD_PI_TS, .fOutMax = FOC_IQ_MAX_A, .fOutMin = -FOC_IQ_MAX_A },
    .xPdPos   = { .fKp = POS_PD_KP, .fKi = POS_PD_KI, .fKd = POS_PD_KD, .fTs = POS_PD_TS, .fOutMax = FOC_IQ_MAX_A, .fOutMin = -FOC_IQ_MAX_A }
};

static const float fSpeedKnobRangeRpm = 1000.0f;  // +/- 1000 rpm
static const float fPosKnobRangeDeg = 360.0f;     // +/- 1 rev.

// PWM out test (open loop)
static float fOpenLoopTheta = 0.0f;
static float fOpenLoopFreqHz = 2.0f;
static float fOpenLoopVMag = 0.2f;
static float fOpenLoopValpha = 0.0f;
static float fOpenLoopVbeta = 0.0f;

static uint32_t ui32LastExecTime = 0u;

// Encoder test
static uint16_t ui16A1339AngleRaw = 0;

/******************************************************************************
* @brief       High-frequency task (4 kHz): angle + current acquisition and
*              motor control (current loop / modulator)
******************************************************************************/
CY_ISR(vMain_HFT_ISR)
{
    // duty meas (start)
    //CR_Leds_Write(1u);
#if defined(MEAS_TASK_FOC_HFT_4K)
    vDrvTimer_Start();
#endif

    // HFT tick
    ui64HftTick++;

    // Angle acquisition
    ui16A1339AngleRaw = ui16DrvA1339_GetRawRotorAngle();
    vDrvA1339_UpdateMotion(ui16A1339AngleRaw);
    vFOC_RotorRawToSinCos(ui16A1339AngleRaw, &xFoc.fSin, &xFoc.fCos);
    vFOC_RotorRawToSinCosM60(ui16A1339AngleRaw, &xFoc.fSinM60, &xFoc.fCosM60);

    // Line current acquisition
    ADC_SAR_Seq_1_StartConvert();
    ADC_SAR_Seq_1_IsEndConversion(ADC_SAR_Seq_1_WAIT_FOR_RESULT);
    s16Ain0Raw = ADC_SAR_Seq_1_GetResult16(0);
    s16Ain1Raw = ADC_SAR_Seq_1_GetResult16(1);
    s16Ain2Raw = ADC_SAR_Seq_1_GetResult16(2);
    bCurrentSenseReady = bDrvAdc_RunCurrentSense(s16Ain0Raw, s16Ain1Raw); // Two-Shunt: U+V+W=0

    // Motor Control
    if (bCurrentSenseReady)
    {
        /*
        // OPEN LOOP (PWM outputs test)
        //fOpenLoopTheta = 0; // only for encoder offset measurement
        fOpenLoopTheta += (TWO_PI * fOpenLoopFreqHz) / 4000.0f;
        if(fOpenLoopTheta >= TWO_PI) fOpenLoopTheta -= TWO_PI;
        fOpenLoopValpha = fOpenLoopVMag * fFOC_FastCos(fOpenLoopTheta);
        fOpenLoopVbeta  = fOpenLoopVMag * fFOC_FastSin(fOpenLoopTheta);
        vFOC_Svpwm(fOpenLoopValpha, fOpenLoopVbeta, FOC_VBUS, PWM_PERIOD_CNT, &xFoc.ui32PwmU, &xFoc.ui32PwmV, &xFoc.ui32PwmW);
        bDrvPwm_UpdateInt(xFoc.ui32PwmU, xFoc.ui32PwmV, xFoc.ui32PwmW);
        */


        // Voltage control, potentiometer as V_q input
        xFoc.fVq = fThrottlePotA * 1.0f;
        vFOC_InverseParkTransform(0.0f, xFoc.fVq, xFoc.fSin, xFoc.fCos, &xFoc.fValpha, &xFoc.fVbeta);
        vFOC_Svpwm(xFoc.fValpha, xFoc.fVbeta, FOC_VBUS, PWM_PERIOD_CNT, &xFoc.ui32PwmU, &xFoc.ui32PwmV, &xFoc.ui32PwmW);
        bDrvPwm_UpdateInt(xFoc.ui32PwmU, xFoc.ui32PwmV, xFoc.ui32PwmW);


        /*
        // FOC
        xFoc.fIu = fDrvAdc_GetCurrentU();
        xFoc.fIv = fDrvAdc_GetCurrentV();
        xFoc.fIw = fDrvAdc_GetCurrentW();
        vFOC_ClarkeTransform(xFoc.fIu, xFoc.fIv, &xFoc.fIalpha, &xFoc.fIbeta);
        vFOC_ParkTransform(xFoc.fIalpha, xFoc.fIbeta, xFoc.fSinM60, xFoc.fCosM60, &xFoc.fId, &xFoc.fIq);

        // update Knob torque reference (test line)
        //xFoc.fIqRef = fThrottlePotA * FOC_IQ_MAX_A;

        // NEW - fIqRef is updated in the outer loop (LFT 1kHz)
        xFoc.fVd = fPID_PiControl(&xFoc.xPiId, 0.0f - xFoc.fId); // no flux weakening
        xFoc.fVq = fPID_PiControl(&xFoc.xPiIq, xFoc.fIqRef - xFoc.fIq);

        xFoc.fVmag = fFOC_FastSqrt(xFoc.fVd * xFoc.fVd + xFoc.fVq * xFoc.fVq);
        if (xFoc.fVmag > FOC_PI_VMAX)
        {
            xFoc.fVd *= (FOC_PI_VMAX / xFoc.fVmag);
            xFoc.fVq *= (FOC_PI_VMAX / xFoc.fVmag);
        }

        vFOC_InverseParkTransform(xFoc.fVd, xFoc.fVq, xFoc.fSin, xFoc.fCos, &xFoc.fValpha, &xFoc.fVbeta);
        vFOC_Svpwm(xFoc.fValpha, xFoc.fVbeta, FOC_VBUS, PWM_PERIOD_CNT, &xFoc.ui32PwmU, &xFoc.ui32PwmV, &xFoc.ui32PwmW);
        bDrvPwm_UpdateInt(xFoc.ui32PwmU, xFoc.ui32PwmV, xFoc.ui32PwmW);
        */

    }
    else
    {
        bDrvPwm_UpdateInt(PWM_PERIOD_CNT / 2u, PWM_PERIOD_CNT / 2u, PWM_PERIOD_CNT / 2u); // static 50% pwm
    }

    // duty meas (end)
    //CR_Leds_Write(0u);
#if defined(MEAS_TASK_FOC_HFT_4K)
    ui32LastExecTime = ui32DrvTimer_GetDiff();
    vDutyMeas_RegisterSample(ui32LastExecTime);
#endif

    // LED blink
    //CR_Leds_Write(((ui64HftTick % 0x200) < 0x40) ? 1:0);
}

/******************************************************************************
* @brief       Low-frequency task (1 kHz): knob acquisition and outer loops
*              (speed / position / test sequences) generating Iq reference
******************************************************************************/
CY_ISR(vMain_LFT_ISR)
{
    // LED duty meas (start)
    //CR_Leds_Write(1u);
#if defined(MEAS_TASK_FOC_LFT_1K)
    vDrvTimer_Start();
#endif

    // LFT tick (1kHz ISR)
    ui64LftTick++;

    // get potentiometer reading
    ADC_SAR_1_StartConvert();
    ADC_SAR_1_IsEndConversion(ADC_SAR_1_WAIT_FOR_RESULT);
    s16Ain7Raw = ADC_SAR_1_GetResult16(); // corner pot
    fThrottlePotA = (float)((int)s16Ain7Raw - (int)0x800) * 0.00048828125f;
    fThrottlePotA = fFOC_IirLpfUpdate(&xPotFilt, fThrottlePotA);

    if(bCurrentSenseReady)
    {
    #if defined(CONTROL_MODE_TORQUE)
        xFoc.fIqRef = fThrottlePotA * FOC_IQ_MAX_A;
    #elif defined(CONTROL_MODE_SPEED)
        xFoc.fSpeedTargetRpm = fThrottlePotA * fSpeedKnobRangeRpm;
        xFoc.fSpeedMeasRpm = fDrvA1339_GetSpeedRpm();
        xFoc.fSpeedIqRef = fPID_PidControl(&xFoc.xPiSpeed, xFoc.fSpeedTargetRpm - xFoc.fSpeedMeasRpm);
        xFoc.fIqRef = xFoc.fSpeedIqRef;
    #elif defined(CONTROL_MODE_POSITION)
        xFoc.fPosTargetDeg = fThrottlePotA * fPosKnobRangeDeg;
        xFoc.fPosMeasDeg = fDrvA1339_GetPositionDeg();
        xFoc.fPosIqRef = fPID_PidControl(&xFoc.xPdPos, xFoc.fPosTargetDeg - xFoc.fPosMeasDeg);
        xFoc.fIqRef = xFoc.fPosIqRef;
    #elif defined(TEST_MODE_TORQUE)
        xFoc.fIqRef = fInverterUtils_TestSeqTorqueIq(ui64LftTick, fThrottlePotA);
    #elif defined(TEST_MODE_SPEED)
        xFoc.fSpeedTargetRpm = fInverterUtils_TestSeqSpeedRpm(ui64LftTick, fThrottlePotA);
        xFoc.fSpeedMeasRpm = fDrvA1339_GetSpeedRpm();
        xFoc.fSpeedIqRef = fPID_PidControl(&xFoc.xPiSpeed, xFoc.fSpeedTargetRpm - xFoc.fSpeedMeasRpm);
        xFoc.fIqRef = xFoc.fSpeedIqRef;
    #elif defined(TEST_MODE_POSITION)
        xFoc.fPosTargetDeg = fInverterUtils_TestSeqPositionDeg(ui64LftTick, fThrottlePotA);
        xFoc.fPosMeasDeg = fDrvA1339_GetPositionDeg();
        xFoc.fPosIqRef = fPID_PidControl(&xFoc.xPdPos, xFoc.fPosTargetDeg - xFoc.fPosMeasDeg);
        xFoc.fIqRef = xFoc.fPosIqRef;
    #endif
    }

    // LED duty meas (end)
    //CR_Leds_Write(0u);
#if defined(MEAS_TASK_FOC_LFT_1K)
    ui32LastExecTime = ui32DrvTimer_GetDiff();
    vDutyMeas_RegisterSample(ui32LastExecTime);
#endif
}

#if defined(TEST_MODE_TORQUE) || defined(TEST_MODE_SPEED) || defined(TEST_MODE_POSITION)
/******************************************************************************
* @brief       Streams ref/meas pairs over UART while a test sequence runs
*              (call from main loop)
******************************************************************************/
static void vMain_TestPrintProcess(void)
{
    static bool bPrinting = false;
    static uint32_t ui32T0 = 0u;
    char cMsg[64];
    bool bRun = bInverterUtils_TestSeqIsRunning();

    if (!bPrinting)
    {
        if (bRun)
        {
            bPrinting = true;
            ui32T0 = (uint32_t)ui64LftTick;
        #if defined(TEST_MODE_TORQUE)
            vDrvUART_PutString("#TEST_BEGIN mode=TORQUE unit=mA\r\n");
        #elif defined(TEST_MODE_SPEED)
            vDrvUART_PutString("#TEST_BEGIN mode=SPEED unit=rpm\r\n");
        #else
            vDrvUART_PutString("#TEST_BEGIN mode=POSITION unit=deg\r\n");
        #endif
        }
        return;
    }

#if defined(TEST_MODE_TORQUE)
    int s32Ref  = (int)(xFoc.fIqRef * 1000.0f);
    int s32Meas = (int)(xFoc.fIq * 1000.0f);
#elif defined(TEST_MODE_SPEED)
    int s32Ref  = (int)xFoc.fSpeedTargetRpm;
    int s32Meas = (int)xFoc.fSpeedMeasRpm;
#else
    int s32Ref  = (int)xFoc.fPosTargetDeg;
    int s32Meas = (int)xFoc.fPosMeasDeg;
#endif
    sprintf(cMsg, "%lu;%d;%d\r\n", (unsigned long)((uint32_t)ui64LftTick - ui32T0),
            s32Ref, s32Meas);
    vDrvUART_PutString(cMsg);

    if (!bRun)
    {
        bPrinting = false;
        vDrvUART_PutString("#TEST_END\r\n");
    }
}
#endif

/******************************************************************************
* @brief       Init (drivers, LUTs, ISRs, PID tuning) and main debug loop
* @return      int  Never returns
******************************************************************************/
int main(void)
{
    vDrvUART_Init();
    ADC_SAR_1_Start();
    ADC_SAR_Seq_1_Start();
    vDrvTimer_Init();
    vDutyMeas_Init();

    // TODO write hdriver for platform
    vFOC_InitTrigLut();

    // Set SysTick
    CySysTickStart();                           // Start the systick
    CySysTickSetReload(cydelay_freq_hz/1000u);
    CySysTickSetCallback(0, vMain_SysTick_ISR); // Add the Systick callback

    // Set HFT ISR
    HFT_ISR_StartEx(vMain_HFT_ISR);
    // Set LFT ISR
    LFT_ISR_StartEx(vMain_LFT_ISR);

    // General Debug
    char cDebugMsg[128];

    vDrvPwm_Init(); // TODO place PWM init inside HFT

    vPID_SetDFilterFc(&xFoc.xPiSpeed, SPD_D_FC_HZ);
    vPID_SetMaxD(&xFoc.xPiSpeed, SPD_D_MAX);
    vPID_SetDFilterFc(&xFoc.xPdPos, POS_D_FC_HZ);
    vPID_SetMaxD(&xFoc.xPdPos, POS_D_MAX);
    //vPID_SetDeadband(&xFoc.xPdPos, POS_DEADBAND_DEG);

    vDrvA1339_Init();   /* SPIM_1_Start() + espera o power-on (tPO) do A1339 */

    // Enable global interrupts.
    CyGlobalIntEnable;

    // timing
    uint64_t ui64LastLoop = 0;
    uint64_t ui64LoopCount = 0;
    const uint64_t ui64LoopPeriod = 5; // ms

    for(;;)
    {
        if((ui32Systick - ui64LastLoop) >= ui64LoopPeriod)
        {
            ui64LastLoop = ui32Systick;
            // ---------------------------------------------- main loop start ---------------------------------------------- //
            /*
            // PRINT ENCODER ANGLE
            uint16_t ui16AngleDeg = (uint16_t)(((uint32_t)ui16A1339AngleRaw * 360u) / A1339_ANGLE_FULLSCALE);    // 0..359
            sprintf(cDebugMsg, "raw:%u  deg:%u\r\n", ui16A1339AngleRaw, ui16AngleDeg);
            vDrvUART_PutString(cDebugMsg);
            */

            /*
            // print electrical vs encoder angle
            uint16_t ui16AngleDeg = (uint16_t)(((uint32_t)ui16A1339AngleRaw * 360u) / A1339_ANGLE_FULLSCALE);    // 0..359
            uint16_t ui16ThetaDeg = (uint16_t)(fOpenLoopTheta * (360.0f/TWO_PI));
            sprintf(cDebugMsg, "> angle_mech_raw:%u, angle_ele:%u\r\n", ui16AngleDeg, ui16ThetaDeg);
            vDrvUART_PutString(cDebugMsg);
            */

            /*
            // print openloop AlBe vs requested AlBe
            int s32AlX100 = (int)(fOpenLoopValpha * 100.0f);
            int s32BeX100 = (int)(fOpenLoopVbeta * 100.0f);
            int s32AlReqX100 = (int)(xFoc.fValpha * 100.0f);
            int s32BeReqX100 = (int)(xFoc.fVbeta * 100.0f);
            sprintf(cDebugMsg, "> Al:%d, Be:%d, AlReq:%d, BeReq:%d\r\n", s32AlX100, s32BeX100, s32AlReqX100, s32BeReqX100);
            vDrvUART_PutString(cDebugMsg);
            */


            // print multi-turn position and speed
            int s32SpeedRpm = (int)(fDrvA1339_GetSpeedRpm());
            int s32PosMultiturnDeg = (int)(fDrvA1339_GetPositionDeg());
            sprintf(cDebugMsg, "> zero:0, speed:%d, pos:%d\r\n", s32SpeedRpm, s32PosMultiturnDeg);
            vDrvUART_PutString(cDebugMsg);


            /*
            // print current sense
            sprintf(cDebugMsg, "> IU:%d, IV:%d, IW:%d \r\n", s16Ain0Raw, s16Ain1Raw, s16Ain2Raw);
            vDrvUART_PutString(cDebugMsg);
            */

            /*
            int s32IuX1000 = (int)(xFoc.fIu * 1000.0f);
            int s32IvX1000 = (int)(xFoc.fIv * 1000.0f);
            int s32IwX1000 = (int)(xFoc.fIw * 1000.0f);
            sprintf(cDebugMsg, "> IU:%d, IV:%d, IW:%d \r\n", s32IuX1000, s32IvX1000, s32IwX1000);
            vDrvUART_PutString(cDebugMsg);
            */

            /*
            // print dq currents and requested AlBe voltages
            int s32AlX100 = (int)(xFoc.fValpha * 100.0f);
            int s32BeX100 = (int)(xFoc.fVbeta * 100.0f);
            int s32DX100 = (int)(xFoc.fId * 100.0f);
            int s32QX100 = (int)(xFoc.fIq * 100.0f);
            sprintf(cDebugMsg, "> zero:0, d:%d, q:%d, al:%d, be:%d \r\n", s32DX100, s32QX100, s32AlX100, s32BeX100);
            vDrvUART_PutString(cDebugMsg);
            */


#if defined(CONTROL_MODE_TORQUE)
            // testing current loops
            int s32QRefX100 = (int)(xFoc.fIqRef * 100.0f);
            int s32DX100 = (int)(xFoc.fId * 100.0f);
            int s32QX100 = (int)(xFoc.fIq * 100.0f);
            sprintf(cDebugMsg, "> zero:0, IqRef:%d, Id:%d, Iq:%d\r\n", s32QRefX100, s32DX100, s32QX100);
            vDrvUART_PutString(cDebugMsg);
#elif defined(CONTROL_MODE_SPEED)
            // testing speed loop
            int s32SpeedRef = (int)(xFoc.fSpeedTargetRpm);
            int s32SpeedMeas = (int)(xFoc.fSpeedMeasRpm);
            sprintf(cDebugMsg, "> zero:0, SpeedRef:%d, SpeedMeas:%d\r\n", s32SpeedRef, s32SpeedMeas);
            vDrvUART_PutString(cDebugMsg);
#elif defined(CONTROL_MODE_POSITION)
            // testing position loop
            int s32PosRef = (int)(xFoc.fPosTargetDeg);
            int s32PosMeas = (int)(xFoc.fPosMeasDeg);
            sprintf(cDebugMsg, "> zero:0, PosRef:%d, PosMeas:%d\r\n", s32PosRef, s32PosMeas);
            vDrvUART_PutString(cDebugMsg);
#endif

#if defined(TEST_MODE_TORQUE) || defined(TEST_MODE_SPEED) || defined(TEST_MODE_POSITION)
            vMain_TestPrintProcess();
#endif

            /*
            // print histogram
            vDutyMeas_Process();
            */

            /*
            // get aux adc samples
            ADC_SAR_Seq_1_StartConvert();
            ADC_SAR_Seq_1_IsEndConversion(ADC_SAR_Seq_1_WAIT_FOR_RESULT);
            s16Ain5Raw = ADC_SAR_Seq_1_GetResult16(0); // Gets sample from MUX input 0
            s16Ain6Raw = ADC_SAR_Seq_1_GetResult16(1); // POT mid
            s16Ain7Raw = ADC_SAR_Seq_1_GetResult16(2); // POT corner
            // convert
            fThrottlePotA = (float)((int)s16Ain7Raw - (int)0x800) * 0.00048828125f;
            fThrottlePotB = (float)((int)s16Ain6Raw - (int)0x800) * 0.00048828125f;
            // debug raw auxiliary ADC samples
            //sprintf(cDebugMsg, "> A5:%u, A6:%u, A7:%u\r\n", s16Ain5Raw, s16Ain6Raw, s16Ain7Raw);
            //vDrvUART_PutString(cDebugMsg);
            // debug float
            //int s32ThrottleX100 = (int)(fThrottlePotB * 100.0f);
            //sprintf(cDebugMsg, "> Pot: %d \r\n", s32ThrottleX100);
            //vDrvUART_PutString(cDebugMsg);
            */

            // ---------------------------------------------- main loop end ---------------------------------------------- //
            ui64LoopCount++;
        }
        CyDelay(1);
    }
}

// ------------------------- tests done on the main loop: ------------------------- //
            // PRINT TICKS
            //sprintf(cDebugMsg, "[%04d] HFT tick = %d\r\n", (int)ui64LoopCount, (int)ui64HftTick);
            //sprintf(cDebugMsg, "[%04d] systick = %d\r\n", (int)ui64LoopCount, (int)ui32Systick);
            //vDrvUART_PutString(cDebugMsg);


                /*
                int s32TempC = (int)fDrvA1339_ReadTemperatureC();
                uint16_t ui16Field = ui16DrvA1339_ReadFieldGauss();
                sprintf(cDebugMsg, "  [diag] temp:%dC  campo:%uG\r\n",
                        s32TempC, ui16Field);
                vDrvUART_PutString(cDebugMsg);
                */

                /*
                // A1339 angle acquisition (registrador 0x20: 12 bits + status)
                uint16_t ui16AngReg   = ui16DrvA1339_ReadRegister(A1339_REG_ANGLE_12BIT);
                uint16_t ui16RawAngle = ui16AngReg & A1339_ANG_ANGLE_MASK;               // 0..4095
                uint16_t ui16AngleDeg = (uint16_t)(((uint32_t)ui16RawAngle * 360u)
                                                   / A1339_ANGLE_FULLSCALE);             // 0..359

                sprintf(cDebugMsg, "raw:%u  deg:%u  EF:%u UV:%u\r\n",
                        ui16RawAngle, ui16AngleDeg,
                        (ui16AngReg & A1339_ANG_EF_MASK) ? 1u : 0u,
                        (ui16AngReg & A1339_ANG_UV_MASK) ? 1u : 0u);
                vDrvUART_PutString(cDebugMsg);
                */



/* [] END OF FILE */
