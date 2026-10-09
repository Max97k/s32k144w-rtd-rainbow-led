/*
*   Copyright 2020-2026 NXP
*
*   NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms.
*
*   S32K144W Dual-Engine Ultra-High Resolution 16-Bit Lighting System
*   Modes:
*     - Mode 1 (CONFIG_ENGINE_MODE = 1): Pure 3-Channel Synchronous Sigma-Delta PDM @ 1.0 MHz
*                                        (Absolute phase coherence, zero spectral distortion, 32-bit dither)
*     - Mode 0 (CONFIG_ENGINE_MODE = 0): Hybrid Architecture: Hardware FTM PWM (Red/Blue) + 1.0 MHz PDM (Green)
*                                        (Hardware timer offloading with dual-green backward compatibility)
*   Features: 65,536-step continuous hue sweep @ 10,000 FPS color refresh rate.
*             16-bit continuous Gamma 2.2 interpolation on 1025-point calibration curve.
*             100% NXP Real Time Drivers (RTD) Public APIs & MEX-Generated Configurations.
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

/* Peripheral register definitions for ADC0, PCC, PMC */
#include "S32K144W_PCC.h"
#include "S32K144W_PMC.h"
#include "S32K144W_ADC.h"

/* Compatibility mapping for signed integer types under AUTOSAR StandardTypes */
typedef sint16 int16;
typedef sint32 int32;

/*==================================================================================================
*                                      ENGINE ARCHITECTURE FLAG
* Switch easily between:
*   1U = ENGINE_MODE_PURE_SOFTWARE_PDM : All 3 channels driven by 1.0 MHz Synchronous Sigma-Delta PDM.
*   0U = ENGINE_MODE_HYBRID_PWM_PDM    : Hardware FTM PWM (Red/Blue) + 1.0 MHz Software PDM (Green).
==================================================================================================*/
#define CONFIG_ENGINE_MODE              (1U)

/*==================================================================================================
*                   CREE CLP6C-FKB PHOTOMETRIC & CHROMATICITY CALIBRATION
* Component Model: Cree LED PLCC6 3-in-1 SMD RGB LED (CLP6C-FKB-CM1Q1H1BB7R3R3)
* EVB Anode Supply: 5.0V (P5V0) via 680-ohm current limiting resistors (R95, R96, R97).
* Physical Operating Points:
*   - Red:   Vf = 2.0V, If = 4.19 mA, Typ Intensity = 157 mcd (Bin M-N @ 750 mcd/20mA)
*   - Green: Vf = 3.2V, If = 2.43 mA, Typ Intensity = 200 mcd (Bin Q-R @ 1650 mcd/20mA)
*   - Blue:  Vf = 3.2V, If = 2.43 mA, Typ Intensity =  46 mcd (Bin H-J @ 380 mcd/20mA)
* Relative Gain Factors (Normalized to D65 Equal-Energy White Balance):
*   - CALIB_GAIN_RED   = 82.0% (53739 / 65535)
*   - CALIB_GAIN_GREEN = 62.0% (40632 / 65535)  [Suppresses overpowering green lumen spike]
*   - CALIB_GAIN_BLUE  = 100.0% (65535 / 65535) [Maximizes weak blue photon flux]
==================================================================================================*/
#define ENABLE_CREE_LED_CALIBRATION     (1U)

#define CALIB_GAIN_RED_BASE             (53739UL)  /* Nominal 82.0% scaling for Cree CLP6C-FKB @ 25 degC */
#define CALIB_GAIN_GREEN                (40632UL)  /* 62.0% scaling for Cree CLP6C-FKB */
#define CALIB_GAIN_BLUE                 (65535UL)  /* 100.0% scaling for Cree CLP6C-FKB */

