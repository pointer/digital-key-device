/*
 (c) NXP B.V. 2022. All rights reserved.

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
 *    @file: phscaUwb.h
 *   @brief: Upper layer UWB application
 */

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include "phscaTypes.h"
#include "phscaUci.h"
#include "phscaNcj29d6.h"


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "fsl_os_abstraction.h"
#include "fsl_debug_console.h"
#include "trace.h"
#include "app.h"
#include "app_nvm.h"

/* =============================================================================
 * Internal Includes
 * ========================================================================== */
#define PHSCAUWB_EXTERN_GUARD
#include "phscaUwb.h"
#undef PHSCAUWB_EXTERN_GUARD

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
static void phscaUwb_MacHostInit(bool EnableReset);
static void phscaUwb_MacHostPeriodicTask(TickType_t * const xLastWakeTime);
static void phscaUwb_MacHostUciRspNtfCallback(const uint8_t u8_MessageType, const uint8_t u8_Gid, const uint8_t u8_Oid, const uint32_t u32_PayloadLength, const uint8_t * const u8arr_Payload);
//static void phscaUwb_Start(void);
void phscaUwb_Reset(void);
static void phscaUwb_Stop(void);
static void phscaUwb_Resume(void);
/* =============================================================================
 * Private Module-wide Visible Variables
 * ========================================================================== */
static OSA_SEMAPHORE_HANDLE_DEFINE(m_pst_InterruptPinSemaphore);
static uint8_t response[500];

/* =============================================================================
 * Function Definitions
 * ========================================================================== */
void phscaUwb_Init(void)
{

	phscaUwb_MacHostInit(true);
}

void phscaUwb_PeriodicTask(TickType_t * const xLastWakeTime)
{
	phscaUwb_MacHostPeriodicTask(xLastWakeTime);
}

void phscaUwb_Starting(void)
{
	//phscaUwb_Start();
}

void phscaUwb_Stopping(void)
{
	phscaUwb_Stop();
}

