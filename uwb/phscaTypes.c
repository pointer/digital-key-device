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
 *    @file: phscaTypes.c
 *   @brief: Standard type definitions and bite-wise/byte-wise operations
 */

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include <string.h>

/* =============================================================================
 * Internal Includes
 * ========================================================================== */
#define PHSCATYPES_EXTERN_GUARD
#include "phscaTypes.h"
#undef PHSCATYPES_EXTERN_GUARD

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

/* =============================================================================
 * Private Module-wide Visible Variables
 * ========================================================================== */

/* =============================================================================
 * Function Definitions
 * ========================================================================== */
inline uint8_t phscaTypes_GetLowByte(const uint16_t u16_Input16Bit)
{
  uint8_t u8_LowByte = PHSCATYPES_u8_MIN_U8;

  u8_LowByte = (uint8_t)(u16_Input16Bit & PHSCATYPES_u16_MASK_LOW_BYTE );

  return u8_LowByte;
}

inline uint8_t phscaTypes_GetHighByte(const uint16_t u16_Input16Bit)
{
  uint8_t u8_HighByte = PHSCATYPES_u8_MIN_U8;

  u8_HighByte = (uint8_t)(((uint16_t)u16_Input16Bit & PHSCATYPES_u16_MASK_HIGH_BYTE) >> PHSCATYPES_u8_ONE_BYTE_SHIFT);

  return u8_HighByte;
}

inline uint8_t phscaTypes_GetLowNibble(const uint8_t u8_InputByte)
{
  uint8_t u8_LowNibble = PHSCATYPES_u8_MIN_U8;

  u8_LowNibble = (uint8_t)(u8_InputByte & PHSCATYPES_u8_MASK_LOW_NIBBLE );

  return u8_LowNibble;
}

inline uint8_t phscaTypes_GetHighNibble(const uint8_t u8_InputByte)
{
  uint8_t u8_HighNibble = PHSCATYPES_u8_MIN_U8;

  u8_HighNibble = (uint8_t)(u8_InputByte & PHSCATYPES_u8_MASK_HIGH_NIBBLE );

  return u8_HighNibble;
}

inline void phscaTypes_SetLowNibble(uint8_t * const pu8_Variable, const uint8_t u8_LowNibble)
{
  *pu8_Variable = (uint8_t)( (uint8_t)(*pu8_Variable & PHSCATYPES_u8_MASK_HIGH_NIBBLE) | (uint8_t)(u8_LowNibble & PHSCATYPES_u8_MASK_LOW_NIBBLE) );
}

inline void phscaTypes_SetHighNibble(uint8_t * const pu8_Variable, const uint8_t u8_HighNibble)
{
  *pu8_Variable = (uint8_t)( (uint8_t)(*pu8_Variable & PHSCATYPES_u8_MASK_LOW_NIBBLE) | (uint8_t)((uint8_t)(u8_HighNibble << 4u) & PHSCATYPES_u8_MASK_HIGH_NIBBLE) );
}

inline void phscaTypes_SetLowByte(uint16_t * const pu16_Variable, const uint8_t u8_LowByte)
{
  *pu16_Variable = (uint16_t)( (uint16_t)(*pu16_Variable & (uint16_t)PHSCATYPES_u16_MASK_HIGH_BYTE) | (uint16_t)(u8_LowByte) );
}

inline void phscaTypes_SetHighByte(uint16_t * const pu16_Variable, const uint8_t u8_HighByte)
{
  *pu16_Variable = (uint16_t)( (uint16_t)(*pu16_Variable & (uint16_t)PHSCATYPES_u16_MASK_LOW_BYTE) | (uint16_t)(u8_HighByte << (uint8_t)8u) );
}

inline bool phscaTypes_ReadBit(const uint32_t u32_Input32Bit, const uint32_t u32_InputBitMask)
{
  bool b_BitValue = PHSCATYPES_b_FALSE;

  b_BitValue = (bool)(u32_Input32Bit & u32_InputBitMask);

  return b_BitValue;
}

inline void phscaTypes_SetBit(uint32_t * const pu32_Input32Bit, const uint32_t u32_InputBitMask)
{
	*pu32_Input32Bit |= u32_InputBitMask;
}

inline void phscaTypes_ClearBit(uint32_t * const pu32_Input32Bit, const uint32_t u32_InputBitMask)
{
	*pu32_Input32Bit &= (uint32_t)(~(u32_InputBitMask));
}

