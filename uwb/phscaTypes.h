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
 *    @file phscaTypes.h
 *   @brief Standard type definitions and bite-wise/byte-wise operations
 */

#ifndef PHSCATYPES_INCLUDE_GUARD
#define PHSCATYPES_INCLUDE_GUARD

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef PHSCATYPES_EXTERN_GUARD
	#define EXTERN /**/
#else
    #define EXTERN extern
#endif

/* =============================================================================
 * Type Definitions
 * ========================================================================== */
/** @brief Typedef of standard char to remind user about size of char */
typedef char char8_t;

/** @brief Typedef of standard float to remind user about size of float */
typedef float float32_t;

/** @brief Typedef of standard double to remind user about size of double */
typedef double float64_t;

/** @brief Standard status codes used throughout the framework. */
typedef enum
{
	PHSCATYPES_STATUS_OK = 0x00000000ul,
	PHSCATYPES_STATUS_ERROR = 0x00000001ul,
	PHSCATYPES_STATUS_UNDEFINED = 0x00000002ul,
	PHSCATYPES_STATUS_BAD_PARAMETER = 0x00000003ul,
	PHSCATYPES_STATUS_NOT_IMPLEMENTED = 0x00000004ul,
	PHSCATYPES_STATUS_TIMEOUT = 0x00000005ul,
	PHSCATYPES_STATUS_NO_RESPONSE = 0x00000006ul,
} phscaTypes_en_Status_t;

/* =============================================================================
 * Symbol Defines
 * ========================================================================== */
/** @brief Standard TRUE */
#define PHSCATYPES_b_TRUE                          (bool)(1u)
/** @brief Standard FALSE */
#define PHSCATYPES_b_FALSE                         (bool)(0u)

/** @brief Minimum value of a uint8_t variable */
#define PHSCATYPES_u8_MIN_U8                       (uint8_t)(0u)
/** @brief Maximum value of a uint8_t variable */
#define PHSCATYPES_u8_MAX_U8                       (uint8_t)(255u)

/** @brief Minimum value of an s8 variable */
#define PHSCATYPES_i8_MIN_I8                       (int8_t)(-127)
/** @brief Zero value of an s8 variable */
#define PHSCATYPES_i8_NUL_I8                       (int8_t)(0)
/** @brief Maximum value of an s8 variable */
#define PHSCATYPES_i8_MAX_I8                       (int8_t)(127)

/** @brief Minimum value of an uint16_t variable */
#define PHSCATYPES_u16_MIN_U16                     (uint16_t)(0u)
/** @brief Maximum value of an uint16_t variable */
#define PHSCATYPES_u16_MAX_U16                     (uint16_t)(65535u)

/** @brief Minimum value of an s16 variable */
#define PHSCATYPES_i16_MIN_I16                     (int16_t)(-32767)
/** @brief Zero value of an s16 variable */
#define PHSCATYPES_i16_NUL_I16                     (int16_t)(0)
/** @brief Maximum value of an s16 variable */
#define PHSCATYPES_i16_MAX_I16                     (int16_t)(32767)

/** @brief Minimum value of an uint32_t variable */
#define PHSCATYPES_u32_MIN_U32                     (uint32_t)(0ul)
/** @brief Maximum value of an uint32_t variable */
#define PHSCATYPES_u32_MAX_U32                     (uint32_t)(4294967295ul)

/** @brief Minimum value of an s32 variable */
#define PHSCATYPES_i32_MIN_I32                     (int32_t)(-2147483647l)
/** @brief Zero value of an s32 variable */
#define PHSCATYPES_i32_NUL_I32                     (int32_t)(0l)
/** @brief Maximum value of an s32 variable */
#define PHSCATYPES_i32_MAX_I32                     (int32_t)(2147483647l)

/** @brief Minimum value of an u64 variable */
#define PHSCATYPES_u64_MIN_U64                     (uint64_t)(0ull)
/** @brief Maximum value of an u64 variable */
#define PHSCATYPES_u64_MAX_U64                     (uint64_t)(18446744073709551615ull)

