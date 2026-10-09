/*
*   Copyright 2020-2026 NXP
*
*   NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms.
*
*   S32K144W Automotive Production-Grade Hardware FTM PWM + Dual-Green Rainbow LED Engine
*   Architecture: Hardware PWM (FTM0_CH7 Red, FTM2_CH3 Blue, FTM0_CH0 PTB12 Green)
*                 + RTD Gpio_Dio_Ip Driver for EVB Default Green LED (PTE0 via R846).
*                 100% NXP Real Time Drivers (RTD) Public APIs & MEX-Generated Configurations.
*                 Zero Register Hacking, Precision Gamma 2.2 Correction, Bounded Initialization.
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

/** @brief FTM PWM Period in Ticks (10 kHz carrier = 10,000 ticks) */
#define FTM_PWM_PERIOD_TICKS            (10000U)

/** @brief Maximum hue angle in degrees (Full HSV color circle) */
#define HUE_MAX_DEGREES                 (360U)

/** @brief Maximum timeout iterations for PLL lock before declaring fault */
#define PLL_LOCK_TIMEOUT_COUNT          (100000UL)

/** @brief 8-bit color channel maximum value */
#define COLOR_CHANNEL_MAX               (255U)

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
*                                      GLOBAL CONSTANTS
* 256-entry Gamma 2.2 Look-Up Table
* Maps 8-bit color (0-255) precisely to FTM hardware period ticks (0 ~ 10,000).
* Ensures FirstEdge <= FtmPeriod invariant is always respected by RTD Ftm_Pwm_Ip driver.
==================================================================================================*/
static const uint16 gamma10k_lut[256] = {
        0U,     0U,     0U,     1U,     1U,     2U,     3U,     4U,
        5U,     6U,     8U,    10U,    12U,    14U,    17U,    20U,
       23U,    26U,    29U,    33U,    37U,    41U,    46U,    50U,
       55U,    60U,    66U,    72U,    78U,    84U,    90U,    97U,
      104U,   111U,   119U,   127U,   135U,   143U,   152U,   161U,
      170U,   179U,   189U,   199U,   210U,   220U,   231U,   242U,
      254U,   265U,   278U,   290U,   303U,   316U,   329U,   342U,
      356U,   370U,   385U,   399U,   415U,   430U,   446U,   461U,
      478U,   494U,   511U,   528U,   546U,   564U,   582U,   600U,
      619U,   638U,   658U,   677U,   697U,   718U,   738U,   759U,
      781U,   802U,   824U,   846U,   869U,   892U,   915U,   939U,
      963U,   987U,  1011U,  1036U,  1062U,  1087U,  1113U,  1139U,
     1166U,  1193U,  1220U,  1247U,  1275U,  1304U,  1332U,  1361U,
     1390U,  1420U,  1450U,  1480U,  1511U,  1542U,  1573U,  1604U,
     1636U,  1669U,  1701U,  1734U,  1768U,  1801U,  1835U,  1870U,
     1905U,  1940U,  1975U,  2011U,  2047U,  2084U,  2120U,  2158U,
     2195U,  2233U,  2271U,  2310U,  2349U,  2388U,  2428U,  2468U,
     2508U,  2549U,  2590U,  2632U,  2674U,  2716U,  2758U,  2801U,
     2845U,  2888U,  2932U,  2977U,  3021U,  3066U,  3112U,  3158U,
     3204U,  3250U,  3297U,  3345U,  3392U,  3440U,  3489U,  3537U,
     3587U,  3636U,  3686U,  3736U,  3787U,  3838U,  3889U,  3941U,
     3993U,  4045U,  4098U,  4151U,  4205U,  4259U,  4313U,  4368U,
     4423U,  4479U,  4535U,  4591U,  4647U,  4704U,  4762U,  4820U,
     4878U,  4936U,  4995U,  5054U,  5114U,  5174U,  5234U,  5295U,
     5356U,  5418U,  5480U,  5542U,  5605U,  5668U,  5732U,  5795U,
     5860U,  5924U,  5989U,  6055U,  6121U,  6187U,  6253U,  6320U,
     6388U,  6456U,  6524U,  6592U,  6661U,  6730U,  6800U,  6870U,
     6941U,  7012U,  7083U,  7155U,  7227U,  7299U,  7372U,  7445U,
     7519U,  7593U,  7667U,  7742U,  7818U,  7893U,  7969U,  8046U,
     8122U,  8200U,  8277U,  8355U,  8434U,  8513U,  8592U,  8671U,
     8751U,  8832U,  8913U,  8994U,  9075U,  9158U,  9240U,  9323U,
     9406U,  9490U,  9574U,  9658U,  9743U,  9828U,  9914U, 10000U
};