void phscaUwb_Resuming(void)
{
	phscaUwb_Resume();
}
static void phscaUwb_MacHostInit(bool EnableReset)
{
    systemParameters_t *pSysParams = NULL;
    App_NvmReadSystemParams(&pSysParams);

	/* Initialize Ranging Application */
	TRACE_INFO("Ranger 5 Init start\r\n");
	phscaUci_Init(PHSCATYPES_pv_NULLPTR);
	phscaNcj29d6_SetIntPinInterruptEnable(PHSCATYPES_b_FALSE);

	uint32_t payloadLength = 0u;

	if(EnableReset==true)
	{
	/* Hard reset using RST_N pin */
	phscaNcj29d6_HardReset(1u, 1u);

	/* Read BOOT_STATUS_NTF */
	do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
	}

	static uint8_t getDeviceInfoCmd[] = {0x20u, 0x02u, 0x00u, 0x00u};
	/*TRACE_INFO("\r\nGetDeviceInfoCmd1\r\n");
	phscaUci_SendCommand(getDeviceInfoCmd, sizeof(getDeviceInfoCmd) - 4u);
	/* Read response of GetDeviceInfo */
	/*do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);*/

	static uint8_t coreSetCfgCmd[] = {0x20,0x04,0x00,0x04,0x01,0x01,0x01,0x00}; //disable low power mode Not implemented on R5
	static uint8_t resetTrimPageCmd[] = {0x2E,0x26,0x00,0x01,0x00}; //reset complete trim page
	static uint8_t sessionInitCmd[] = {0x21,0x00,0x00,0x05,0xCA,0xFE,0xFA,0xCE,0xA0};
	static uint8_t sessionSetAppCfgCmd[] = {0x21,0x03,0x00,0x30,
                          0x00,0x00,0x00,0xA0, // Session Handle instead of Session ID
                          0x0C, //# 12 parameters
                          0x04,0x01,0x09,  // # channel = 9
                          0x05,0x01,0x06,   //# number_of_anchors = 6 but will be changed to the default value in the NVM
                          0x08,0x02,0x60,0x09,  //# ranging_slot_length = 0x0960 = 2400 RSTU = 2ms
                          0x09,0x04,0x20,0x01,0x00,0x00, //# Ranging_interval = 0x60 = 96ms
                          0x14,0x01,0x09,   //# preamble_id = 10 to match Pramble default in radio configuration
                          0x15,0x01,0x02,   //# SFD_id = 2
                          0xF2,0x01,0x96,   //# Tx_power_id = 0x10 = 16 = 14 - 16/6 dBm
                          0x1B,0x01,0x0C,   //# slots_per_rr = 0xC = 12
                          0x00,0x01,0x01,   //# device_type = controler
                          0x11,0x01,0x01,   //# device_role = Initiator
                          0xA1,0x01,0x01,   //# CCC_CONFIG_QUIRKS = 1
                          0xA0,0x04,0x00,0x00,0x00,0x00, //# STS Index 0 = 0x00000000
	};
    sessionSetAppCfgCmd[14] =((pSysParams->system_params).fields.number_of_anchors); //change the number of anchors to the default value in the NVM
	static uint8_t setSTSindexRestart[] = {0x21,0x03,0x00,0x08,0x00,0x00,0x00,0xA0,0x01,0xF9,0x01,0x01};
	//static uint8_t sessionRangeStartCmd[] = {0x22,0x00,0x00,0x04,0x00,0x00,0x00,0xA0}; // Session Handle instead of Session ID
	osa_status_t en_SemaphoreWaitStatus;


	/* Reset complete trim page */
	/*TRACE_INFO("Reset complete trim page\r\n");
	phscaUci_SendCommand(resetTrimPageCmd, (uint32_t)sizeof(resetTrimPageCmd) - (uint32_t)PHSCAUCI_u8_UCI_HEADER_SIZE_BYTES);
	do
	{
		payloadLength = phscaUci_GetResponse(response);//Response
	} while(payloadLength == 0u);
		do
	{
		payloadLength = phscaUci_GetResponse(response);//Notification
	} while(payloadLength == 0u);

	/* Init CCC session */
	TRACE_INFO("Init CCC session\r\n");
	phscaUci_SendCommand(sessionInitCmd, (uint32_t)sizeof(sessionInitCmd) - (uint32_t)PHSCAUCI_u8_UCI_HEADER_SIZE_BYTES);
	do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
		do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);

	/* Set app cfg */
	TRACE_INFO("Set app cfg\r\n");
	phscaUci_SendCommand(sessionSetAppCfgCmd, (uint32_t)sizeof(sessionSetAppCfgCmd) - (uint32_t)PHSCAUCI_u8_UCI_HEADER_SIZE_BYTES);
	do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
		do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
		/* Set STS Index Restart */
			PRINTF("Set STS Index Restart\r\n");
			phscaUci_SendCommand(setSTSindexRestart, (uint32_t)sizeof(setSTSindexRestart) - (uint32_t)PHSCAUCI_u8_UCI_HEADER_SIZE_BYTES);
			do
			{
				payloadLength = phscaUci_GetResponse(response);
			} while(payloadLength == 0u);
			if((response[0] == 0x60)&&(response[1] == 0x01))
				{
					PRINTF("Mac host Init error sts index restart\r\n");
				}

			TRACE_INFO("Ranger 5 Init complete\r\n");

/*}

static void phscaUwb_Start(void)
{*/
	//uint32_t payloadLength = 0u;
	static uint8_t sessionRangeStartCmd[] = {0x22,0x00,0x00,0x04,0x00,0x00,0x00,0xA0}; // Session Handle instead of Session ID
	/* Range start */
	TRACE_INFO("Ranging starting ....\r\n");
	phscaUci_SendCommand(sessionRangeStartCmd, (uint32_t)sizeof(sessionRangeStartCmd) - (uint32_t)PHSCAUCI_u8_UCI_HEADER_SIZE_BYTES);
	do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
		do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
	TRACE_INFO("Ranging started!\r\n");
    Led6On();
}

void phscaUwb_Reset(void)
{

	uint32_t payloadLength = 0u;
	/* Hard reset using RST_N pin */
	phscaNcj29d6_HardReset(1u, 1u);

	/* Read BOOT_STATUS_NTF */
	do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
}


static void phscaUwb_Stop(void)
{

	uint32_t payloadLength = 0u;
	static uint8_t sessionRangeStopCmd[] = {0x22,0x01,0x00,0x04,0x00,0x00,0x00,0xA0}; // Session Handle instead of Session ID
	/* Range start */
	TRACE_INFO("Ranging stopping ....\r\n");
	phscaNcj29d6_HardReset(1u, 1u);
#if 0
	phscaUci_SendCommand(sessionRangeStopCmd, (uint32_t)sizeof(sessionRangeStopCmd) - (uint32_t)PHSCAUCI_u8_UCI_HEADER_SIZE_BYTES);
	/* Read BOOT_STATUS_NTF */
	do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
#endif
	TRACE_INFO("Ranging stopped!\r\n");
	Led6Off();
}