/** @brief Minimum value of an s64 variable */
#define PHSCATYPES_i64_MIN_I64                     (uint64_t)(-9223372036854775807ll)
/** @brief Zero value of an s64 variable */
#define PHSCATYPES_i64_NUL_I64                     (uint64_t)(0ll)
/** @brief Maximum value of an s64 variable */
#define PHSCATYPES_i64_MAX_I64                     (uint64_t)(9223372036854775807ll)

/** @brief Mask to access the lower nibble of a uint8_t variable */
#define PHSCATYPES_u8_MASK_LOW_NIBBLE              (uint8_t)(0x0Fu)
/** @brief Mask to access the higher nibble of a uint8_t variable */
#define PHSCATYPES_u8_MASK_HIGH_NIBBLE             (uint8_t)(0xF0u)

/** @brief Mask to access the lower byte of a uint16_t variable */
#define PHSCATYPES_u16_MASK_LOW_BYTE               (uint16_t)(0x00FFu)
/** @brief Mask to access the higher byte of a uint16_t variable */
#define PHSCATYPES_u16_MASK_HIGH_BYTE              (uint16_t)(0xFF00u)

/** @brief Mask to access byte 0 of a uint32_t variable */
#define PHSCATYPES_u32_MASK_BYTE0                  (uint32_t)(0x000000FFul)
/** @brief Mask to access byte 1 of a uint32_t variable */
#define PHSCATYPES_u32_MASK_BYTE1                  (uint32_t)(0x0000FF00ul)
/** @brief Mask to access byte 2 of a uint32_t variable */
#define PHSCATYPES_u32_MASK_BYTE2                  (uint32_t)(0x00FF0000ul)
/** @brief Mask to access byte 3 of a uint32_t variable */
#define PHSCATYPES_u32_MASK_BYTE3                  (uint32_t)(0xFF000000ul)

/** @brief Mask to access byte 0 of a u64 variable */
#define PHSCATYPES_u64_MASK_BYTE0                  (uint64_t)(0x00000000000000FFull)
/** @brief Mask to access byte 1 of a u64 variable */
#define PHSCATYPES_u64_MASK_BYTE1                  (uint64_t)(0x000000000000FF00ull)
/** @brief Mask to access byte 2 of a u64 variable */
#define PHSCATYPES_u64_MASK_BYTE2                  (uint64_t)(0x0000000000FF0000ull)
/** @brief Mask to access byte 3 of a u64 variable */
#define PHSCATYPES_u64_MASK_BYTE3                  (uint64_t)(0x00000000FF000000ull)
/** @brief Mask to access byte 4 of a u64 variable */
#define PHSCATYPES_u64_MASK_BYTE4                  (uint64_t)(0x000000FF00000000ull)
/** @brief Mask to access byte 5 of a u64 variable */
#define PHSCATYPES_u64_MASK_BYTE5                  (uint64_t)(0x0000FF0000000000ull)
/** @brief Mask to access byte 6 of a u64 variable */
#define PHSCATYPES_u64_MASK_BYTE6                  (uint64_t)(0x00FF000000000000ull)
/** @brief Mask to access byte 7 of a u64 variable */
#define PHSCATYPES_u64_MASK_BYTE7                  (uint64_t)(0xFF00000000000000ull)

/** @brief Mask to access bit 0 of a uint8_t variable */
#define PHSCATYPES_u8_MASK_BIT_0                   (uint8_t)(0x01u)
/** @brief Mask to access bit 1 of a uint8_t variable */
#define PHSCATYPES_u8_MASK_BIT_1                   (uint8_t)(0x02u)
/** @brief Mask to access bit 2 of a uint8_t variable */
#define PHSCATYPES_u8_MASK_BIT_2                   (uint8_t)(0x04u)
/** @brief Mask to access bit 3 of a uint8_t variable */
#define PHSCATYPES_u8_MASK_BIT_3                   (uint8_t)(0x08u)
/** @brief Mask to access bit 4 of a uint8_t variable */
#define PHSCATYPES_u8_MASK_BIT_4                   (uint8_t)(0x10u)
/** @brief Mask to access bit 5 of a uint8_t variable */
#define PHSCATYPES_u8_MASK_BIT_5                  (uint8_t)(0x20u)
/** @brief Mask to access bit 6 of a uint8_t variable */
#define PHSCATYPES_u8_MASK_BIT_6                   (uint8_t)(0x40u)
/** @brief Mask to access bit 7 of a uint8_t variable */
#define PHSCATYPES_u8_MASK_BIT_7                  (uint8_t)(0x80u)