inline void phscaTypes_ToggleBit(uint32_t * const pu32_Input32Bit, const uint32_t u32_InputBitMask)
{
	*pu32_Input32Bit ^= u32_InputBitMask;
}

inline void phscaTypes_SetMask(uint32_t * const pu32_Input32Bit, const uint32_t u32_InputMask)
{
	/* Clear mask first to remove any 1 bits */
	*pu32_Input32Bit &= (uint8_t)(~(u32_InputMask));
	/* Then set mask */
	*pu32_Input32Bit |= u32_InputMask;
}

inline uint32_t phscaTypes_ReadMask(const uint32_t u32_Input32Bit, const uint32_t u32_InputMask)
{
	uint32_t u32_ReadMask = PHSCATYPES_u32_MIN_U32;

	u32_ReadMask = u32_Input32Bit & u32_InputMask;

	return u32_ReadMask;
}

inline void phscaTypes_ConvertU32toU8(const uint32_t u32_Input32Bit, uint8_t * const pu8_OutputByte0,
    uint8_t * const pu8_OutputByte1, uint8_t * const pu8_OutputByte2, uint8_t * const pu8_OutputByte3)
{
  *pu8_OutputByte0 = (uint8_t)((u32_Input32Bit & PHSCATYPES_u32_MASK_BYTE0 ));
  *pu8_OutputByte1 = (uint8_t)((u32_Input32Bit & PHSCATYPES_u32_MASK_BYTE1 )
      >> PHSCATYPES_u8_ONE_BYTE_SHIFT );
  *pu8_OutputByte2 = (uint8_t)((u32_Input32Bit & PHSCATYPES_u32_MASK_BYTE2 )
      >> PHSCATYPES_u8_TWO_BYTE_SHIFT );
  *pu8_OutputByte3 = (uint8_t)((u32_Input32Bit & PHSCATYPES_u32_MASK_BYTE3 )
      >> PHSCATYPES_u8_THREE_BYTE_SHIFT );
}

inline void phscaTypes_ConvertU64toU8(const uint64_t u64_Input64Bit, uint8_t * const pu8_OutputByte0,
    uint8_t * const pu8_OutputByte1, uint8_t * const pu8_OutputByte2, uint8_t * const pu8_OutputByte3,
    uint8_t * const pu8_OutputByte4, uint8_t * const pu8_OutputByte5, uint8_t * const pu8_OutputByte6,
    uint8_t * const pu8_OutputByte7)
{
  *pu8_OutputByte0 = (uint8_t)((u64_Input64Bit & PHSCATYPES_u64_MASK_BYTE0 ));
  *pu8_OutputByte1 = (uint8_t)((u64_Input64Bit & PHSCATYPES_u64_MASK_BYTE1 )
      >> PHSCATYPES_u8_ONE_BYTE_SHIFT );
  *pu8_OutputByte2 = (uint8_t)((u64_Input64Bit & PHSCATYPES_u64_MASK_BYTE2 )
      >> PHSCATYPES_u8_TWO_BYTE_SHIFT );
  *pu8_OutputByte3 = (uint8_t)((u64_Input64Bit & PHSCATYPES_u64_MASK_BYTE3 )
      >> PHSCATYPES_u8_THREE_BYTE_SHIFT );
  *pu8_OutputByte4 = (uint8_t)((u64_Input64Bit & PHSCATYPES_u64_MASK_BYTE4 )
      >> PHSCATYPES_u8_FOUR_BYTE_SHIFT );
  *pu8_OutputByte5 = (uint8_t)((u64_Input64Bit & PHSCATYPES_u64_MASK_BYTE5 )
      >> PHSCATYPES_u8_FIVE_BYTE_SHIFT );
  *pu8_OutputByte6 = (uint8_t)((u64_Input64Bit & PHSCATYPES_u64_MASK_BYTE6 )
      >> PHSCATYPES_u8_SIX_BYTE_SHIFT );
  *pu8_OutputByte7 = (uint8_t)((u64_Input64Bit & PHSCATYPES_u64_MASK_BYTE7 )
      >> PHSCATYPES_u8_SEVEN_BYTE_SHIFT );
}

