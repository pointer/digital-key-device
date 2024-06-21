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
 *    @file: phscaNcj29d6.c
 *   @brief: Driver for NCJ29D6 UWB module
 */

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include "phscaTypes.h"

/* =============================================================================
 * Internal Includes
 * ========================================================================== */
#define PHSCANCJ29D6_EXTERN_GUARD
#include "phscaNcj29d6_Cfg.h"
#include "phscaNcj29d6.h"
#undef PHSCANCJ29D6_EXTERN_GUARD

/* =============================================================================
 * Private Symbol Defines
 * ========================================================================== */

/* =============================================================================
 * Private Function-like Macros
 * ========================================================================== */

/* =============================================================================
 * Private Type Definitions
 * ========================================================================== */

/* =============================================================================
 * Private Function Prototypes
 * ========================================================================== */
/* @brief Calculates the next byte a CRC calculation based on the input CRC value */
static uint16_t phscaNcj29d6_CalculateCrc16Byte(uint16_t u16_CRCValue, uint8_t u8_NewByte);

/* =============================================================================
 * Private Module-wide Visible Variables
 * ========================================================================== */
static void (*m_pf_IntCallback)(void) = PHSCATYPES_pv_NULLPTR;

/* =============================================================================
 * Function Definitions
 * ========================================================================== */

/*****************************************************************************************************/
/*** Low Level RCI functions *************************************************************************/
/*****************************************************************************************************/
void phscaNcj29d6_Init(void (*pf_IntCallback)(void))
{
	m_pf_IntCallback = pf_IntCallback;
	/* Initialize NCJ29D6 6-wire interface e.g. SPI/GPIO pin muxing as well
	* as SPI peripheral initialization */
	phscaNcj29d6_InitDevice();
}

void phscaNcj29d6_IntPinCallbackIsr(void)
{
	phscaNcj29d6_ClearIntIrqStatus();
	if(m_pf_IntCallback != PHSCATYPES_pv_NULLPTR)
	{
		m_pf_IntCallback();
	}
	else
	{
		/* Callback for INT_N low edge not defined. Do nothing. */
	}
}

void phscaNcj29d6_HardReset(const uint32_t u32_ResetTimeMilliseconds,
		                      const uint32_t u32_DelayAfterResetMilliseconds)
{
  phscaNcj29d6_SetRst(PHSCATYPES_b_TRUE);
  phscaNcj29d6_DelayMilliseconds(u32_ResetTimeMilliseconds);
  phscaNcj29d6_SetRst(PHSCATYPES_b_FALSE);
  phscaNcj29d6_DelayMilliseconds(u32_DelayAfterResetMilliseconds);
}

void phscaNcj29d6_Disable(void)
{
  phscaNcj29d6_SetRst(PHSCATYPES_b_TRUE);
}

void phscaNcj29d6_Enable(void)
{
	phscaNcj29d6_SetRst(PHSCATYPES_b_FALSE);
}

void phscaNcj29d6_Wakeup(void)
{
  phscaNcj29d6_SetCs(PHSCATYPES_b_TRUE);
  phscaNcj29d6_DelayMilliseconds(PHSCANCJ29D6_u16_PIN_WAKEUP_TIME_MS);
  phscaNcj29d6_SetCs(PHSCATYPES_b_FALSE);
}

static uint16_t phscaNcj29d6_CalculateCrc16Byte(uint16_t u16_CRCValue, uint8_t u8_NewByte)
{
  uint8_t u8_ByteLoopIndex;

  for(u8_ByteLoopIndex = PHSCATYPES_u8_MIN_U8; u8_ByteLoopIndex < PHSCATYPES_u8_BITS_IN_ONE_BYTE; u8_ByteLoopIndex++)
  {
    if(((u16_CRCValue & 0x8000u) >> 8u) ^ (u8_NewByte & 0x80u))
    {
      u16_CRCValue = (uint16_t)(u16_CRCValue << (uint16_t)1u) ^ (uint16_t)PHSCANCJ29D6_u32_CRC16_POLYNOMIAL;
    }
    else
    {
      u16_CRCValue = (uint16_t)((u16_CRCValue << (uint16_t)1u));
    }
    u8_NewByte = (uint8_t)(u8_NewByte << 1U);
  }

  return u16_CRCValue;
}

uint16_t phscaNcj29d6_CalculateCrc16Sw(const uint8_t const u8arr_Data[], const uint16_t u16_DataLength)
{
  uint16_t u16_CRCValue = PHSCATYPES_u16_MIN_U16;
  uint16_t u16_AuxiliaryState = PHSCATYPES_u16_MIN_U16;

  /* u16_CRCValue = 0x0000; //Initialization of crc to 0x0000 for DNP */
  /* u16_CRCValue = 0xFFFF; //Initialization of crc to 0xFFFF for CCITT */
  while(u16_AuxiliaryState < u16_DataLength)
  {
    u16_CRCValue = phscaNcj29d6_CalculateCrc16Byte(u16_CRCValue, u8arr_Data[u16_AuxiliaryState]);
    u16_AuxiliaryState++;
  }

  return u16_CRCValue;
}
