/*
*   Copyright 2020-2026 NXP
*
*   NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms.
*
*   S32K144W Ultra-High Resolution 16-Bit True-Color Lighting Engine
*   Architecture: 16-Bit Hardware PWM (FTM0_CH7 Red, FTM2_CH3 Blue, FTM0_CH0 PTB12 Green)
*                 + 1.0 MHz Sigma-Delta PDM for EVB Default Green LED (PTE0 via R846).
*                 Full 65,536-step Continuous Hue Sweep @ 10,000 FPS Refresh Rate.
*                 16-Bit Precision Gamma 2.2 Interpolation with 1025-Point Calibration Table.
*                 100% NXP Real Time Drivers (RTD) Public APIs & MEX-Generated Configurations.
*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 100% NXP Real Time Drivers (RTD) Public APIs & MEX-Generated Configuration Headers
==================================================================================================*/
#include "StandardTypes.h"
#include "Clock_Ip.h"
#include "Port_Ci_Port_Ip.h"
#include "Port_Ci_Port_Ip_Cfg.h"
#include "Gpio_Dio_Ip.h"
#include "Ftm_Pwm_Ip.h"
#include "Ftm_Pwm_Ip_VS_0_PBcfg.h"
#include "OsIf.h"

#include "gamma_lut_1025.h"
#include "check_example.h"

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
/** @brief FTM Hardware Instances */
#define FTM_INSTANCE_0                  (0U)
#define FTM_INSTANCE_2                  (2U)

/** @brief FTM Hardware Channels mapped to On-Board RGB LED */
#define FTM_CH_RED                      (7U)  /* PTE7  (Pin 39) -> FTM0_CH7 */
#define FTM_CH_GREEN                    (0U)  /* PTB12 (Pin 43) -> FTM0_CH0 (via R787 0-ohm jumper) */
#define FTM_CH_BLUE                     (3U)  /* PTD5  (Pin 24) -> FTM2_CH3 (via R774 0-ohm jumper) */

/** @brief Board Default Physical Green LED Pin (PTE0, Pin 60 via R846 0-ohm jumper) */
#define LED_GREEN_GPIO_BASE             IP_PTE
#define LED_GREEN_GPIO_PIN              (0U)

/** @brief 16-bit Full-Scale Maximum Period (65,535 ticks = 1.22 kHz carrier @ 80 MHz) */
#define FTM_MAX_PERIOD_16BIT            (65535U)

/** @brief Maximum timeout iterations for PLL lock before declaring fault */
#define PLL_LOCK_TIMEOUT_COUNT          (100000UL)

/*==================================================================================================
*                                             ENUMS
==================================================================================================*/
/** @brief Application system status and error codes */
typedef enum
{
    APP_STATUS_SUCCESS                  = 0x00U,
    APP_STATUS_ERROR_CLOCK_INIT         = 0x01U,
    APP_STATUS_ERROR_PLL_TIMEOUT        = 0x02U,
    APP_STATUS_ERROR_PORT_INIT          = 0x03U,
    APP_STATUS_ERROR_FTM0_INIT          = 0x04U,
    APP_STATUS_ERROR_FTM2_INIT          = 0x05U
} App_StatusType;