static void phscaUwb_Resume(void)
{

	uint32_t payloadLength = 0u;
	static uint8_t sessionRangeResumeCmd[] = {0x22,0x21,0x00,0x04,0x00,0x00,0x00,0xA0}; // Session Handle instead of Session ID
	/* Range start */
	TRACE_INFO("Ranging resuming ....\r\n");
	phscaUci_SendCommand(sessionRangeResumeCmd, (uint32_t)sizeof(sessionRangeResumeCmd) - (uint32_t)PHSCAUCI_u8_UCI_HEADER_SIZE_BYTES);

	/* Read BOOT_STATUS_NTF */
	do
	{
		payloadLength = phscaUci_GetResponse(response);
	} while(payloadLength == 0u);
	TRACE_INFO("Ranging resumed!\r\n");
	Led6On();

}

static void phscaUwb_MacHostPeriodicTask(TickType_t * const xLastWakeTime)
{

	uint32_t payloadLength = 0u;
	uint8_t ntf_range_ccc_data[128];
	uint32_t ranging_status = 0u;
	uint32_t ranging_distance = 0u;
	TRACE_INFO("Periodic task\r\n");
	/* Read notifications */
	do
	{
		payloadLength = phscaUci_GetResponse(ntf_range_ccc_data);
	} while(payloadLength == 0u);
	TRACE_INFO("Periodic \r\n");

	  if((ntf_range_ccc_data[0] == 0x60)&&(ntf_range_ccc_data[1] == 0x01)&&(ntf_range_ccc_data[4] == 0x01))
		        {
		  TRACE_INFO("Ranging Restarting due to SPI reset\r\n");
		  Led6Off();
		  phscaUwb_MacHostInit(false);
		  //phscaUwb_Start();
		  TRACE_INFO("Ranging Restarted due to SPI reset\r\n");
		        }

	  /*if((ntf_range_ccc_data[0] == 0x60)&&(ntf_range_ccc_data[1] == 0x07))
		        {
		  TRACE_INFO("Ranging Restarting due to HW reset\r\n");
		  phscaUwb_MacHostInit(false);
		  phscaUwb_Start();
		  TRACE_INFO("Ranging Restarted due to HW reset\r\n");
		        }*/


	       /* if((ntf_range_ccc_data[0] == 0x62)&&(ntf_range_ccc_data[3] == 0x17))
	        {
	          TRACE_INFO("Receiving RANGE_CCC_DATA_NTF\r\n");
	          TRACE_INFO(ntf_range_ccc_data);
	          ranging_status = ntf_range_ccc_data[8];
	        }
	          if (ranging_status==0)
	         {
	        		  TRACE_INFO("\r\nRanging Status = SUCCESS\r\n");
	            ranging_distance = (ntf_range_ccc_data[15] + (255*ntf_range_ccc_data[16]));
			    //TRACE_INFO("Distance = " + (ranging_distance) + " cm \r\n");
			    TRACE_INFO("Distance = %ld cm \r\n",(ranging_distance));
	         }
	          else
			    {
	        	  TRACE_INFO("Ranging Status = %ld (ERROR)\r\n" ,(ranging_status));
			    }
	        if (ntf_range_ccc_data[0] == 0x6E)
			  {
	        	TRACE_INFO("Receiving LOG_NTF\r\n");
				//TRACE_INFO(ntf_range_ccc_data);
	        	TRACE_INFO(" Data =\r\n");
	        	for ( uint8_t u8_index=0;  u8_index < 32; u8_index++ )
	            {
	        		TRACE_INFO(" %ld ", ntf_range_ccc_data[u8_index] ) ;
	            }
	        	TRACE_INFO("\r\n");

			  }*/


	vTaskDelay(pdMS_TO_TICKS(1000u));

}

static void phscaUwb_MacHostUciRspNtfCallback(const uint8_t u8_MessageType, const uint8_t u8_Gid, const uint8_t u8_Oid, const uint32_t u32_PayloadLength, const uint8_t * const u8arr_Payload)
{
	phscaUci_en_MessageType_t en_MessageType = (phscaUci_en_MessageType_t)u8_MessageType;
	osa_status_t semaphorePostStatus;

	switch(en_MessageType)
	{
		case PHSCAUCI_MESSAGETYPE_RESPONSE:
		{
			semaphorePostStatus = OSA_SemaphorePost(m_pst_InterruptPinSemaphore);
		}
			break;
		case PHSCAUCI_MESSAGETYPE_NOTIFICATION:
		{
			semaphorePostStatus = OSA_SemaphorePost(m_pst_InterruptPinSemaphore);
		}
			break;
		default:
			break;
	}

	(void)semaphorePostStatus;
}