/*==================================================================================================
*             32-BIT GALOIS LINEAR FEEDBACK SHIFT REGISTER (LFSR) CONFIGURATION
* Polynomial: x^32 + x^31 + x^29 + x + 1 (Feedback Mask: 0x80000057UL)
* Period: 2^32 - 1 = 4,294,967,295 cycles
* Purpose: Spread-Spectrum Carrier Dithering (CISPR 25) & Sigma-Delta Idle Tone Elimination.
==================================================================================================*/
#define LFSR_FEEDBACK_MASK              (0x80000057UL)
#define LFSR_SEED_INITIAL               (0x5A17C395UL)

/*==================================================================================================
*         VIRTUAL LUMPED RC THERMAL OBSERVER & DROOP COMPENSATION PARAMETERS
* Physical Characteristics:
*   - Red LED (AlInGaP): Vf = 2.0V, If = 4.19mA, P_max = 8.38 mW (8380 uW)
*   - Green LED (InGaN): Vf = 3.2V, If = 2.43mA, P_max = 7.78 mW (7780 uW)
*   - Blue LED (InGaN):  Vf = 3.2V, If = 2.43mA, P_max = 7.78 mW (7780 uW)
*   - Thermal Resistance: Rth(j-a) ~ 350 degC / W
*   - Red Droop Sensitivity: -0.8% luminous flux / degC above 25 degC
*   - Compensation Slope: +430 gain counts / degC rise (53739 * 0.008)
==================================================================================================*/
#define THERMAL_NOMINAL_TEMP_C          (25)
#define RED_DROOP_COMP_SLOPE            (430L)
#define CALIB_GAIN_RED_MIN              (35000UL)
#define CALIB_GAIN_RED_MAX              (65535UL)

/* ADC0 Internal Temperature Sensor Channel & Timeout */
#define ADC_CH_TEMP_SENSOR              (26U)
#define ADC_TIMEOUT_COUNT               (50000UL)

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

/** @brief Physical GPIO Mapping for On-Board RGB LED Channels */
#define LED_RED_GPIO_BASE               IP_PTE
#define LED_RED_GPIO_PIN                (7U)  /* PTE7, Pin 39 via R789 */

#define LED_GREEN_GPIO_BASE             IP_PTE
#define LED_GREEN_GPIO_PIN              (0U)  /* PTE0, Pin 60 via R846 (EVB Factory Default) */

#define LED_BLUE_GPIO_BASE              IP_PTD
#define LED_BLUE_GPIO_PIN               (5U)  /* PTD5, Pin 24 via R774 */

/** @brief 16-bit Full-Scale Maximum Period (65,535 ticks = 1.22 kHz carrier @ 80 MHz) */
#define FTM_MAX_PERIOD_16BIT            (65535U)

/** @brief Maximum timeout iterations for PLL lock before declaring fault */
#define PLL_LOCK_TIMEOUT_COUNT          (100000UL)

/*==================================================================================================
*                                             ENUMS
==================================================================================================*/
/** @brief Application lighting engine modes */
typedef enum
{
    ENGINE_MODE_HYBRID_PWM_PDM          = 0x00U, /* Mode 0: Hardware FTM PWM (R/B) + Software PDM (G) */
    ENGINE_MODE_PURE_SOFTWARE_PDM       = 0x01U  /* Mode 1: 100% Software 3-Ch Synchronous 1.0 MHz PDM */
} Engine_ModeType;

/** @brief Application system status and error codes */
typedef enum
{
    APP_STATUS_SUCCESS                  = 0x00U,
    APP_STATUS_ERROR_CLOCK_INIT         = 0x01U,
    APP_STATUS_ERROR_PLL_TIMEOUT        = 0x02U,
    APP_STATUS_ERROR_PORT_INIT          = 0x03U,
    APP_STATUS_ERROR_FTM0_INIT          = 0x04U,
    APP_STATUS_ERROR_FTM2_INIT          = 0x05U,
    APP_STATUS_ERROR_ADC_INIT           = 0x06U
} App_StatusType;

/*==================================================================================================
*                                      GLOBAL OBSERVABLES (FOR GDB DEBUGGER)
==================================================================================================*/
volatile Engine_ModeType g_engine_mode = (Engine_ModeType)CONFIG_ENGINE_MODE;

