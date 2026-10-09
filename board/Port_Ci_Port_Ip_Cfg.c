/*==================================================================================================
*   Project              : RTD AUTOSAR 4.7 
*   Platform             : CORTEXM
*   Peripheral           : PORT_CI
*   Dependencies         : none
*
*   Autosar Version      : 4.7.0
*   Autosar Revision     : ASR_REL_4_7_REV_0000
*   Autosar Conf.Variant :
*   SW Version           : 3.0.0
*   Build Version        : S32K1_S32M24_RTD_3_0_0_D2503_ASR_REL_4_7_REV_0000_20250307
*
*   Copyright 2020-2025 NXP
*
*   NXP Confidential. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms. By expressly
*   accepting such terms or by downloading, installing, activating and/or otherwise
*   using the software, you are agreeing that you have read, and that you agree to
*   comply with and are bound by, such license terms. If you do not agree to be
*   bound by the applicable license terms, then you may not retain, install,
*   activate or otherwise use the software.
==================================================================================================*/

/* clang-format off */
/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Pins v17.0
processor: S32K144W
package_id: S32K144W_LQFP64
mcu_data: PlatformSDK_S32K1_S32M24
processor_version: 0.0.0
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */
/* clang-format on */
/*==================================================================================================*/

/**
*   @file      Port_Ci_Port_Ip_Cfg.c
*
*   @addtogroup Port_CFG
*   @{
*/

#ifdef __cplusplus
extern "C"{
#endif


/*==================================================================================================
                                         INCLUDE FILES
 1) system and project includes
 2) needed interfaces from external units
 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Port_Ci_Port_Ip_Cfg.h"

/*==================================================================================================
*                              SOURCE FILE VERSION INFORMATION
==================================================================================================*/
#define PORT_CI_PORT_IP_VENDOR_ID_CFG_C                       43
#define PORT_CI_PORT_IP_AR_RELEASE_MAJOR_VERSION_CFG_C        4
#define PORT_CI_PORT_IP_AR_RELEASE_MINOR_VERSION_CFG_C        7
#define PORT_CI_PORT_IP_AR_RELEASE_REVISION_VERSION_CFG_C     0
#define PORT_CI_PORT_IP_SW_MAJOR_VERSION_CFG_C                3
#define PORT_CI_PORT_IP_SW_MINOR_VERSION_CFG_C                0
#define PORT_CI_PORT_IP_SW_PATCH_VERSION_CFG_C                0

/*==================================================================================================
*                                     FILE VERSION CHECKS
==================================================================================================*/
/* Check if Port_Ci_Port_Ip_Cfg.c and Port_Ci_Port_Ip_Cfg.h are of the same vendor */
#if (PORT_CI_PORT_IP_VENDOR_ID_CFG_C != PORT_CI_PORT_IP_VENDOR_ID_CFG_H)
    #error "Port_Ci_Port_Ip_Cfg.c and Port_Ci_Port_Ip_Cfg.h have different vendor ids"
#endif
/* Check if Port_Ci_Port_Ip_Cfg.c and Port_Ci_Port_Ip_Cfg.h are of the same Autosar version */
#if ((PORT_CI_PORT_IP_AR_RELEASE_MAJOR_VERSION_CFG_C    != PORT_CI_PORT_IP_AR_RELEASE_MAJOR_VERSION_CFG_H) || \
     (PORT_CI_PORT_IP_AR_RELEASE_MINOR_VERSION_CFG_C    != PORT_CI_PORT_IP_AR_RELEASE_MINOR_VERSION_CFG_H) || \
     (PORT_CI_PORT_IP_AR_RELEASE_REVISION_VERSION_CFG_C != PORT_CI_PORT_IP_AR_RELEASE_REVISION_VERSION_CFG_H) \
    )
    #error "AutoSar Version Numbers of Port_Ci_Port_Ip_Cfg.c and Port_Ci_Port_Ip_Cfg.h are different"
#endif
/* Check if Port_Ci_Port_Ip_Cfg.c and Port_Ci_Port_Ip_Cfg.h are of the same Software version */
#if ((PORT_CI_PORT_IP_SW_MAJOR_VERSION_CFG_C != PORT_CI_PORT_IP_SW_MAJOR_VERSION_CFG_H) || \
     (PORT_CI_PORT_IP_SW_MINOR_VERSION_CFG_C != PORT_CI_PORT_IP_SW_MINOR_VERSION_CFG_H) || \
     (PORT_CI_PORT_IP_SW_PATCH_VERSION_CFG_C != PORT_CI_PORT_IP_SW_PATCH_VERSION_CFG_H)    \
    )
    #error "Software Version Numbers of Port_Ci_Port_Ip_Cfg.c and Port_Ci_Port_Ip_Cfg.h are different"
#endif

/*==================================================================================================
                             LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
                                             LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                                            LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                           LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                                           GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                           GLOBAL VARIABLES
==================================================================================================*/


