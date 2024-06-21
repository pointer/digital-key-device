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
 *    @file: phscaUci_Cfg.c
 *   @brief: Configuration of ranging application driver for NCJ29D6
 */

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include "phscaTypes.h"
#include "phscaNcj29d6.h"
#include <stdarg.h>
#include <stdio.h>

/* =============================================================================
 * Internal Includes
 * ========================================================================== */
#define PHSCAUCI_CFG_EXTERN_GUARD
#include "phscaUci_Cfg.h"
#undef PHSCAUCI_CFG_EXTERN_GUARD

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
void phscaUci_InitDevice(void (*pf_IntCallback)(void))
{
	phscaNcj29d6_Init(pf_IntCallback);
}

void phscaUci_Transceive(const uint32_t u32_DataLength, const uint8_t const u8arr_DataToTransmit[], uint8_t u8arr_DataReceived[])
{
	phscaNcj29d6_SpiTransceive(u32_DataLength, u8arr_DataToTransmit, u8arr_DataReceived);
}

bool phscaUci_IsResponseAvailable(void)
{
	bool b_Ncj295InterruptLine = PHSCATYPES_b_FALSE;

	b_Ncj295InterruptLine = phscaNcj29d6_GetInt();

	return !b_Ncj295InterruptLine;
}

void phscaUci_StartCommandTx(void)
{
	bool b_NotReady = PHSCATYPES_b_TRUE;

	/* Pull chip select low */
	phscaNcj29d6_SetCs(PHSCATYPES_b_TRUE);

	/* Wait for Ready to go low */
	do
	{
		b_NotReady = PHSCATYPES_b_TRUE;
		b_NotReady = phscaNcj29d6_GetRdy();
	}
	while(b_NotReady == PHSCATYPES_b_TRUE);
}

void phscaUci_StopCommandTx(void)
{
	bool b_NotReady = PHSCATYPES_b_TRUE;

	/* Pull chip select up */
	phscaNcj29d6_SetCs(PHSCATYPES_b_FALSE);

	/* UCI command: wait for RDY to go high */
	do
	{
		b_NotReady = PHSCATYPES_b_FALSE;
		b_NotReady = phscaNcj29d6_GetRdy();
	}
	while(b_NotReady == PHSCATYPES_b_FALSE);
}

void phscaUci_StartResponseTx(void)
{
	/* Pull chip select low */
	phscaNcj29d6_SetCs(PHSCATYPES_b_TRUE);
}

void phscaUci_StopResponseTx(void)
{
	bool b_NotReady = PHSCATYPES_b_FALSE;

	/* UCI response: wait for INT to go high */
	do
	{
		b_NotReady = PHSCATYPES_b_FALSE;
		b_NotReady = phscaNcj29d6_GetInt();
	}
	while(b_NotReady == PHSCATYPES_b_FALSE);


	/* Pull chip select up */
	phscaNcj29d6_SetCs(PHSCATYPES_b_FALSE);
}
