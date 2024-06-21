/*
   (c) NXP B.V. 2009-2020. All rights reserved.

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
 *    @file phscaUci.h
 *   @brief UCI communication interface
 */

#ifndef PHSCAUCI_INCLUDE_GUARD
#define PHSCAUCI_INCLUDE_GUARD

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include "phscaTypes.h"
#include "phscaUci_Cfg.h"

#ifdef PHSCAUCI_EXTERN_GUARD
	#define EXTERN /**/
#else
   #define EXTERN extern
#endif

/* =============================================================================
 * Symbol Defines
 * ========================================================================== */
#define PHSCAUCI_u8_UCI_HEADER_SIZE_BYTES			   (uint8_t)(4u)
#define PHSCAUCI_u8_UCI_MT_PBF_GID_BYTE_POS        	   (uint8_t)(0u)
#define PHSCAUCI_u8_UCI_OID_BYTE_POS             	   (uint8_t)(1u)
#define PHSCAUCI_u8_UCI_PAYLOADLENGTH_HIGH_BYTE_POS    (uint8_t)(2u)
#define PHSCAUCI_u8_UCI_PAYLOADLENGTH_BYTE_POS    	   (uint8_t)(3u)
#define PHSCAUCI_u8_UCI_TX_PAYLOAD_START_BYTE_POS      (uint8_t)(4u)
#define PHSCAUCI_u8_UCI_RX_PAYLOAD_START_BYTE_POS      (uint8_t)(4u)

#define PHSCAUCI_u8_UCI_MT_MASK  				  	   (uint8_t)(0xE0u)
#define PHSCAUCI_u8_UCI_MT_SHIFT 				   	   (uint8_t)(0x05u)
#define PHSCAUCI_u8_UCI_PBF_MASK  				   	   (uint8_t)(0x10u)
#define PHSCAUCI_u8_UCI_PBF_SHIFT 				   	   (uint8_t)(0x04u)
#define PHSCAUCI_u8_UCI_GID_MASK  				   	   (uint8_t)(0x0Fu)
#define PHSCAUCI_u8_UCI_GID_SHIFT 				   	   (uint8_t)(0x00u)
#define PHSCAUCI_u8_UCI_RFU1_MASK  					   (uint8_t)(0xC0u)
#define PHSCAUCI_u8_UCI_RFU1_SHIFT 					   (uint8_t)(0x06u)
#define PHSCAUCI_u8_UCI_OID_MASK  				   	   (uint8_t)(0x3Fu)
#define PHSCAUCI_u8_UCI_OID_SHIFT 				   	   (uint8_t)(0x00u)
#define PHSCAUCI_u8_UCI_RFU2_MASK  			    	   (uint8_t)(0xFFu)
#define PHSCAUCI_u8_UCI_RFU2_SHIFT 			    	   (uint8_t)(0x00u)
#define PHSCAUCI_u8_UCI_PAYLOADLENGTH_MASK  	  	   (uint8_t)(0xFFu)
#define PHSCAUCI_u8_UCI_PAYLOADLENGTH_SHIFT 		   (uint8_t)(0x00u)

/** Macros to read out MacUci format for each byte */
#define PHSCAUCI_u8_READ_BYTE_UCI_MESSAGE_TYPE(x) 						((uint8_t)((((uint8_t)(x)) & PHSCAUCI_u8_UCI_MT_MASK) >> PHSCAUCI_u8_UCI_MT_SHIFT))
#define PHSCAUCI_u8_READ_BYTE_UCI_PACKAGE_BOUNDARY_FLAG(x) 				((uint8_t)((((uint8_t)(x)) & PHSCAUCI_u8_UCI_PBF_MASK) >> PHSCAUCI_u8_UCI_PBF_SHIFT))
#define PHSCAUCI_u8_READ_BYTE_UCI_GROUP_ID(x) 							((uint8_t)((((uint8_t)(x)) & PHSCAUCI_u8_UCI_GID_MASK) >> PHSCAUCI_u8_UCI_GID_SHIFT))
#define PHSCAUCI_u8_READ_BYTE_UCI_RFU1(x) 								((uint8_t)((((uint8_t)(x)) & PHSCAUCI_u8_UCI_RFU1_MASK) >> PHSCAUCI_u8_UCI_RFU1_SHIFT))
#define PHSCAUCI_u8_READ_BYTE_UCI_OPCODE_ID(x) 							((uint8_t)((((uint8_t)(x)) & PHSCAUCI_u8_UCI_OID_MASK) >> PHSCAUCI_u8_UCI_OID_SHIFT))
#define PHSCAUCI_u8_READ_BYTE_UCI_RFU2(x) 								((uint8_t)((((uint8_t)(x)) & PHSCAUCI_u8_UCI_RFU2_MASK) >> PHSCAUCI_u8_UCI_RFU2_SHIFT))
#define PHSCAUCI_u8_READ_BYTE_UCI_PAYLOAD_LENGTH(x) 					((uint8_t)((((uint8_t)(x)) & PHSCAUCI_u8_UCI_PAYLOADLENGTH_MASK) >> PHSCAUCI_u8_UCI_PAYLOADLENGTH_SHIFT))

/* =============================================================================
 * Type Definitions
 * ========================================================================== */
/** @brief Type of message communicated over the UCI interface */
typedef enum
{
	PHSCAUCI_MESSAGETYPE_COMMAND = 0x01u, ///< 0x01
	PHSCAUCI_MESSAGETYPE_RESPONSE = 0x02u, ///< 0x02
	PHSCAUCI_MESSAGETYPE_NOTIFICATION = 0x03u, ///< 0x03
} phscaUci_en_MessageType_t;

/** @brief Pointer to function type for callback on response or notification received over the UCI interface
 * @param en_MessageType  */
typedef void (*phscaUci_pf_RspNtfReceivedCallback_t)(const phscaUci_en_MessageType_t en_MessageType, const uint8_t u8_Gid, const uint8_t u8_Oid, const uint32_t u32_PayloadLength, const uint8_t * const u8arr_Payload);

/* =============================================================================
 * Public Function-like Macros
 * ========================================================================== */

/* =============================================================================
 * Public Standard Enumerators
 * ========================================================================== */

/* =============================================================================
 * Public Function Prototypes
 * ========================================================================== */
/** @brief Initializes the UCI interface driver
 * @param pf_RspNtfReceivedCallback callback on response or notification received over UCI interface  */
EXTERN void phscaUci_Init(const phscaUci_pf_RspNtfReceivedCallback_t pf_RspNtfReceivedCallback);

/** @brief Transmits the host command to the NCJ29D6 using UCI interface
 * @param u8_BytesToTransmit data to be transmitted over the UCI interface
 * @param u32_DataLengthBytes length of data to be transmitted */
EXTERN void phscaUci_SendCommand(const uint8_t u8_BytesToTransmit[], const uint32_t u32_DataLengthBytes);

/** @brief Get the response/notification from NCJ29D6 back to the host using UCI interface
 * @param u8arr_ReceivedData application supplied buffer in which response or notification data is received
 * @return the number of received bytes from UCI response/notification */
EXTERN uint32_t phscaUci_GetResponse(uint8_t * const u8arr_ReceivedData);

#undef EXTERN
#endif