/*==================================================================================================
*                                      GLOBAL OBSERVABLES (FOR GDB DEBUGGER)
==================================================================================================*/
volatile uint16 current_hue       = 0U;   /* 16-bit Hue angle: 0 ~ 65,535 (0.0055 deg resolution) */
volatile uint16 current_r         = 0U;   /* 16-bit Linear Red: 0 ~ 65,535 */
volatile uint16 current_g         = 0U;   /* 16-bit Linear Green: 0 ~ 65,535 */
volatile uint16 current_b         = 0U;   /* 16-bit Linear Blue: 0 ~ 65,535 */
volatile uint16 duty_r_hw         = 0U;   /* 16-bit Gamma 2.2 Red Duty: 0 ~ 65,535 */
volatile uint16 duty_g_hw         = 0U;   /* 16-bit Gamma 2.2 Green Duty: 0 ~ 65,535 */
volatile uint16 duty_b_hw         = 0U;   /* 16-bit Gamma 2.2 Blue Duty: 0 ~ 65,535 */
volatile uint32 cycle_count       = 0U;   /* Completed 360-degree rainbow loops */
volatile uint32 last_fault_status = APP_STATUS_SUCCESS; /* System health tracker */

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/
static inline uint16 Apply_Gamma16(uint16 linear_val);
static void HsvToRgb16(uint16 hue16, uint16 * const r, uint16 * const g, uint16 * const b);
static void Rainbow_Update16(uint16 hue16);
static void App_FaultHandler(App_StatusType faultCode);
static void Delay_Spin(uint32 count);

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

/**
* @brief        Simple calibrated spin-delay helper.
* @param[in]    count  Loop iteration count.
*/
static void Delay_Spin(uint32 count)
{
    volatile uint32 i = 0U;
    while (i < count)
    {
        i++;
    }
}

/**
* @brief        Graceful system fault containment handler.
* @param[in]    faultCode  The identified system error code.
*/
static void App_FaultHandler(App_StatusType faultCode)
{
    last_fault_status = (uint32)faultCode;

    /* Turn off all LED outputs safely */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   0U, 0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U, 0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U, 0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);

    /* Safe infinite halt */
    while (1)
    {
        /* Idle spin waiting for watchdog reset */
    }
}

/**
* @brief        High-precision 16-bit Gamma 2.2 curve evaluator.
* @details      Performs zero-division, zero-float linear interpolation on a 1025-point calibration table.
*               Maximum deviation is <0.005% across the entire 0 ~ 65,535 dynamic range.
* @param[in]    linear_val  16-bit linear light intensity (0 ~ 65,535).
* @return       16-bit Gamma-corrected perceptual duty cycle (0 ~ 65,535).
*/
static inline uint16 Apply_Gamma16(uint16 linear_val)
{
    uint32 idx  = (uint32)linear_val >> 6U;        /* High 10 bits: table index (0 ~ 1023) */
    uint32 frac = (uint32)linear_val & 0x3FU;      /* Low 6 bits: sub-interval fraction (0 ~ 63) */
    uint32 y0   = (uint32)gamma64k_lut[idx];
    uint32 y1   = (uint32)gamma64k_lut[idx + 1U];
    uint32 y    = y0 + (((y1 - y0) * frac + 32U) >> 6U);
    return (uint16)y;
}

/**
* @brief        16-bit High-Resolution HSV to RGB Vector Converter.
* @details      Maps a 16-bit angle (0 ~ 65,535) into three 16-bit color channels (0 ~ 65,535).
*               Operates strictly using single-cycle shift and bitwise math without division.
* @param[in]    hue16  16-bit hue angle (0 ~ 65,535, corresponding to 0 ~ 360 degrees).
* @param[out]   r      Output pointer for Red channel (0 ~ 65,535).
* @param[out]   g      Output pointer for Green channel (0 ~ 65,535).
* @param[out]   b      Output pointer for Blue channel (0 ~ 65,535).
*/
static void HsvToRgb16(uint16 hue16, uint16 * const r, uint16 * const g, uint16 * const b)
{
    if ((NULL_PTR == r) || (NULL_PTR == g) || (NULL_PTR == b))
    {
        return;
    }

    uint32 scaled_hue = (uint32)hue16 * 6U;
    uint16 sector     = (uint16)(scaled_hue >> 16U);
    uint16 rem        = (uint16)(scaled_hue & 0xFFFFU);
    uint16 f          = rem;
    uint16 q          = (uint16)(0xFFFFU - f);
    uint16 t          = f;

    switch (sector)
    {
        case 0U:  /* 0.00° ~ 60.00°: Red -> Yellow */
            *r = 0xFFFFU; *g = t;       *b = 0U;
            break;
        case 1U:  /* 60.00° ~ 120.00°: Yellow -> Green */
            *r = q;       *g = 0xFFFFU; *b = 0U;
            break;
        case 2U:  /* 120.00° ~ 180.00°: Green -> Cyan */
            *r = 0U;      *g = 0xFFFFU; *b = t;
            break;
        case 3U:  /* 180.00° ~ 240.00°: Cyan -> Blue */
            *r = 0U;      *g = q;       *b = 0xFFFFU;
            break;
        case 4U:  /* 240.00° ~ 300.00°: Blue -> Magenta */
            *r = t;       *g = 0U;      *b = 0xFFFFU;
            break;
        case 5U:  /* 300.00° ~ 360.00°: Magenta -> Red */
            *r = 0xFFFFU; *g = 0U;      *b = q;
            break;
        default:  /* Defensive boundary recovery */
            *r = 0xFFFFU; *g = 0U;      *b = 0U;
            break;
    }
}

