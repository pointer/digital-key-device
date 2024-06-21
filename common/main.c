/*! *********************************************************************************
 * \addtogroup Main
 * @{
 ********************************************************************************** */
/*! *********************************************************************************
* Copyright 2021-2022 NXP
* All rights reserved.
*
* \file
*
* This is the source file for the main entry point for a FreeRTOS application.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

/************************************************************************************
 *************************************************************************************
 * Include
 *************************************************************************************
 ************************************************************************************/
#include "app.h"
#include "app_conn.h"
#include "fsl_os_abstraction.h"
#include "keyfob_manager.h"
#include "fsl_debug_console.h"
#include "trace.h"
#include "software_version.h"
#include "uwb_manager.h"
#include "phscaEseDal.h"
#include "command.h"
#include <phscaEseDal_Gpio.h>
#include <phscaEseTypes.h>
#include <phscaEseDal_Uart.h>
#include "fsl_vbat.h"
#include "app_digital_key_device.h"

/************************************************************************************
 *************************************************************************************
 * Private functions prototypes
 *************************************************************************************
 ************************************************************************************/
static void ble_task(void *argument);
static void keyfob_task(void *argument);
static void uwb_task(void *argument);

#define WELCOME_MESSAGE "\r\n"                                            \
                        " ______   ____    ____  ____      ____ \r\n"     \
                        "|_   _ \\ |_   \\  /   _||_  _|    |_  _|\r\n"   \
                        "  | |_) |  |   \\/   |    \\ \\  /\\  / /  \r\n" \
                        "  |  __'.  | |\\  /| |     \\ \\/  \\/ /   \r\n" \
                        " _| |__) |_| |_\\/_| |_     \\  /\\  /    \r\n"  \
                        "|_______/|_____||_____|     \\/  \\/     \r\n"


#define gBleThreadStackSize_c         2048
#define gBleThreadPriority_c          2

#define gKeyfobThreadStackSize_c      gMainThreadStackSize_c
#define gKeyfobThreadPriority_c       gMainThreadPriority_c

/************************************************************************************
 *************************************************************************************
 * Private memory declarations
 *************************************************************************************
 ************************************************************************************/
static OSA_TASK_HANDLE_DEFINE(s_BleTaskHandle);
static OSA_TASK_DEFINE(ble_task, gBleThreadPriority_c, 1, gBleThreadStackSize_c, 0);

static OSA_TASK_HANDLE_DEFINE(s_KeyfobTaskHandle);
static OSA_TASK_DEFINE(keyfob_task, gKeyfobThreadPriority_c, 1, gKeyfobThreadStackSize_c, 0);

static OSA_TASK_HANDLE_DEFINE(s_uwbTaskHandle);
static OSA_TASK_DEFINE(uwb_task, gMainThreadPriority_c, 1, gMainThreadStackSize_c, 0);

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

static void ble_task(void *argument)
{
    /* Start BLE Platform related resources such as clocks, Link layer and HCI transport to Link Layer */
    (void)APP_InitBle();

    /* Start Host stack */
    BluetoothLEHost_AppInit();

    while(TRUE)
    {
        BluetoothLEHost_HandleMessages();
    }
}

static void keyfob_task(void *argument)
{
    KEYFOB_MGR_run();
}

static void uwb_task(void *argument)
{
    UWB_MGR_run();
}

static void nfc_task(void *argument)
{
/*    nfc_init();
    nfc_configure_peer_to_peer_mode();

    while (1) {
        perform_owner_pairing();
        // Add delay or event-based waiting mechanism as needed
        // Handle NFC events
    }*/
	return;

}

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/
int main(void)
{
    /* Clear all Smart power switch status flags */
    VBAT_ClearStatusFlags(VBAT0, 	        kVBAT_StatusFlagPORDetect |
    										kVBAT_StatusFlagWakeupPin |
											kVBAT_StatusFlagBandgapTimer0 |
											kVBAT_StatusFlagBandgapTimer1);

	/* Init OSA: should be called before any other OSA API */
    OSA_Init();

    BOARD_InitHardware();

    /* Start Application services (timers, serial manager, low power, led, button, etc..) */
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d > 0))
    TRACE_Init();
#endif
    APP_InitServices();
    KEYFOB_MGR_init();
    PRINTF(WELCOME_MESSAGE, strlen(WELCOME_MESSAGE));
    PRINTF("\r\n");
    PRINTF("BMW Keyfob PoC Demo (Device side) - v%s", SOFTWARE_VERSION);
    PRINTF("\r\n\r\n");
    UWB_MGR_init();

    /* Power_off the SE */
    SetSePower(mAppSePoweredOff_c);

    (void)OSA_TaskCreate((osa_task_handle_t)s_BleTaskHandle, OSA_TASK(ble_task), NULL);
    (void)OSA_TaskCreate((osa_task_handle_t)s_KeyfobTaskHandle, OSA_TASK(keyfob_task), NULL);
    (void)OSA_TaskCreate((osa_task_handle_t)s_uwbTaskHandle, OSA_TASK(uwb_task), NULL);
//    (void)OSA_TaskCreate((osa_task_handle_t)s_nfcTaskHandle, OSA_TASK(nfc_task), NULL);

#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d == 0))
    _keyfob_go_to_sleep();
#endif

    /* Start scheduler */
    OSA_Start();

    /* Won't run here */
    assert(0);
    return 0;
}

/*! *********************************************************************************
 * @}
 ********************************************************************************** */