/** @brief Standard number of bits in one byte */
#define PHSCATYPES_u8_BITS_IN_ONE_BYTE             (uint8_t)(8u)

/** @brief Null pointer definition */
#define PHSCATYPES_pv_NULLPTR                      (void*)(0u)

/** @brief Number of milliseconds in 1 second */
#define PHSCATYPES_u16_MILLISEC_IN_ONE_SECOND      (uint16_t)(1000u)

/** @brief Number of 10 milliseconds in 1 second */
#define PHSCATYPES_u16_TEN_MILLISEC_IN_ONE_SECOND  (uint16_t)(100u)

/** @brief Number of microseconds in 1 second */
#define PHSCATYPES_u32_MICROSEC_IN_ONE_SECOND      (uint32_t)(1000000ul)

/** @brief 1-byte shifting */
#define PHSCATYPES_u8_ONE_BYTE_SHIFT               (uint8_t)(8u)
/** @brief 2-byte shifting */
#define PHSCATYPES_u8_TWO_BYTE_SHIFT               (uint8_t)(16u)
/** @brief 3-byte shifting */
#define PHSCATYPES_u8_THREE_BYTE_SHIFT             (uint8_t)(24u)
/** @brief 4-byte shifting */
#define PHSCATYPES_u8_FOUR_BYTE_SHIFT              (uint8_t)(32u)
/** @brief 5-byte shifting */
#define PHSCATYPES_u8_FIVE_BYTE_SHIFT              (uint8_t)(40u)
/** @brief 6-byte shifting */
#define PHSCATYPES_u8_SIX_BYTE_SHIFT               (uint8_t)(48u)
/** @brief 7-byte shifting */
#define PHSCATYPES_u8_SEVEN_BYTE_SHIFT             (uint8_t)(56u)

/* Compiler specific options. Only GCC currently supported! */
/* Naming scheme: atr -> attribute */
#define PHSCATYPES_atr_GCC_PACK_STRUCT              __attribute__((packed))
#define PHSCATYPES_atr_GCC_CODE_SECTION_RAM         __attribute__((section (".code_ram")))
#define PHSCATYPES_atr_GCC_CODE_SECTION_EEEPROM     __attribute__((section(".eeeprom")))
#define PHSCATYPES_atr_GCC_UNUSED_SYMBOL           __attribute__ ((unused))

/* =============================================================================
 * Public Function-like Macros
 * ========================================================================== */
/** @brief Generic macro to get the absolute value of a input signed variable
 *  This particular macro has no specified output data type in the macro name */
#define PHSCATYPES_ABS(SignedInput)                (((SignedInput) < 0) ? -(SignedInput) : (SignedInput))

/* =============================================================================
 * Public Standard Enumerators
 * ========================================================================== */

/* =============================================================================
 * Public Function Prototypes
 * ========================================================================== */
/** @brief Gets the low byte of the input uint16_t value
 * @param u16_Input16Bit uint16_t value
 * @return output low byte of the uint16_t value */
uint8_t phscaTypes_GetLowByte(const uint16_t u16_Input16Bit);

/** @brief Gets the high byte of the input uint16_t value
 * @param u16_Input16Bit uint16_t value
 * @return output high byte of the uint16_t value */
uint8_t phscaTypes_GetHighByte(const uint16_t u16_Input16Bit);

/** @brief Gets the low nibble (4-bits) of the input uint8_t value
 * @param u8_InputByte uint8_t value
 * @return output low nibble of the uint8_t value */
uint8_t phscaTypes_GetLowNibble(const uint8_t u8_InputByte);

/** @brief Gets the high nibble (4-bits) of the input uint8_t value
 * @param u8_InputByte uint8_t value
 * @return output high nibble of the uint8_t value */
uint8_t phscaTypes_GetHighNibble(const uint8_t u8_InputByte);