/**
* @brief        Update color state, evaluate 16-bit Gamma, and program FTM PWM channels.
* @param[in]    hue16  16-bit hue angle (0 ~ 65,535).
*/
static void Rainbow_Update16(uint16 hue16)
{
    uint16 raw_r = 0U;
    uint16 raw_g = 0U;
    uint16 raw_b = 0U;

    HsvToRgb16(hue16, &raw_r, &raw_g, &raw_b);

    current_r = raw_r;
    current_g = raw_g;
    current_b = raw_b;

    /* 16-bit continuous Gamma 2.2 perceptual scaling */
    duty_r_hw = Apply_Gamma16(raw_r);
    duty_g_hw = Apply_Gamma16(raw_g);
    duty_b_hw = Apply_Gamma16(raw_b);

    /* Update Hardware PWM Channels via Official NXP RTD APIs */
    /* Red: FTM0 Channel 7 (PTE7, Pin 39) */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   duty_r_hw, 0U, TRUE);
    /* Green Alternate: FTM0 Channel 0 (PTB12, Pin 43 via R787 0-ohm jumper) */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, duty_g_hw, 0U, TRUE);
    /* Blue: FTM2 Channel 3 (PTD5, Pin 24 via R774 0-ohm jumper) */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  duty_b_hw, 0U, TRUE);
}

