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

/*
 *    @file: phscaNcj29d6_Cfg.h
 *   @brief: Configuration of driver for NCJ29D6 UWB module
 */

#ifndef PHSCANCJ29D6_CFG_INCLUDE_GUARD
#define PHSCANCJ29D6_CFG_INCLUDE_GUARD

/* =============================================================================
 * External Includes
 * ========================================================================== */


#ifdef PHSCANCJ29D6_CFG_EXTERN_GUARD
  #define EXTERN /**/
#else
   #define EXTERN extern
#endif

/* =============================================================================
 * Symbol Defines
 * ========================================================================== */
#define PHSCANCJ29D6_u16_RESET_DELAY_TIME_MS        (uint16_t)(1u)
#define PHSCANCJ29D6_u16_STARTUP_TIME_MS            (uint16_t)(1u)
#define PHSCANCJ29D6_u16_PIN_WAKEUP_TIME_MS         (uint16_t)(1u)
#define PHSCANCJ29D6_u32_CRC16_POLYNOMIAL           (uint32_t)(0x00001021ul)
#define PHSCANCJ29D6_u32_CRC16_SEED                 (uint32_t)(0x00000000ul)

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
/** @brief Initializes hardware specific NCJ29D6 HW device */
EXTERN void phscaNcj29d6_InitDevice(void);

/** @brief Hardware specific function providing blocking delay functionality
 * @param  u32_DelayMilliseconds delay time in milliseconds */
EXTERN void phscaNcj29d6_DelayMilliseconds(const uint32_t u32_DelayMilliseconds);

/** @brief Hardware specific function to assert/deassert RST_N line
 * @param  b_AssertRst true -> RST_N line is asserted (low level), false -> RST_N line is deasserted (high level) */
EXTERN void phscaNcj29d6_SetRst(const bool b_AssertRst);

/** @brief Hardware specific function to assert/deassert CS_N line
 * @param  b_AssertCs true -> CS_N line is asserted (low level), false -> CS_N line is deasserted (high level) */
EXTERN void phscaNcj29d6_SetCs(const bool b_AssertCs);

/** @brief Hardware specific function to get the level of RDY_N line driven by NCJ29D6
 * @return  true -> RDY_N is asserted (low level), false -> RDY_N line is deasserted (high level) */
EXTERN bool phscaNcj29d6_GetRdy(void);

/** @brief Hardware specific function to get the level of INT_N line driven by NCJ29D6
 * @return  true -> INT_N is asserted (low level), false -> INT_N line is deasserted (high level) */
EXTERN bool phscaNcj29d6_GetInt(void);

/** @brief Hardware specific function to get the level of INT_N line driven by NCJ29D6
 * @return  true -> INT_N is asserted (low level), false -> INT_N line is deasserted (high level) */
EXTERN void phscaNcj29d6_SetIntPinInterruptEnable(const bool b_EnableIntPinInterrupt);

/** @brief Hardware specific function to calculate CRC-16 in case it is enabled by RangingApp on NCJ29D6
 * @param u8arr_Data input data on which CRC-16 shall be calculated
 * @param u16_DataLength length of the input data
 * @return Calculated CRC-16 value for the input data */
EXTERN uint16_t phscaNcj29d6_CalculateCrc16(uint8_t u8arr_Data[], uint16_t u16_DataLength);

/** @brief Hardware specific function to transceive data on the SPI bus
 * @param u32_DataLength for the SPI transaction
 * @param u8arr_DataToTransmit length of the input data
 * @param u8arr_DataReceived length of the input data*/
EXTERN void phscaNcj29d6_SpiTransceive(const uint32_t u32_DataLength, const uint8_t u8arr_DataToTransmit[], uint8_t u8arr_DataReceived[]);

/* @brief Hardware specific function to clear the interrupt line status on reception of a response interrupt (INT_N asserted) */
EXTERN void phscaNcj29d6_ClearIntIrqStatus(void);

#undef EXTERN
#endif