/** @brief Sets the low nibble (4-bits) of the input uint8_t value
 * @param pu8_Variable pointer to the uint8_t value to be set
 * @param u8_LowNibble low nibble to be set */
void phscaTypes_SetLowNibble(uint8_t * const pu8_Variable, const uint8_t u8_LowNibble);

/** @brief Sets the high nibble (4-bits) of the input uint8_t value
 * @param pu8_Variable pointer to the uint8_t value to be set
 * @param u8_HighNibble high nibble to be set */
void phscaTypes_SetHighNibble(uint8_t * const pu8_Variable, const uint8_t u8_HighNibble);

/** @brief Sets the low byte of the input uint16_t value
 * @param pu16_Variable pointer to the uint16_t value to be set
 * @param u8_LowByte low byte to be set */
void phscaTypes_SetLowByte(uint16_t * const pu16_Variable, const uint8_t u8_LowByte);

/** @brief Sets the high byte of the input uint16_t value
 * @param pu16_Variable pointer to the uint16_t value to be set
 * @param u8_HighByte high byte to be set */
void phscaTypes_SetHighByte(uint16_t * const pu16_Variable, const uint8_t u8_HighByte);

/** @brief Reads one bit of the input value based on the input bit mask
 * @param u32_Input32Bit value to be read
 * @param u32_InputBitMask mask for the input value
 * @return true -> bit is set, false -> is not set */
bool phscaTypes_ReadBit(const uint32_t u32_Input32Bit, const uint32_t u32_InputBitMask);

/** @brief Sets bits of the input value based on the input bit mask
 * @param pu32_Input32Bit value to be set
 * @param u32_InputBitMask mask for the input value */
void phscaTypes_SetBit(uint32_t * const pu32_Input32Bit, const uint32_t u32_InputBitMask);

/** @brief Clears bits of the input value based on the input bit mask
 * @param pu32_Input32Bit value to be cleared
 * @param u32_InputBitMask mask for the input value */
void phscaTypes_ClearBit(uint32_t * const pu32_Input32Bit, const uint32_t u32_InputBitMask);

/** @brief Toggles bits of the input value based on the input bit mask
 * @param pu32_Input32Bit value to be toggled
 * @param u32_InputBitMask mask for the input value */
void phscaTypes_ToggleBit(uint32_t * constpu32_Input32Bit, const uint32_t u32_InputBitMask);

/** @brief Sets a complete mask of bits of the input value based on the input bit mask
 * @param pu32_Input32Bit value to be set
 * @param u32_InputMask mask for the input value */
void phscaTypes_SetMask(uint32_t * const pu32_Input32Bit, const uint32_t u32_InputMask);

/** @brief Reads a complete mask of bits of the input value based on the input bit mask
 * @param u32_Input32Bit value to be read
 * @param u32_InputMask mask for the input value
 * @return masked value of the input data */
uint32_t phscaTypes_ReadMask(const uint32_t u32_Input32Bit, const uint32_t u32_InputMask);

/** @brief Splits a uint32_t variable into four uint8_t variables
 * @param       u32_Input32Bit   Input uint32_t variable
 * @param       pu8_OutputByte0 Output pointer to lower byte of uint32_t value
 * @param       pu8_OutputByte1 Output pointer to second byte of uint32_t value
 * @param       pu8_OutputByte2 Output pointer to third byte of uint32_t value
 * @param       pu8_OutputByte3 Output pointer to upper byte of uint32_t value */
void phscaTypes_ConvertU32toU8(const uint32_t u32_Input32Bit, uint8_t * const pu8_OutputByte0, uint8_t * const pu8_OutputByte1, uint8_t * const pu8_OutputByte2, uint8_t * const pu8_OutputByte3);

/** @brief Splits a uint64_t variable into eight uint8_t variables
 * @param       u32In   Input uint32_t variable
 * @param       pu8_OutputByte0 Output pointer to lower byte of uint64_t value
 * @param       pu8_OutputByte1 Output pointer to second byte of uint64_t value
 * @param       pu8_OutputByte2 Output pointer to third byte of uint64_t value
 * @param       pu8_OutputByte3 Output pointer to fourth byte of uint64_t value
 * @param       pu8_OutputByte4 Output pointer to fifth byte of uint64_t value
 * @param       pu8_OutputByte5 Output pointer to sixth byte of uint64_t value
 * @param       pu8_OutputByte6 Output pointer to seventh byte of uint64_t value
 * @param       pu8_OutputByte7 Output pointer to upper (eighth) byte of uint64_t value */