inline uint32_t phscaTypes_ConvertU8toU32(const uint8_t u8_InputByte0, const uint8_t u8_InputByte1,
		const uint8_t u8_InputByte2, const uint8_t u8_InputByte3)
{
  uint32_t u32Output = PHSCATYPES_u32_MIN_U32;

  u32Output = (uint32_t)(u8_InputByte0) + ((uint32_t)(u8_InputByte1) << PHSCATYPES_u8_ONE_BYTE_SHIFT )
      + ((uint32_t)(u8_InputByte2) << PHSCATYPES_u8_TWO_BYTE_SHIFT )
      + ((uint32_t)(u8_InputByte3) << PHSCATYPES_u8_THREE_BYTE_SHIFT );

  return (u32Output);
}

inline uint64_t phscaTypes_ConvertU8toU64(const uint8_t u8_InputByte0, const uint8_t u8_InputByte1,
		const uint8_t u8_InputByte2, const uint8_t u8_InputByte3, const uint8_t u8_InputByte4, const uint8_t u8_InputByte5,
		const uint8_t u8_InputByte6,const  uint8_t u8_InputByte7)
{
  uint64_t u64_Output = PHSCATYPES_u64_MIN_U64;

  u64_Output = (uint64_t)(u8_InputByte0)
      + ((uint64_t)(u8_InputByte1) << PHSCATYPES_u8_ONE_BYTE_SHIFT )
      + ((uint64_t)(u8_InputByte2) << PHSCATYPES_u8_TWO_BYTE_SHIFT )
      + ((uint64_t)(u8_InputByte3) << PHSCATYPES_u8_THREE_BYTE_SHIFT )
      + ((uint64_t)(u8_InputByte4) << PHSCATYPES_u8_FOUR_BYTE_SHIFT )
      + ((uint64_t)(u8_InputByte5) << PHSCATYPES_u8_FIVE_BYTE_SHIFT )
      + ((uint64_t)(u8_InputByte6) << PHSCATYPES_u8_SIX_BYTE_SHIFT )
      + ((uint64_t)(u8_InputByte7) << PHSCATYPES_u8_SEVEN_BYTE_SHIFT );

  return (u64_Output);
}

inline void phscaTypes_ConvertU16toU8(const uint16_t u16Input16Bit, uint8_t * const pu8_OutputByte0,
    uint8_t * const pu8_OutputByte1)
{
  *pu8_OutputByte0 = (uint8_t)(u16Input16Bit & PHSCATYPES_u16_MASK_LOW_BYTE );
  *pu8_OutputByte1 = (uint8_t)((u16Input16Bit & PHSCATYPES_u16_MASK_HIGH_BYTE )
      >> PHSCATYPES_u8_ONE_BYTE_SHIFT );
}

inline uint16_t phscaTypes_ConvertU8toU16(const uint8_t u8_InputByte0, const uint8_t u8_InputByte1)
{
  uint16_t u16_Output = PHSCATYPES_u16_MIN_U16;

  u16_Output = (uint16_t)((uint16_t)(u8_InputByte0)
      + (uint16_t)((u8_InputByte1) << PHSCATYPES_u8_ONE_BYTE_SHIFT ));

  return (u16_Output);
}

inline uint64_t phscaTypes_ConvertFloat64toU64(const float64_t d_InputDoubleToBeSerialized)
{
  uint64_t u64SerializedDouble = PHSCATYPES_u64_MIN_U64;

  memcpy((void*)&u64SerializedDouble, (void*)&d_InputDoubleToBeSerialized,
      sizeof(d_InputDoubleToBeSerialized));

  return u64SerializedDouble;
}

inline float64_t phscaTypes_ConvertU64toFloat64(const uint64_t u64_Input64BitToBeDeserialized)
{
  float64_t d_RecreatedDouble = 0.0;

  memcpy((void*)&d_RecreatedDouble, (void*)&u64_Input64BitToBeDeserialized,
      sizeof(d_RecreatedDouble));

  return d_RecreatedDouble;
}

inline float32_t phscaTypes_ConvertU32toFloat32(const uint32_t u32_Input32BitToBeDeserialized)
{
  float32_t d_RecreatedFloat = 0.0;

  memcpy((void*)&d_RecreatedFloat, (void*)&u32_Input32BitToBeDeserialized,
      sizeof(d_RecreatedFloat));

  return d_RecreatedFloat;
}

inline bool phscaTypes_ToggleBoolean(const bool b_InputBoolean)
{
  bool b_OutputBoolean = b_InputBoolean;

  b_OutputBoolean ^= (bool)PHSCATYPES_b_TRUE;

  return b_OutputBoolean;
}
