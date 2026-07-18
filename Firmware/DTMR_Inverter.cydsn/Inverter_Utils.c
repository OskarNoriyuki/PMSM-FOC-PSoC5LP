/* ========================================
 * Copyright (c) 2026 oskarnoriyuki
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 * ========================================
*/

#include "Inverter_Utils.h"

#define TEST_TRIGGER_LEVEL  0.5f
#define TEST_RELEASE_LEVEL  0.1f
#define TEST_STEPS          (4u * TEST_CYCLES)
#define TEST_SETTLE_STEPS   1u

typedef struct
{
    bool     bArmed;
    bool     bRunning;
    uint64_t ui64Start;
} xTestSeq;

static volatile bool bTestRunning = false;

/******************************************************************************
* @brief       Step sequence generator: 0 -> +A -> 0 -> -A, TEST_CYCLES times,
*              one step per TEST_INTERVAL; armed/triggered by the knob
* @param[in]   ui64Tick    LFT tick [ms]
* @param[in]   fKnob       Knob input [-1..1]; > 0.5 triggers, < 0.1 re-arms
* @param[in]   fAmp        Step amplitude
* @param[in]   pxS         Sequence state
* @return      float       Current reference output
******************************************************************************/
static float fInverterUtils_RunSequence(uint64_t ui64Tick, float fKnob, float fAmp, xTestSeq *pxS)
{
    float fOut = 0.0f;

    if (pxS->bRunning)
    {
        uint32_t ui32Step = (uint32_t)((ui64Tick - pxS->ui64Start) / TEST_INTERVAL);
        if (ui32Step >= (TEST_STEPS + TEST_SETTLE_STEPS))
        {
            pxS->bRunning = false;
        }
        else
        {
            uint32_t ui32Phase = ui32Step & 3u;
            if      (ui32Phase == 1u) { fOut =  fAmp; }
            else if (ui32Phase == 3u) { fOut = -fAmp; }
        }
    }
    else if (pxS->bArmed && (fKnob > TEST_TRIGGER_LEVEL))
    {
        pxS->bRunning  = true;
        pxS->bArmed    = false;
        pxS->ui64Start = ui64Tick;
    }
    else if (fKnob < TEST_RELEASE_LEVEL)
    {
        pxS->bArmed = true;
    }

    bTestRunning = pxS->bRunning;
    return fOut;
}

/******************************************************************************
* @brief       true while a test sequence is being generated
******************************************************************************/
bool bInverterUtils_TestSeqIsRunning(void)
{
    return bTestRunning;
}

/******************************************************************************
* @brief       Torque (Iq) step test sequence [A]
******************************************************************************/
float fInverterUtils_TestSeqTorqueIq(uint64_t ui64Tick, float fKnobInput)
{
    static xTestSeq xS = { false, false, 0u };
    return fInverterUtils_RunSequence(ui64Tick, fKnobInput, TEST_AMPLITUDE_TORQUE, &xS);
}

/******************************************************************************
* @brief       Speed step test sequence [rpm]
******************************************************************************/
float fInverterUtils_TestSeqSpeedRpm(uint64_t ui64Tick, float fKnobInput)
{
    static xTestSeq xS = { false, false, 0u };
    return fInverterUtils_RunSequence(ui64Tick, fKnobInput, TEST_AMPLITUDE_SPEED, &xS);
}

/******************************************************************************
* @brief       Position step test sequence [deg]
******************************************************************************/
float fInverterUtils_TestSeqPositionDeg(uint64_t ui64Tick, float fKnobInput)
{
    static xTestSeq xS = { false, false, 0u };
    return fInverterUtils_RunSequence(ui64Tick, fKnobInput, TEST_AMPLITUDE_POSITION, &xS);
}

/* [] END OF FILE */