void phscaTypes_ConvertU64toU8(const uint64_t u64_Input64Bit, uint8_t * const pu8_OutputByte0, uint8_t * const pu8_OutputByte1, uint8_t * const pu8_OutputByte2, uint8_t * const pu8_OutputByte3, uint8_t * const pu8_OutputByte4, uint8_t * const pu8_OutputByte5, uint8_t * const pu8_OutputByte6, uint8_t * const pu8_OutputByte7);

/** @brief Concatenates four uint8_t variables into one uint32_t variable
 * @param   u8_InputByte0 Lower uint32_t byte
 * @param   u8_InputByte1 Second uint32_t byte
 * @param   u8_InputByte2 Third uint32_t byte
 * @param   u8_InputByte3 Upper uint32_t byte
 * @return Concatenated uint32_t variable */
uint32_t phscaTypes_ConvertU8toU32(const uint8_t u8_InputByte0, const uint8_t u8_InputByte1, const uint8_t u8_InputByte2, const uint8_t u8_InputByte3);

/** @brief Concatenates eight uint8_t variables into one uint64_t variable
 * @param   u8_InputByte0 Lower byte
 * @param   u8_InputByte1 Second byte
 * @param   u8_InputByte2 Third byte
 * @param   u8_InputByte3 fourth byte
 * @param   u8_InputByte4 fifth byte
 * @param   u8_InputByte5 sixth byte
 * @param   u8_InputByte6 seventh byte
 * @param   u8_InputByte7 Upper (eighth) byte
 * @return Concatenated uint64_t variable */
uint64_t phscaTypes_ConvertU8toU64(const uint8_t u8_InputByte0, const uint8_t u8_InputByte1, const uint8_t u8_InputByte2, const uint8_t u8_InputByte3, const uint8_t u8_InputByte4, const uint8_t u8_InputByte5, const uint8_t u8_InputByte6, const uint8_t u8_InputByte7);

/** @brief Splits a uint16_t variable into two uint8_t variables
 * @param u16_Input16Bit   Input uint16_t variable
 * @param pu8_OutputByte0 Output pointer to lower byte of uint16_t byte
 * @param pu8_OutputByte1 Output pointer to upper byte of uint16_t byte */
void phscaTypes_ConvertU16toU8(const uint16_t u16_Input16Bit, uint8_t * const pu8_OutputByte0, uint8_t * const pu8_OutputByte1);

/** @brief Concatenates two uint8_t variables into one uint16_t variables
 * @param u8_InputByte0 Lower uint16_t byte
 * @param u8_InputByte1 Upper uint16_t byte
 * @return Concatenated uint16_t variable */
uint16_t phscaTypes_ConvertU8toU16(const uint8_t u8_InputByte0, const uint8_t u8_InputByte1);

/** @brief Converts float64_t into uint64_t IEEE representation
 * @param d_InputDoubleToBeSerialized float64_t value to be serialized into uint64_t value
 * @return output uint64_t variable */
uint64_t phscaTypes_ConvertFloat64toU64(const float64_t d_InputDoubleToBeSerialized);

/** @brief Converts uint64_t into float64_t IEEE representation
 * @param u64_Input64BitToBeDeserialized uint64_t value to be converted
 * @return output float64_t variable */
float64_t phscaTypes_ConvertU64toFloat64(const uint64_t u64_Input64BitToBeDeserialized);

/** @brief Converts uint32_t into float32_t IEEE representation
 * @param u32_Input32BitToBeDeserialized uint32_t value to be converted
 * @return output float32_t variable */
float32_t phscaTypes_ConvertU32toFloat32(const uint32_t u32_Input32BitToBeDeserialized);

/** @brief Toggles the input boolean value
 * @param b_InputBoolean input boolean to be toggled
 * @return output toggled bool variable */
bool phscaTypes_ToggleBoolean(const bool b_InputBoolean);

#undef EXTERN
#endif