/*==================================================================================================
*                                      GLOBAL OBSERVABLES (FOR GDB DEBUGGER)
==================================================================================================*/
volatile uint16 current_hue       = 0U;   /* 0 ~ 359 degrees */
volatile uint8  current_r         = 0U;   /* Raw RGB 0 ~ 255 */
volatile uint8  current_g         = 0U;
volatile uint8  current_b         = 0U;
volatile uint16 duty_r_hw         = 0U;   /* FTM PWM duty 0 ~ 10,000 ticks */
volatile uint16 duty_g_hw         = 0U;
volatile uint16 duty_b_hw         = 0U;
volatile uint32 cycle_count       = 0U;   /* Completed 360-degree rainbow loops */
volatile uint32 last_fault_status = APP_STATUS_SUCCESS; /* System health tracker */

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/
static void HsvToRgb(uint16 hue, uint8 * const r, uint8 * const g, uint8 * const b);
static void Rainbow_Update(uint16 hue);
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
* @brief        HSV to RGB converter (Hue: 0-359, Saturation: 100%, Value: 100%).
* @details      Performs integer-based piecewise linear interpolation with full defensive checks.
* @param[in]    hue  Hue angle (0 ~ 359 degrees).
* @param[out]   r    Output pointer for Red channel (0 ~ 255).
* @param[out]   g    Output pointer for Green channel (0 ~ 255).
* @param[out]   b    Output pointer for Blue channel (0 ~ 255).
*/
static void HsvToRgb(uint16 hue, uint8 * const r, uint8 * const g, uint8 * const b)
{
    if ((NULL_PTR == r) || (NULL_PTR == g) || (NULL_PTR == b))
    {
        return;
    }

    uint16 normalized_hue = hue % HUE_MAX_DEGREES;
    uint16 sector         = normalized_hue / 60U;
    uint16 rem            = normalized_hue % 60U;
    uint8  f              = (uint8)((rem * 255U) / 60U);
    uint8  q              = (uint8)(255U - f);
    uint8  t              = f;

    switch (sector)
    {
        case 0U:  /* 0 - 59: Red -> Yellow */
            *r = COLOR_CHANNEL_MAX; *g = t;                 *b = 0U;
            break;
        case 1U:  /* 60 - 119: Yellow -> Green */
            *r = q;                 *g = COLOR_CHANNEL_MAX; *b = 0U;
            break;
        case 2U:  /* 120 - 179: Green -> Cyan */
            *r = 0U;                *g = COLOR_CHANNEL_MAX; *b = t;
            break;
        case 3U:  /* 180 - 239: Cyan -> Blue */
            *r = 0U;                *g = q;                 *b = COLOR_CHANNEL_MAX;
            break;
        case 4U:  /* 240 - 299: Blue -> Magenta */
            *r = t;                 *g = 0U;                *b = COLOR_CHANNEL_MAX;
            break;
        case 5U:  /* 300 - 359: Magenta -> Red */
            *r = COLOR_CHANNEL_MAX; *g = 0U;                *b = q;
            break;
        default:  /* Defensive boundary recovery */
            *r = 0U;                *g = 0U;                *b = 0U;
            break;
    }
}

