/*
 (c) NXP B.V. 2019-2021. All rights reserved.

 Disclaimer
 1. The NXP Software/Source Code is provided to Licensee "AS IS" without any
 warranties of any kind. NXP makes no warranties to Licensee and shall not
 indemnify Licensee or hold it harmless for any reason related to the NXP
 Software/Source Code or otherwise be liable to the NXP customer. The NXP
 customer acknowledges and agrees that the NXP Software/Source Code is
 provided AS-IS and accepts all risks of utilizing the NXP Software under
 the conditions set forth according to this disclaimer.

 2. NXP EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING,
 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY
 RIGHTS. NXP SHALL HAVE NO LIABILITY TO THE NXP CUSTOMER, OR ITS
 SUBSIDIARIES, AFFILIATES, OR ANY OTHER THIRD PARTY FOR ANY DAMAGES,
 INCLUDING WITHOUT LIMITATION, DAMAGES RESULTING OR ALLEGDED TO HAVE
 RESULTED FROM ANY DEFECT, ERROR OR OMMISSION IN THE NXP SOFTWARE/SOURCE
 CODE, THIRD PARTY APPLICATION SOFTWARE AND/OR DOCUMENTATION, OR AS A
 RESULT OF ANY INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHT OF ANY
 THIRD PARTY. IN NO EVENT SHALL NXP BE LIABLE FOR ANY INCIDENTAL,
 INDIRECT, SPECIAL, EXEMPLARY, PUNITIVE, OR CONSEQUENTIAL DAMAGES
 (INCLUDING LOST PROFITS) SUFFERED BY NXP CUSTOMER OR ITS SUBSIDIARIES,
 AFFILIATES, OR ANY OTHER THIRD PARTY ARISING OUT OF OR RELATED TO THE NXP
 SOFTWARE/SOURCE CODE EVEN IF NXP HAS BEEN ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGES.

 3. NXP reserves the right to make changes to the NXP Software/Sourcecode any
 time, also without informing customer.

 4. Licensee agrees to indemnify and hold harmless NXP and its affiliated
 companies from and against any claims, suits, losses, damages,
 liabilities, costs and expenses (including reasonable attorney's fees)
 resulting from Licensee's and/or Licensee customer's/licensee's use of the
 NXP Software/Source Code.
 */

/**
 *    @file phscaNcj29d6.h
 *   @brief Driver for NCJ29D6 UWB module
 */

#ifndef PHSCANCJ29D6_INCLUDE_GUARD
#define PHSCANCJ29D6_INCLUDE_GUARD

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include "phscaTypes.h"
#include "phscaNcj29d6_Cfg.h"

#ifdef PHSCANCJ29D6_EXTERN_GUARD
	#define EXTERN /**/
#else
   #define EXTERN extern
#endif

/* =============================================================================
 * Symbol Defines
 * ========================================================================== */

/* =============================================================================
 * Type Definitions
 * ========================================================================== */

/* =============================================================================
 * Public Function-like Macros
 * ========================================================================== */

/* =============================================================================
 * Public Standard Enumerators
 * ========================================================================== */

/* =============================================================================
 * Public Function Prototypes
 * ========================================================================== */
/** @brief Initialize NCJ29D6 driver
 * @param pf_IntCallback callback on INT_N assertion */
EXTERN void phscaNcj29d6_Init(void (*pf_IntCallback)(void));

/** @brief Callback on NCJ29D6 INT_N low edge */
EXTERN void phscaNcj29d6_IntPinCallbackIsr(void);

/** @brief Performs a hard reset of NCJ29D6 by pulling the RST line low for
 * u32_ResetTimeMilliseconds and pulling it up to high again then waiting
 * for u32_DelayAfterResetMilliseconds before return
 * @param u32_ResetTimeMilliseconds time in milliseconds between RST_N low and RST_N high
 * @param u32_DelayAfterResetMilliseconds time in milliseconds after RST_N high */
EXTERN void phscaNcj29d6_HardReset(const uint32_t u32_ResetTimeMilliseconds, const uint32_t u32_DelayAfterResetMilliseconds);

/** @brief Pulls the reset line low to keep NCJ29D6 in lowest power consumption state */
EXTERN void phscaNcj29d6_Disable(void);

/** @brief Pulls the reset line high to start the NCJ29D6 in case phscaNcj29d6_Disable was called */
EXTERN void phscaNcj29d6_Enable(void);

/** @brief Pulls CS_N line low then high to wakeup NCJ29D6 from HPD (in case CS_N was selected as WUP source) */
EXTERN void phscaNcj29d6_Wakeup(void);

/** @brief Software based calculation of CRC16 in case a hardware CRC block
 * is not present on host controller
 * @param u8arr_Data input data on which the CRC-16 shall be calculated
 * @param u16_DataLength length of the input data */
EXTERN uint16_t phscaNcj29d6_CalculateCrc16Sw(const uint8_t const u8arr_Data[], const uint16_t u16_DataLength);

#undef EXTERN
#endif