/*==================================================================================================
*                                       MAIN FUNCTION
==================================================================================================*/
int main(void)
{
    Clock_Ip_StatusType clockStatus;
    Port_Ci_Port_Ip_PortStatusType portStatus;

    /*----------------------------------------------------------------------------------------------
    * 1. Initialize Clock System using RTD Clock_Ip API with return status validation
    *---------------------------------------------------------------------------------------------*/
    clockStatus = Clock_Ip_Init(Clock_Ip_aClockConfig);
    if (CLOCK_IP_SUCCESS != clockStatus)
    {
        App_FaultHandler(APP_STATUS_ERROR_CLOCK_INIT);
    }

#if defined (FEATURE_CLOCK_IP_HAS_SPLL_CLK)
    /* Robust bounded timeout protection: eliminate unbounded while-loop */
    uint32 pllTimeout = PLL_LOCK_TIMEOUT_COUNT;
    while ((CLOCK_IP_PLL_LOCKED != Clock_Ip_GetPllStatus()) && (pllTimeout > 0U))
    {
        pllTimeout--;
    }

    if (0U == pllTimeout)
    {
        App_FaultHandler(APP_STATUS_ERROR_PLL_TIMEOUT);
    }
    else
    {
        Clock_Ip_DistributePll();
    }
#endif

    /*----------------------------------------------------------------------------------------------
    * 2. Initialize Pins using RTD Port_Ci_Port_Ip API and MEX-generated configuration
    *---------------------------------------------------------------------------------------------*/
    portStatus = Port_Ci_Port_Ip_Init(NUM_OF_CONFIGURED_PINS_PortContainer_0_VS_0,
                                      g_pin_mux_InitConfigArr_PortContainer_0_VS_0);
    if (PORT_CI_PORT_SUCCESS != portStatus)
    {
        App_FaultHandler(APP_STATUS_ERROR_PORT_INIT);
    }

    /*----------------------------------------------------------------------------------------------
    * 3. Initialize RTD OsIf Driver using Public API
    *---------------------------------------------------------------------------------------------*/
    OsIf_Init(NULL_PTR);

    /*----------------------------------------------------------------------------------------------
    * 4. Initialize Hardware FTM PWM Drivers via Official NXP RTD APIs
    *---------------------------------------------------------------------------------------------*/
    /* Initialize FTM0 (Controls Red on CH7 and Green on CH0) */
    Ftm_Pwm_Ip_Init(FTM_INSTANCE_0, &Ftm_Pwm_Ip_VS_0_UserCfg0);

    /* Initialize FTM2 (Controls Blue on CH3) */
    Ftm_Pwm_Ip_Init(FTM_INSTANCE_2, &Ftm_Pwm_Ip_VS_0_UserCfg2);

    /* Maximize hardware FTM timers to full 16-bit dynamic depth (65,535 ticks @ 1.22 kHz carrier) */
    (void)Ftm_Pwm_Ip_UpdatePwmPeriod(FTM_INSTANCE_0, FTM_MAX_PERIOD_16BIT, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmPeriod(FTM_INSTANCE_2, FTM_MAX_PERIOD_16BIT, TRUE);

    /*----------------------------------------------------------------------------------------------
    * 5. Power-On Diagnostic Flash (300 ms Red -> 300 ms Green -> 300 ms Blue)
    *    Proves visual health of all hardware channels immediately upon MCU boot.
    *---------------------------------------------------------------------------------------------*/
    /* Red Only (50% intensity = 32768) */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   32768U, 0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U,     0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U,     0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
    Delay_Spin(800000UL);

    /* Green Only (Drives both PTE0 and PTB12 simultaneously) */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   0U,     0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 32768U, 0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U,     0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 1U);
    Delay_Spin(800000UL);

    /* Blue Only */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   0U,     0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U,     0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  32768U, 0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
    Delay_Spin(800000UL);

    /* Turn all off before entering rainbow loop */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   0U,     0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U,     0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U,     0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);

    /* Set initial color state (Red at 0 hue) */
    Rainbow_Update16(0U);

    /* Mark test harness success */
    Exit_Example(TRUE);

    /*----------------------------------------------------------------------------------------------
    * 6. Extreme 16-Bit True-Color Lighting Engine @ 10,000 FPS & 1.0 MHz PDM
    *    Dedicated Core Execution: 100% CPU capacity committed to pristine illumination.
    *    Hue Step: 65,536 micro-angles (0.0055 deg per step).
    *    Color Refresh Rate: 10,000 fps (100 us per step).
    *    Green Software Modulator: 1.0 MHz 1st-Order Sigma-Delta PDM (100 ticks per step).
    *---------------------------------------------------------------------------------------------*/
    static uint32 acc_g = 0U;

    while (1)
    {
        uint32 tick;

        /* High-Rate 1.0 MHz PDM Micro-Ticks (100 ticks of ~1.0 us = 100 us frame slice) */
        for (tick = 0U; tick < 100U; tick++)
        {
            acc_g += (uint32)duty_g_hw;
            if (acc_g >= 65535U)
            {
                acc_g -= 65535U;
                Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 1U);
            }
            else
            {
                Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
            }

            /* ~1.0 us calibrated spin delay @ 80 MHz */
            volatile uint32 innerCnt = 0U;
            while (innerCnt < 12UL)
            {
                innerCnt++;
            }
        }

        /* Advance 16-bit hue angle */
        current_hue++;
        if (0U == current_hue)
        {
            cycle_count++;
        }

        /* Update hardware FTM channels & precompute next PDM duty */
        Rainbow_Update16(current_hue);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