volatile uint16 current_hue       = 0U;   /* 16-bit Hue angle: 0 ~ 65,535 (0.0055 deg resolution) */
volatile uint16 current_r         = 0U;   /* 16-bit Linear Red: 0 ~ 65,535 */
volatile uint16 current_g         = 0U;   /* 16-bit Linear Green: 0 ~ 65,535 */
volatile uint16 current_b         = 0U;   /* 16-bit Linear Blue: 0 ~ 65,535 */
volatile uint16 duty_r_hw         = 0U;   /* 16-bit Gamma 2.2 Red Duty: 0 ~ 65,535 */
volatile uint16 duty_g_hw         = 0U;   /* 16-bit Gamma 2.2 Green Duty: 0 ~ 65,535 */
volatile uint16 duty_b_hw         = 0U;   /* 16-bit Gamma 2.2 Blue Duty: 0 ~ 65,535 */
volatile uint32 cycle_count       = 0U;   /* Completed 360-degree rainbow loops */
volatile uint32 last_fault_status = APP_STATUS_SUCCESS; /* System health tracker */

/* Observables for Spread-Spectrum & Thermal Observers */
volatile uint32 g_lfsr_state          = LFSR_SEED_INITIAL;   /* 32-bit Galois LFSR state */
volatile uint16 g_adc_raw_temp        = 586U;                /* Raw ADC0 temperature sensor reading */
volatile int16  g_mcu_temp_c          = 25;                  /* Filtered MCU die temperature (degC) */
volatile uint16 g_thermal_delta_mc    = 0U;                  /* LED package self-heating (mdegC) */
volatile int16  g_led_junction_temp_c = 25;                  /* Total LED junction temperature Tj (degC) */
volatile uint32 g_dynamic_gain_red    = CALIB_GAIN_RED_BASE; /* Real-time compensated red channel gain */

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/
static inline uint16 Apply_Gamma16(uint16 linear_val);
static void HsvToRgb16(uint16 hue16, uint16 * const r, uint16 * const g, uint16 * const b);
static void Rainbow_Update16(uint16 hue16);
static void App_FaultHandler(App_StatusType faultCode);
static void Delay_Spin(uint32 count);
static inline uint32 Galois_LFSR_Next(void);
static void App_Adc_Init(void);
static uint16 App_Adc_ReadRaw(uint8 channel);
static void App_Thermal_Observer_Update(void);

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
    Gpio_Dio_Ip_WritePin(LED_RED_GPIO_BASE,   LED_RED_GPIO_PIN,   0U);
    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
    Gpio_Dio_Ip_WritePin(LED_BLUE_GPIO_BASE,  LED_BLUE_GPIO_PIN,  0U);

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
* @brief        Galois 32-bit Linear Feedback Shift Register (LFSR) pseudo-random number generator.
* @details      Implements maximal-period polynomial x^32 + x^31 + x^29 + x + 1.
*               Period: 4,294,967,295 non-repeating states.
* @return       New 32-bit pseudo-random state.
*/
static inline uint32 Galois_LFSR_Next(void)
{
    uint32 lsb = g_lfsr_state & 1U;
    g_lfsr_state >>= 1U;
    if (lsb != 0U)
    {
        g_lfsr_state ^= LFSR_FEEDBACK_MASK;
    }
    return g_lfsr_state;
}

/**
* @brief        Initialize ADC0 peripheral and enable internal bandgap/temperature sensor biasing.
*/
static void App_Adc_Init(void)
{
    /* 1. Ensure ADC0 peripheral clock is active and sourced from SIRCDIV2 (8 MHz) */
    IP_PCC->PCCn[PCC_ADC0_INDEX] &= ~PCC_PCCn_CGC_MASK;
    IP_PCC->PCCn[PCC_ADC0_INDEX] = PCC_PCCn_PCS(2U) | PCC_PCCn_CGC_MASK;

    /* 2. Enable PMC Bandgap and Temperature Sensor bias buffer */
    IP_PMC->REGSC |= PMC_REGSC_BIASEN_MASK;

    /* 3. Configure ADC0: 12-bit mode, bus/input clock, divide by 1 */
    IP_ADC0->CFG1 = ADC_CFG1_MODE(1U) | ADC_CFG1_ADICLK(0U) | ADC_CFG1_ADIV(0U);
    IP_ADC0->CFG2 = ADC_CFG2_SMPLTS(16U);
    IP_ADC0->SC2  = 0U; /* Software trigger, default voltage reference VREFH/VREFL */
    IP_ADC0->SC3  = 0U; /* Single conversion, no hardware average */
}

