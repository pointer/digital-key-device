/*
   (c) NXP B.V. 2009-2019. All rights reserved.

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
 *    @file: phscaUci_Cfg.h
 *   @brief: Configuration of UCI communication interface
 */

#ifndef PHSCAUCI_CFG_INCLUDE_GUARD
#define PHSCAUCI_CFG_INCLUDE_GUARD

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include "phscaTypes.h"

#ifdef PHSCAUCI_CFG_EXTERN_GUARD
	#define EXTERN /**/
#else
   #define EXTERN extern
#endif

/* =============================================================================
 * Symbol Defines
 * ========================================================================== */
/* @brief CRC size in bytes: 0 for no CRC, 1u for CRC-8, 2 for CRC-16, 4 for CRC-32 */
#define PHSCAUCI_u8_CRC_SIZE_BYTES      (0u)

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
/** @brief Hardware specific function to initialize a device that uses the UCI interface
 * @param pf_IntCallback callback on response or notification received over UCI interface  */
EXTERN void phscaUci_InitDevice(void (*pf_IntCallback)(void));

/** @brief Hardware specific function to transceive data over the UCI interface
 * @param u32_DataLength length of data to be transceived over the UCI interface
 * @param u8arr_DataToTransmit data to be transmitted over the UCI interface
 * @param u8arr_DataReceived data received over the UCI interface if the physical interface supports it transceiving */
EXTERN void phscaUci_Transceive(const uint32_t u32_DataLength, const uint8_t const u8arr_DataToTransmit[], uint8_t u8arr_DataReceived[]);

/** @brief Hardware specific function to transceive data over the UCI interface
 * @return true -> data is available on the UCI interface and is ready to be read out */
EXTERN bool phscaUci_IsResponseAvailable(void);

/** @brief Hardware specific function to start command transmission */
EXTERN void phscaUci_StartCommandTx(void);

/** @brief Hardware specific function to stop command transmission */
EXTERN void phscaUci_StopCommandTx(void);

/** @brief Hardware specific function to start response reception */
EXTERN void phscaUci_StartResponseTx(void);

/** @brief Hardware specific function to stop response reception */
EXTERN void phscaUci_StopResponseTx(void);

#undef EXTERN
#endif