/* clang-format off */
/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
PortContainer_0_VS_0:
- options: {callFromInitBoot: 'true', coreID: core0}
- pin_list:
  - {pin_num: '39', peripheral: FTM0, signal: ftm0_ch7, pin_signal: PTE7, direction: OUTPUT}
  - {pin_num: '43', peripheral: FTM0, signal: ftm0_ch0, pin_signal: PTB12, direction: OUTPUT}
  - {pin_num: '24', peripheral: FTM2, signal: ftm2_ch3, pin_signal: PTD5, direction: OUTPUT}
  - {pin_num: '60', peripheral: PORTE, signal: 'port, 0', pin_signal: PTE0, direction: OUTPUT}
  - {pin_num: '63', peripheral: SYSTEM, signal: reset_b, pin_signal: PTA5}
  - {pin_num: '58', peripheral: JTAG_TRACENOETM, signal: jtag_tdo_noetm_trace_swo, pin_signal: PTA10}
  - {pin_num: '61', peripheral: JTAG, signal: jtag_tdi, pin_signal: PTC5}
  - {pin_num: '62', peripheral: JTAG_SWD, signal: jtag_tclk_swd_clk, pin_signal: PTC4}
  - {pin_num: '64', peripheral: JTAG_SWD, signal: jtag_tms_swd_dio, pin_signal: PTA4}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */
/* clang-format on */

#define PORT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Port_MemMap.h"

/* Generate array of configured pin structures */
Port_Ci_Port_Ip_PinSettingsConfig g_pin_mux_InitConfigArr_PortContainer_0_VS_0[NUM_OF_CONFIGURED_PINS_PortContainer_0_VS_0] = {
    {
        .portBase        = IP_PORTA,
        .gpioBase        = NULL_PTR,
        .pinPortIdx      = 10U,
        .pullConfig      = PORT_INTERNAL_PULL_NOT_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_HIGH,
        .passiveFilter   = (boolean)FALSE,
        .mux             = PORT_MUX_ALT7,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .digitalFilter   = (boolean)FALSE,
    },
    {
        .portBase        = IP_PORTA,
        .gpioBase        = NULL_PTR,
        .pinPortIdx      = 4U,
        .pullConfig      = PORT_INTERNAL_PULL_UP_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_LOW,
        .passiveFilter   = (boolean)FALSE,
        .mux             = PORT_MUX_ALT7,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .digitalFilter   = (boolean)FALSE,
    },
    {
        .portBase        = IP_PORTA,
        .gpioBase        = NULL_PTR,
        .pinPortIdx      = 5U,
        .pullConfig      = PORT_INTERNAL_PULL_UP_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_LOW,
        .passiveFilter   = (boolean)TRUE,
        .mux             = PORT_MUX_ALT7,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .digitalFilter   = (boolean)FALSE,
    },
    {
        .portBase        = IP_PORTB,
        .gpioBase        = NULL_PTR,
        .pinPortIdx      = 12U,
        .pullConfig      = PORT_INTERNAL_PULL_NOT_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_LOW,
        .passiveFilter   = (boolean)FALSE,
        .mux             = PORT_MUX_ALT2,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .digitalFilter   = (boolean)FALSE,
    },
    {
        .portBase        = IP_PORTC,
        .gpioBase        = NULL_PTR,
        .pinPortIdx      = 4U,
        .pullConfig      = PORT_INTERNAL_PULL_DOWN_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_LOW,
        .passiveFilter   = (boolean)FALSE,
        .mux             = PORT_MUX_ALT7,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .digitalFilter   = (boolean)FALSE,
    },
    {
        .portBase        = IP_PORTC,
        .gpioBase        = NULL_PTR,
        .pinPortIdx      = 5U,
        .pullConfig      = PORT_INTERNAL_PULL_UP_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_LOW,
        .passiveFilter   = (boolean)FALSE,
        .mux             = PORT_MUX_ALT7,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .digitalFilter   = (boolean)FALSE,
    },
    {
        .portBase        = IP_PORTD,
        .gpioBase        = NULL_PTR,
        .pinPortIdx      = 5U,
        .pullConfig      = PORT_INTERNAL_PULL_NOT_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_LOW,
        .passiveFilter   = (boolean)FALSE,
        .mux             = PORT_MUX_ALT2,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .digitalFilter   = (boolean)FALSE,
    },
    {
        .portBase        = IP_PORTE,
        .gpioBase        = IP_PTE,
        .pinPortIdx      = 0U,
        .pullConfig      = PORT_INTERNAL_PULL_NOT_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_LOW,
        .passiveFilter   = (boolean)FALSE,
        .mux             = PORT_MUX_AS_GPIO,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .direction       = PORT_CI_PORT_PIN_OUT,
        .digitalFilter   = (boolean)FALSE,
        .initValue       = 0U,
    },
    {
        .portBase        = IP_PORTE,
        .gpioBase        = NULL_PTR,
        .pinPortIdx      = 7U,
        .pullConfig      = PORT_INTERNAL_PULL_NOT_ENABLED,
        .driveStrength   = PORT_DRIVE_STRENGTH_LOW,
        .passiveFilter   = (boolean)FALSE,
        .mux             = PORT_MUX_ALT2,
        .lockRegister    = PORT_LOCK_REGISTER_DISABLED,
        .digitalFilter   = (boolean)FALSE,
    },
};

#define PORT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Port_MemMap.h"

/*==================================================================================================
                                      LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                                           LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
                                           GLOBAL FUNCTIONS
==================================================================================================*/


#ifdef __cplusplus
}
#endif

/** @} */

/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/