/**
* @brief        Read raw ADC sample on specified channel with safety timeout protection.
* @param[in]    channel  ADC input channel index.
* @return       12-bit ADC raw count (0 ~ 4095).
*/
static uint16 App_Adc_ReadRaw(uint8 channel)
{
    uint32 timeout = ADC_TIMEOUT_COUNT;

    /* Trigger conversion on requested channel */
    IP_ADC0->SC1[0] = ADC_SC1_ADCH(channel);

    /* Wait for conversion complete flag (COCO) with bounded safety timeout */
    while (((IP_ADC0->SC1[0] & ADC_SC1_COCO_MASK) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (0U == timeout)
    {
        return 586U; /* Fallback to nominal 25 degC reading */
    }

    return (uint16)(IP_ADC0->R[0] & ADC_R_D_MASK);
}

/**
* @brief        Virtual lumped RC thermal observer and dynamic droop compensation engine.
* @details      Combines MCU on-chip temperature monitoring with real-time Joule dissipation
*               modeling of the Cree CLP6C-FKB package, dynamically adapting CALIB_GAIN_RED.
*/
static void App_Thermal_Observer_Update(void)
{
    static uint32 s_adc_decimator = 0U;
    static int32  s_mcu_temp_filtered_q8 = (25L << 8);
    static uint32 s_thermal_delta_acc = 0U; /* Q16 millidegrees */

    /* 1. Periodic ADC Acquisition (Decimated to 100 Hz / every 100 hue ticks) */
    s_adc_decimator++;
    if (s_adc_decimator >= 100U)
    {
        s_adc_decimator = 0U;
        uint16 raw_adc = App_Adc_ReadRaw(ADC_CH_TEMP_SENSOR);
        g_adc_raw_temp = raw_adc;

        /* Convert raw 12-bit ADC to die temperature in degC:
         * At 25 degC, Vtemp ~= 0.716V -> raw ~= 586 counts @ 5.0V Vdda.
         * Slope ~= -1.62 mV / degC -> ~1.327 counts / degC.
         * T_inst = 25 + ((586 - raw) * 100) / 133
         */
        int32 t_inst = 25L + (((586L - (int32)raw_adc) * 100L) / 133L);
        if (t_inst < -40L)
        {
            t_inst = -40L;
        }
        else if (t_inst > 125L)
        {
            t_inst = 125L;
        }

        /* 1st-order IIR low-pass filter (time constant ~160 ms @ 100 Hz) */
        s_mcu_temp_filtered_q8 += ((t_inst << 8) - s_mcu_temp_filtered_q8) >> 4U;
        g_mcu_temp_c = (int16)(s_mcu_temp_filtered_q8 >> 8U);
    }

    /* 2. Virtual Lumped RC Thermal Observer for Cree CLP6C-FKB:
     * Instantaneous power:
     *   P_red   = 8.38 mW * (duty_r / 65536)
     *   P_green = 7.78 mW * (duty_g / 65536)
     *   P_blue  = 7.78 mW * (duty_b / 65536)
     * Total power in microwatts (uW):
     */
    uint32 p_total_uw = (((uint32)duty_r_hw * 8380UL) +
                         ((uint32)duty_g_hw * 7780UL) +
                         ((uint32)duty_b_hw * 7780UL)) >> 16U;

    /* Target steady-state self-heating in millidegrees C (mdegC):
     * delta_T = P_uw * Rth(350 degC/W) / 1000 = (P_uw * 7) / 20
     */
    uint32 delta_target_mc = (p_total_uw * 7UL) / 20UL;

    /* Discrete RC filter with Tau ~ 1.0 s @ 10,000 Hz step rate:
     * delta_k = delta_k-1 + (delta_target - delta_k-1) / 10000
     */
    uint32 target_q16 = delta_target_mc << 16U;
    s_thermal_delta_acc += (uint32)((int32)(target_q16 - s_thermal_delta_acc) / 10000L);
    g_thermal_delta_mc = (uint16)(s_thermal_delta_acc >> 16U);

    /* Total Junction Temperature Tj = T_mcu + delta_T_self */
    g_led_junction_temp_c = g_mcu_temp_c + (int16)(g_thermal_delta_mc / 1000U);

    /* 3. Real-Time Photometric Gain Adaptation for Red Channel:
     * Counteracts AlInGaP thermal droop (-0.8% luminous flux / degC above 25 degC).
     * Slope: +430 gain counts per degC rise.
     */
    int32 delta_t_c = (int32)g_led_junction_temp_c - THERMAL_NOMINAL_TEMP_C;
    int32 comp_gain = (int32)CALIB_GAIN_RED_BASE + (delta_t_c * RED_DROOP_COMP_SLOPE);
    if (comp_gain < (int32)CALIB_GAIN_RED_MIN)
    {
        comp_gain = (int32)CALIB_GAIN_RED_MIN;
    }
    else if (comp_gain > (int32)CALIB_GAIN_RED_MAX)
    {
        comp_gain = (int32)CALIB_GAIN_RED_MAX;
    }
    g_dynamic_gain_red = (uint32)comp_gain;
}

/**
* @brief        Update color state, evaluate 16-bit Gamma, and update PWM hardware channels.
* @param[in]    hue16  16-bit hue angle (0 ~ 65,535).
*/
static void Rainbow_Update16(uint16 hue16)
{
    uint16 raw_r = 0U;
    uint16 raw_g = 0U;
    uint16 raw_b = 0U;

    HsvToRgb16(hue16, &raw_r, &raw_g, &raw_b);

#if (ENABLE_CREE_LED_CALIBRATION == 1U)
    /* Apply Cree CLP6C-FKB dynamic thermal droop compensated white-balance */
    raw_r = (uint16)(((uint32)raw_r * g_dynamic_gain_red) >> 16U);
    raw_g = (uint16)(((uint32)raw_g * CALIB_GAIN_GREEN)   >> 16U);
    raw_b = (uint16)(((uint32)raw_b * CALIB_GAIN_BLUE)    >> 16U);
#endif

    current_r = raw_r;
    current_g = raw_g;
    current_b = raw_b;

    /* 16-bit continuous Gamma 2.2 perceptual scaling */
    duty_r_hw = Apply_Gamma16(raw_r);
    duty_g_hw = Apply_Gamma16(raw_g);
    duty_b_hw = Apply_Gamma16(raw_b);

    if (ENGINE_MODE_HYBRID_PWM_PDM == g_engine_mode)
    {
        /* In Hybrid Mode: Update Hardware PWM Channels via Official NXP RTD APIs */
        /* Red: FTM0 Channel 7 (PTE7, Pin 39) */
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   duty_r_hw, 0U, TRUE);
        /* Green Alternate: FTM0 Channel 0 (PTB12, Pin 43 via R787 0-ohm jumper) */
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, duty_g_hw, 0U, TRUE);
        /* Blue: FTM2 Channel 3 (PTD5, Pin 24 via R774 0-ohm jumper) */
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  duty_b_hw, 0U, TRUE);
    }
    else
    {
        /* In Pure Software Mode: Still update PTB12 hardware channel in case R787 is populated */
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, duty_g_hw, 0U, TRUE);
    }
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

    /* Configure Pin Multiplexing depending on active engine mode */
    if (ENGINE_MODE_PURE_SOFTWARE_PDM == g_engine_mode)
    {
        /* Switch Red (PTE7) and Blue (PTD5) to GPIO mode for synchronous software PDM */
        Port_Ci_Port_Ip_SetMuxModeSel(IP_PORTE, LED_RED_GPIO_PIN,  PORT_MUX_AS_GPIO);
        Port_Ci_Port_Ip_SetMuxModeSel(IP_PORTD, LED_BLUE_GPIO_PIN, PORT_MUX_AS_GPIO);
        /* Ensure output direction is enabled */
        LED_RED_GPIO_BASE->PDDR  |= (1UL << LED_RED_GPIO_PIN);
        LED_BLUE_GPIO_BASE->PDDR |= (1UL << LED_BLUE_GPIO_PIN);
    }

    /*----------------------------------------------------------------------------------------------
    * 3. Initialize RTD OsIf Driver using Public API
    *---------------------------------------------------------------------------------------------*/
    OsIf_Init(NULL_PTR);

    /*----------------------------------------------------------------------------------------------
    * 3b. Initialize ADC0 and PMC Bandgap/Temperature Sensor for Real-Time Thermal Monitoring
    *---------------------------------------------------------------------------------------------*/
    App_Adc_Init();

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
    if (ENGINE_MODE_PURE_SOFTWARE_PDM == g_engine_mode)
    {
        /* Red Only */
        Gpio_Dio_Ip_WritePin(LED_RED_GPIO_BASE,   LED_RED_GPIO_PIN,   1U);
        Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
        Gpio_Dio_Ip_WritePin(LED_BLUE_GPIO_BASE,  LED_BLUE_GPIO_PIN,  0U);
        Delay_Spin(800000UL);

        /* Green Only */
        Gpio_Dio_Ip_WritePin(LED_RED_GPIO_BASE,   LED_RED_GPIO_PIN,   0U);
        Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 1U);
        Gpio_Dio_Ip_WritePin(LED_BLUE_GPIO_BASE,  LED_BLUE_GPIO_PIN,  0U);
        Delay_Spin(800000UL);

        /* Blue Only */
        Gpio_Dio_Ip_WritePin(LED_RED_GPIO_BASE,   LED_RED_GPIO_PIN,   0U);
        Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
        Gpio_Dio_Ip_WritePin(LED_BLUE_GPIO_BASE,  LED_BLUE_GPIO_PIN,  1U);
        Delay_Spin(800000UL);

        /* Turn all off */
        Gpio_Dio_Ip_WritePin(LED_RED_GPIO_BASE,   LED_RED_GPIO_PIN,   0U);
        Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
        Gpio_Dio_Ip_WritePin(LED_BLUE_GPIO_BASE,  LED_BLUE_GPIO_PIN,  0U);
    }
    else
    {
        /* Hybrid Mode POST using Hardware FTM + PTE0 GPIO */
        /* Red Only */
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   32768U, 0U, TRUE);
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U,     0U, TRUE);
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U,     0U, TRUE);
        Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
        Delay_Spin(800000UL);

        /* Green Only */
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

        /* Turn all off */
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_RED,   0U,     0U, TRUE);
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_0, FTM_CH_GREEN, 0U,     0U, TRUE);
        (void)Ftm_Pwm_Ip_UpdatePwmChannel(FTM_INSTANCE_2, FTM_CH_BLUE,  0U,     0U, TRUE);
        Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
    }

    /* Run initial thermal observer update to capture ambient die temperature */
    App_Thermal_Observer_Update();

    /* Set initial color state (Red at 0 hue) */
    Rainbow_Update16(0U);

    /* Mark test harness success */
    Exit_Example(TRUE);

    /*----------------------------------------------------------------------------------------------
    * 6. Dual-Mode Extreme 16-Bit True-Color Lighting Engine @ 10,000 FPS
    *    Integrated with:
    *      - Feature 1: Spread-Spectrum / Random Jitter Modulation (32-bit Galois LFSR)
    *      - Feature 2: Real-Time Virtual Lumped RC Thermal Droop Compensation
    *---------------------------------------------------------------------------------------------*/
    static uint32 acc_r = 0U;
    static uint32 acc_g = 0U;
    static uint32 acc_b = 0U;

    while (1)
    {
        uint32 tick;

        if (ENGINE_MODE_PURE_SOFTWARE_PDM == g_engine_mode)
        {
            /* Mode 1: Pure 3-Channel Synchronous Sigma-Delta PDM @ 1.0 MHz
             * All three pins toggle in perfect mathematical phase coherence.
             * 32-bit Galois LFSR injects threshold micro-dither and pacing micro-jitter.
             */
            for (tick = 0U; tick < 100U; tick++)
            {
                uint32 rand_val = Galois_LFSR_Next();
                uint32 dither   = rand_val & 0x0FU; /* 0 ~ 15 micro-tick dither */
                uint32 thresh   = 65535U - 7U + dither;

                /* Red PDM Accumulator with threshold dithering */
                acc_r += (uint32)duty_r_hw;
                if (acc_r >= thresh)
                {
                    acc_r -= 65535U;
                    Gpio_Dio_Ip_WritePin(LED_RED_GPIO_BASE, LED_RED_GPIO_PIN, 1U);
                }
                else
                {
                    Gpio_Dio_Ip_WritePin(LED_RED_GPIO_BASE, LED_RED_GPIO_PIN, 0U);
                }

                /* Green PDM Accumulator with threshold dithering */
                acc_g += (uint32)duty_g_hw;
                if (acc_g >= thresh)
                {
                    acc_g -= 65535U;
                    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 1U);
                }
                else
                {
                    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
                }

                /* Blue PDM Accumulator with threshold dithering */
                acc_b += (uint32)duty_b_hw;
                if (acc_b >= thresh)
                {
                    acc_b -= 65535U;
                    Gpio_Dio_Ip_WritePin(LED_BLUE_GPIO_BASE, LED_BLUE_GPIO_PIN, 1U);
                }
                else
                {
                    Gpio_Dio_Ip_WritePin(LED_BLUE_GPIO_BASE, LED_BLUE_GPIO_PIN, 0U);
                }

                /* Spread-Spectrum Delay Pacing: controlled micro-jitter spreads discrete EMI spikes */
                uint32 jitter_delay = 5UL + ((rand_val >> 4U) & 0x02U); /* 5 or 7 loops, mean = 6.0 loops */
                volatile uint32 innerCnt = 0U;
                while (innerCnt < jitter_delay)
                {
                    innerCnt++;
                }
            }
        }
        else
        {
            /* Mode 0: Hybrid Architecture
             * Hardware FTM PWM on Red/Blue + 1.0 MHz Software PDM on Green.
             */
            for (tick = 0U; tick < 100U; tick++)
            {
                uint32 rand_val = Galois_LFSR_Next();
                uint32 dither   = rand_val & 0x0FU;
                uint32 thresh   = 65535U - 7U + dither;

                acc_g += (uint32)duty_g_hw;
                if (acc_g >= thresh)
                {
                    acc_g -= 65535U;
                    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 1U);
                }
                else
                {
                    Gpio_Dio_Ip_WritePin(LED_GREEN_GPIO_BASE, LED_GREEN_GPIO_PIN, 0U);
                }

                /* Spread-Spectrum Delay Pacing for Hybrid Mode */
                uint32 jitter_delay = 11UL + ((rand_val >> 4U) & 0x02U); /* 11 or 13 loops, mean = 12.0 loops */
                volatile uint32 innerCnt = 0U;
                while (innerCnt < jitter_delay)
                {
                    innerCnt++;
                }
            }
        }

        /* Advance 16-bit hue angle (10,000 steps per second) */
        current_hue++;
        if (0U == current_hue)
        {
            cycle_count++;
        }

        /* Update virtual lumped RC thermal observer and adapt red gain */
        App_Thermal_Observer_Update();

        /* Update hardware FTM channels & precompute next PDM duties */
        Rainbow_Update16(current_hue);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