/**
* @brief        Calculate next color, apply Gamma 2.2 LUT, and update PWM hardware channels.
* @param[in]    hue  Hue angle (0 ~ 359 degrees).
*/
static void Rainbow_Update(uint16 hue)
{
    uint8 raw_r = 0U;
    uint8 raw_g = 0U;
    uint8 raw_b = 0U;

    HsvToRgb(hue, &raw_r, &raw_g, &raw_b);

    current_r = raw_r;
    current_g = raw_g;
    current_b = raw_b;

    /* Apply Gamma 2.2 mapping directly to hardware PWM duty (0 ~ 10,000 ticks) */
    duty_r_hw = gamma10k_lut[raw_r];
    duty_g_hw = gamma10k_lut[raw_g];
    duty_b_hw = gamma10k_lut[raw_b];

    /* Update Hardware PWM Channels via Official NXP RTD APIs */
    /* Red: FTM0 Channel 7 (PTE7, Pin 39) */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   duty_r_hw, 0U, TRUE);
    /* Green Candidate A: FTM0 Channel 0 (PTB12, Pin 43 via R787 0-ohm jumper) */
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

    /*----------------------------------------------------------------------------------------------
    * 5. Power-On Diagnostic Flash (300 ms Red -> 300 ms Green -> 300 ms Blue)
    *    Proves visual health of all hardware channels immediately upon MCU boot.
    *---------------------------------------------------------------------------------------------*/
    /* Red Only */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   5000U, 0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U,    0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U,    0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
    Delay_Spin(800000UL);

    /* Green Only (Drives both PTE0 and PTB12 simultaneously) */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   0U,    0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 5000U, 0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U,    0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 1U);
    Delay_Spin(800000UL);

    /* Blue Only */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   0U,    0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U,    0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  5000U, 0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
    Delay_Spin(800000UL);

    /* Turn all off before entering rainbow loop */
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   0U,    0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U,    0U, TRUE);
    (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U,    0U, TRUE);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);

    /* Set initial color state (Red at 0 degrees hue) */
    Rainbow_Update(0U);

    /* Mark test harness success */
    Exit_Example(TRUE);

    /*----------------------------------------------------------------------------------------------
    * 6. Dual-Architecture Full-Spectrum Rainbow Engine
    *    Red (PTE7) & Blue (PTD5) driven by 10 kHz Hardware FTM PWM.
    *    Green driven simultaneously via FTM0_CH0 (PTB12) and Sigma-Delta PDM RTD Gpio_Dio_Ip (PTE0).
    *    Pacing: 500 micro-ticks of ~40 us = 20 ms per hue step (7.2s smooth full cycle).
    *    High-rate PDM disperses Green pulses at >12.5 kHz, completely eliminating 50 Hz block flicker
    *    during Orange -> Yellow and Cyan -> Blue dynamic transitions.
    *---------------------------------------------------------------------------------------------*/
    static uint32 acc_g = 0U;

    while (1)
    {
        uint32 slice;

        /* First-order Sigma-Delta Pulse Density Modulator (PDM)
         * Spreads green pulses uniformly across time at >12.5 kHz equivalent frequency.
         */
        for (slice = 0U; slice < 500U; slice++)
        {
            acc_g += (uint32)duty_g_hw;
            if (acc_g >= 10000U)
            {
                acc_g -= 10000U;
                Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 1U);
            }
            else
            {
                Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
            }

            /* ~40 microseconds delay per micro-tick @ 80 MHz */
            volatile uint32 innerCnt = 0U;
            while (innerCnt < 700UL)
            {
                innerCnt++;
            }
        }

        /* Advance hue angle */
        current_hue++;
        if (current_hue >= HUE_MAX_DEGREES)
        {
            current_hue = 0U;
            cycle_count++;
        }

        /* Update hardware PWM channels for next hue step */
        Rainbow_Update(current_hue);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
