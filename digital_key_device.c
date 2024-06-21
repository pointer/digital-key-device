/*! *********************************************************************************
* \addtogroup Digital Key Device
* @{
********************************************************************************** */
/*! *********************************************************************************
* \file digital_key_device.c
*
* Copyright 2020-2022 NXP
*
* NXP Confidential Proprietary
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from NXP.
********************************************************************************** */

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
/* Framework / Drivers */
#include "EmbeddedTypes.h"
#include "fsl_component_button.h"
#include "fsl_component_timer_manager.h"
#include "fsl_component_panic.h"
#include "FunctionLib.h"
#include "fsl_component_mem_manager.h"
#include "fsl_adapter_reset.h"
#include "fsl_format.h"
#include "fsl_debug_console.h"
#include "app.h"
#include "board.h"
#include "fwk_platform_ble.h"
#include "NVM_Interface.h"
#include "fsl_adapter_flash.h"

#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
#include "fsl_shell.h"
#endif

/* BLE Host Stack */
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gap_interface.h"
#include "gatt_db_app_interface.h"
#include "gatt_db_handles.h"
#include "gatt_database.h"

/* Profile / Services */
#include "digital_key_interface.h"

#include "ble_conn_manager.h"
#include "ble_service_discovery.h"

#include "app_conn.h"
#include "digital_key_device.h"

#include "app_digital_key_device.h"
#include "shell_digital_key_device.h"

#include "app_nvm.h"

#include "software_version.h"

#include "trace.h"

#include "stdlib.h"

#include "keyfob_manager.h"

/************************************************************************************
*************************************************************************************
* Extern variables
*************************************************************************************
************************************************************************************/
extern gapRole_t mGapRole;
extern deviceId_t mCurrentPeerId;

/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/
/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/
uint16_t mSystemParamsValuesHandles[] = {(uint16_t)value_scanning_interval,
                                         (uint16_t)value_scan_windows,
                                         (uint16_t)value_connection_interval,
                                         (uint16_t)value_ble_power_output,
                                         (uint16_t)value_ble_scan_on_step,
                                         (uint16_t)value_ble_disc_during_still,
                                         (uint16_t)value_delta_rssi_low,
                                         (uint16_t)value_delta_rssi_medium,
                                         (uint16_t)value_delta_rssi_high,
                                         (uint16_t)value_rssi_intent_high,
                                         (uint16_t)value_timeout_between_same_intents,
                                         (uint16_t)value_rssi_on_duration,
                                         (uint16_t)value_ms_threshold_no_motion_detected,
                                         (uint16_t)value_ms_osr,
                                         (uint16_t)value_ms_odr,
                                         (uint16_t)value_ms_accuracy_range,
                                         (uint16_t)value_ms_motion_still_duration,
                                         (uint16_t)value_ms_wrist_mode,
										 (uint16_t)value_ms_step_in_scan,
										 (uint16_t)value_ms_step_out_scan,
										 (uint16_t)value_ms_total_step,
										 (uint16_t)value_ms_still_detected,
										 (uint16_t)value_temperature,
										 (uint16_t)value_battery_level,
                                         (uint16_t)value_number_of_anchors,
                                         (uint16_t)value_commands_reset,
                                         (uint16_t)value_keys_IRK,
                                         (uint16_t)value_keys_LTK,
                                         (uint16_t)value_BD_ADDR};
typedef struct
{
    uint16_t u16Handle;
    void (*pcfCharWriteCb)(int32_t, int32_t);
    int32_t arg;
}app_ble_chars_metadata;

typedef struct
{
    uint16_t u16Handle;
    void (*pcfCharWriteCb)(int32_t, uint8_t*, uint16_t);
    int32_t arg;
    int32_t max_value;
}app_keys_chars_metadata;

static void BleApp_PerformReset(int32_t arg, int32_t value);
static void BleApp_SetParam(int32_t arg, int32_t value);
static void BleApp_SetKey(int32_t arg, uint8_t *key_bytes, uint16_t Key_size);

static const app_ble_chars_metadata mCharRegistry[] =
{
    {(uint16_t)value_scanning_interval,               BleApp_SetParam, SlowScanIntervalID},
    {(uint16_t)value_scan_windows,                    BleApp_SetParam, SlowScanWindowID},
    {(uint16_t)value_connection_interval,             BleApp_SetParam, ConnectionIntervalID},
    {(uint16_t)value_ble_power_output,                BleApp_SetParam, BlePowerOutputID},
    {(uint16_t)value_ble_scan_on_step,                BleApp_SetParam, BleScanOnStepID},
    {(uint16_t)value_ble_disc_during_still,           BleApp_SetParam, BleDiscDuringStillID},
    {(uint16_t)value_delta_rssi_low,                  BleApp_SetParam, DeltaRssiLowID},
    {(uint16_t)value_delta_rssi_medium,               BleApp_SetParam, DeltaRssiMediumID},
    {(uint16_t)value_delta_rssi_high,                 BleApp_SetParam, DeltaRssiHighID},
    {(uint16_t)value_rssi_intent_high,                BleApp_SetParam, RssiIntentHighID},
    {(uint16_t)value_timeout_between_same_intents,    BleApp_SetParam, TimeoutBetweenSameIntentsID},
    {(uint16_t)value_rssi_on_duration,                BleApp_SetParam, RssiOnDurationID},
    {(uint16_t)value_ms_threshold_no_motion_detected, BleApp_SetParam, MsThresholdNoMotionDetectionID},
    {(uint16_t)value_ms_osr,                          BleApp_SetParam, MsOsrID},
    {(uint16_t)value_ms_odr,                          BleApp_SetParam, MsOdrID},
    {(uint16_t)value_ms_accuracy_range,               BleApp_SetParam, MsAccuracyRangeID},
    {(uint16_t)value_ms_motion_still_duration,        BleApp_SetParam, MsMotionStillDurationID},
	{(uint16_t)value_ms_wrist_mode,                   BleApp_SetParam, MsWristModeID},
	{(uint16_t)value_ms_step_in_scan,                 BleApp_SetParam, MsStepInScanID},
	{(uint16_t)value_ms_step_out_scan,                BleApp_SetParam, MsStepOutScanID},
	{(uint16_t)value_ms_total_step,                   BleApp_SetParam, MsTotalStepID},
	{(uint16_t)value_ms_still_detected,               BleApp_SetParam, MsStillDetectedID},
	{(uint16_t)value_temperature,                     BleApp_SetParam, TemperatureID},
	{(uint16_t)value_battery_level,                   BleApp_SetParam, BatteryLevelID},
	{(uint16_t)value_number_of_anchors,               BleApp_SetParam, NumberOfAnchorsID},
    {(uint16_t)value_commands_reset,                  BleApp_PerformReset, 0},
};

static const app_keys_chars_metadata mKeysCharRegistry[] =
{
	{(uint16_t)value_keys_IRK,        BleApp_SetKey,      IrkKeyID,       KEY_MAX_SIZE},
	{(uint16_t)value_keys_LTK,        BleApp_SetKey,      LtkKeyID,       KEY_MAX_SIZE},
	{(uint16_t)value_BD_ADDR,         BleApp_SetKey,      BdAddrKeyID,    BD_ADDR_MAX_SIZE},
};

static appScanningParams_t appScanParams = {
    &gScanParams,
    gGapDuplicateFilteringEnable_c,
    gGapScanContinuously_d,
    gGapScanPeriodicDisabled_d
};

static appAdvertisingParams_t mAppAdvParams = {
    &gAdvParams,
    &gAppAdvertisingData,
    &gAppScanRspData
};

    advState_t mAdvState;
    bool_t   mScanningOn = FALSE;
    bool_t   mFoundDeviceToConnect = FALSE;

/* Timers */
#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
static TIMER_MANAGER_HANDLE_DEFINE(mAppTimerId);
#endif

/* Application callback */
pfBleCallback_t mpfBleEventHandler = NULL;

/* This global will be TRUE if the user adds or removes a bond */
bool_t gPrivacyStateChangedByUser = FALSE;
/************************************************************************************
*************************************************************************************
* Private functions prototypes
*************************************************************************************
************************************************************************************/

/* Host Stack callbacks */
static void BleApp_AdvertisingCallback(gapAdvertisingEvent_t *pAdvertisingEvent);

static void BleApp_ScanningCallback
(
    gapScanningEvent_t* pScanningEvent
);

static void BleApp_ConnectionCallback
(
    deviceId_t peerDeviceId,
    gapConnectionEvent_t* pConnectionEvent
);

static void BleApp_ConnectionCallback_SignalSimpleEvents(deviceId_t peerDeviceId, appEvent_t connectionCallbackEvent);

static void BleApp_GattClientCallback
(
    deviceId_t              serverDeviceId,
    gattProcedureType_t     procedureType,
    gattProcedureResult_t   procedureResult,
    bleResult_t             error
);

static void BleApp_GattServerCallback
(
    deviceId_t deviceId,
    gattServerEvent_t *pServerEvent
);

static void BleApp_ServiceDiscoveryCallback
(
    deviceId_t peerDeviceId,
    servDiscEvent_t* pEvent
);

/* PSM callbacks */
static void BleApp_L2capPsmDataCallback (deviceId_t deviceId, uint16_t lePsm, uint8_t* pPacket, uint16_t packetLength);
static void BleApp_L2capPsmControlCallback (l2capControlMessage_t* pMessage);

static bool_t CheckScanEventLegacy(gapScannedDevice_t* pData);
static bool_t CheckScanEventExtended(gapExtScannedDevice_t* pData);

static void BleApp_StoreServiceHandles
(
    deviceId_t      peerDeviceId,
    gattService_t   *pService
);

static void BleApp_GenericCallback_HandlePrivacyEvents(gapGenericEvent_t* pGenericEvent);
#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
static void ScanningTimeoutTimerCallback(void* pParam);
#endif
static void BluetoothLEHost_Initialized(void);

#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 0))
button_status_t BleApp_HandleKeys0(void *buttonHandle, button_callback_message_t *message,void *callbackParam);
#endif /*gAppButtonCnt_c > 0*/
#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 1))
button_status_t BleApp_HandleKeys1(void *buttonHandle, button_callback_message_t *message,void *callbackParam);
#endif /*gAppButtonCnt_c > 1*/
#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 2))
button_status_t BleApp_HandleKeys2(void *buttonHandle, button_callback_message_t *message,void *callbackParam);
#endif /*gAppButtonCnt_c > 2*/
/************************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
************************************************************************************/
/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief  This is the initialization function for each application. This function 
*         should contain all the initialization code required by the bluetooth demo
********************************************************************************** */
void BluetoothLEHost_AppInit(void)
{
    union Prompt_tag
    {
        const char * constPrompt;
        char * prompt;
    } shellPrompt;
    
    uint8_t mPeerId = 0;

    for (mPeerId = 0; mPeerId < (uint8_t)gAppMaxConnections_c; mPeerId++)
    {
        maPeerInformation[mPeerId].deviceId = gInvalidDeviceId_c;
        FLib_MemSet(&maPeerInformation[mPeerId].oobData, 0x00, sizeof(gapLeScOobData_t));
        FLib_MemSet(&maPeerInformation[mPeerId].peerOobData, 0x00, sizeof(gapLeScOobData_t));
    }

    //LedStartFlashingAllLeds();

#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 0))
    (void)BUTTON_InstallCallback((button_handle_t)g_buttonHandle[0], BleApp_HandleKeys0, NULL);
#endif /* (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 0)) */
#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 1))
    (void)BUTTON_InstallCallback((button_handle_t)g_buttonHandle[1], BleApp_HandleKeys1, NULL);
#endif /* (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 1)) */
#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 2))
    (void)BUTTON_InstallCallback((button_handle_t)g_buttonHandle[2], BleApp_HandleKeys2, NULL);
#endif /* (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 2)) */

    BleApp_RegisterEventHandler(APP_BleEventHandler);

    /* Set generic callback */
    BluetoothLEHost_SetGenericCallback(BleApp_GenericCallback);

    /* Initialize Bluetooth Host Stack */
    BluetoothLEHost_Init(BluetoothLEHost_Initialized);

    (void)Gap_ControllerEnhancedNotification(((uint32_t)gNotifConnCreated_c | (uint32_t)gNotifPhyUpdateInd_c), 0U);
    
    /* UI */
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d > 0))
    shellPrompt.constPrompt = "Device>";
    AppShellInit(shellPrompt.prompt);
#endif
}

/*! *********************************************************************************
 * \brief        Start BLE connection
 ********************************************************************************** */
void BleApp_Connect(void)
{
	TRACE_INFO("Connecting...");
	/* Temporarily use minimum interval to speed up connection */
    systemParameters_t *pSysParams = NULL;
	App_NvmReadSystemParams(&pSysParams);
	gConnReqParams.connIntervalMin = gConnectionParamConversionMsToGapUnit((pSysParams->system_params).fields.connection_interval);
	gConnReqParams.connIntervalMin = gConnectionParamConversionMsToGapUnit((pSysParams->system_params).fields.connection_interval);
	(void)BluetoothLEHost_Connect(&gConnReqParams, BleApp_ConnectionCallback);
}

/*! *********************************************************************************
 * \brief        Register function to handle events from BLE to APP
 *
 * \param[in]    pCallback       event handler
 ********************************************************************************** */
void BleApp_RegisterEventHandler(pfBleCallback_t pfBleEventHandler)
{
    mpfBleEventHandler = pfBleEventHandler;
}

/*! *********************************************************************************
* \brief    Starts the BLE application.
*
********************************************************************************** */
void BleApp_Start(gapRole_t gapRole, uint32_t scannnigInterval, uint32_t scanWindows)
{
    switch (gapRole)
    {
        case gGapCentral_c:
        {
        	BleApp_SetBondingDataOfBMWVehicle();
            if (!mScanningOn)
            {
                /* Start scanning */
                appScanParams.pHostScanParams->interval = scannnigInterval;
                appScanParams.pHostScanParams->window = scanWindows;
                (void)BluetoothLEHost_StartScanning(&appScanParams, BleApp_ScanningCallback);
            }
            break;
        }

        case gGapPeripheral_c:
        {
            /* Start ADV only if it's not already started */
            if (!mAdvState.advOn)
            {
                (void)BluetoothLEHost_StartAdvertising(&mAppAdvParams, BleApp_AdvertisingCallback, BleApp_ConnectionCallback);
            }
            break;
        }

        default:
        {
            ; /* No action required */
            break;
        }
    }
}

/*! *********************************************************************************
* \brief        Handles keyboard events.
*
* \param[in]    events    Key event structure.
********************************************************************************** */
#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 0))
button_status_t  BleApp_HandleKeys0(void *buttonHandle, button_callback_message_t *message,void *callbackParam)
{
    switch (message->event)
    {
        case kBUTTON_EventOneClick:
        case kBUTTON_EventShortPress:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_KBD_EventPressPB1_c;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;

        /* Disconnect on long button press */
        case kBUTTON_EventLongPress:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_KBD_EventLongPB1_c;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;
        
        /* Very Long Press not available - use Double Click */
        case kBUTTON_EventDoubleClick:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_KBD_EventVeryLongPB1_c;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;

        default:
        {
            ; /* No action required */
            break;
        }
    }

    return kStatus_BUTTON_Success; 
}
#endif /*gAppButtonCnt_c > 0*/

#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 1))
/*! *********************************************************************************
* \brief        Handles keyboard events.
*
* \param[in]    buttonHandle    button handle
* \param[in]    message         Button press event
* \param[in]    callbackParam   parameter
********************************************************************************** */
button_status_t BleApp_HandleKeys1(void *buttonHandle, button_callback_message_t *message,void *callbackParam)
{
    switch (message->event)
    {
        case kBUTTON_EventOneClick:
        case kBUTTON_EventShortPress:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_KBD_EventPressPB2_c;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;

        case kBUTTON_EventLongPress:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_KBD_EventLongPB2_c;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;

        default:
        {
            ; /* No action required */
            break;
        }
    }

    return kStatus_BUTTON_Success;
}
#endif /* (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 1)) */

#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 2))
/*! *********************************************************************************
* \brief        Handles keyboard events.
*
* \param[in]    buttonHandle    button handle
* \param[in]    message         Button press event
* \param[in]    callbackParam   parameter
********************************************************************************** */
button_status_t BleApp_HandleKeys2(void *buttonHandle, button_callback_message_t *message,void *callbackParam)
{
    switch (message->event)
    {
        case kBUTTON_EventOneClick:
        case kBUTTON_EventShortPress:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_KBD_EventPressPB3_c;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;

        default:
        {
            ; /* No action required */
            break;
        }
    }

    return kStatus_BUTTON_Success;
}
#endif /* (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 2)) */

/*! *********************************************************************************
* \brief        Handles BLE generic callback.
*
* \param[in]    pGenericEvent    Pointer to gapGenericEvent_t.
********************************************************************************** */
void BleApp_GenericCallback (gapGenericEvent_t* pGenericEvent)
{
    /* Call BLE Conn Manager */
    BleConnManager_GenericEvent(pGenericEvent);

    switch (pGenericEvent->eventType)
    {
        case gLePhyEvent_c:
            {
                if(mpfBleEventHandler != NULL)
                {
                    appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(gapPhyEvent_t));
                    if(pEventData != NULL)
                    {
                        pEventData->appEvent = mAppEvt_GenericCallback_LePhyEvent_c;
                        pEventData->eventData.pData = pEventData + 1;
                        FLib_MemCpy(pEventData->eventData.pData, &pGenericEvent->eventData.phyEvent, sizeof(gapPhyEvent_t));
                        if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                        {
                            (void)MEM_BufferFree(pEventData);
                        }
                    }
                }
            }
            break;
        case gLeScLocalOobData_c:
            {
                if(mpfBleEventHandler != NULL)
                {
                    appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(gapLeScOobData_t));
                    if(pEventData != NULL)
                    {
                        pEventData->appEvent = mAppEvt_GenericCallback_LeScLocalOobData_c;
                        pEventData->eventData.pData = pEventData + 1;
                        FLib_MemCpy(pEventData->eventData.pData, &pGenericEvent->eventData.localOobData, sizeof(gapLeScOobData_t));
                        if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                        {
                            (void)MEM_BufferFree(pEventData);
                        }
                    }
                }
            }
            break;
        
        case gRandomAddressReady_c:
            {
                if(mpfBleEventHandler != NULL)
                {
                    appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(gcBleDeviceAddressSize_c));
                    if(pEventData != NULL)
                    {
                        pEventData->appEvent = mAppEvt_GenericCallback_RandomAddressReady_c;
                        pEventData->eventData.pData = pEventData + 1;
                        FLib_MemCpy(pEventData->eventData.pData, pGenericEvent->eventData.addrReady.aAddress, sizeof(gcBleDeviceAddressSize_c));
                        if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                        {
                            (void)MEM_BufferFree(pEventData);
                        }
                    }
                }
            }
            break;
         
        case gControllerNotificationEvent_c:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(bleNotificationEvent_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_GenericCallback_CtrlNotifEvent_c;
                    pEventData->eventData.pData = pEventData +1U;
                    FLib_MemCpy(pEventData->eventData.pData, &pGenericEvent->eventData.notifEvent, sizeof(bleNotificationEvent_t));  
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;
        
        case gBondCreatedEvent_c:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(bleBondCreatedEvent_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_GenericCallback_BondCreatedEvent_c;
                    pEventData->eventData.pData = pEventData + 1;
                    FLib_MemCpy(pEventData->eventData.pData, &pGenericEvent->eventData.bondCreatedEvent, sizeof(bleBondCreatedEvent_t));  
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;
        case gHostPrivacyStateChanged_c:
        case gControllerPrivacyStateChanged_c:
        {
            BleApp_GenericCallback_HandlePrivacyEvents(pGenericEvent);
        }
        break;
            
        default:
            {
                ; /* No action required */
            }
            break;
    }
}

/*! *********************************************************************************
* \brief        Handles Shell_Factory Reset Command event.
********************************************************************************** */
void BleApp_FactoryReset(void)
{
    /* Erase NVM Datasets */
    NVM_Status_t status = NvFormat();
    if (status != gNVM_OK_c)
    {
         /* NvFormat exited with an error status */ 
         panic(0, (uint32_t)BleApp_FactoryReset, 0, 0);
    }
    
    /* Reset MCU */
    HAL_ResetMCU();
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief        Handler of gHostPrivacyStateChanged_c and 
*               gControllerPrivacyStateChanged_c from BleApp_GenericCallback.
*
* \param[in]    pGenericEvent    Pointer to gapGenericEvent_t.
********************************************************************************** */
static void BleApp_GenericCallback_HandlePrivacyEvents(gapGenericEvent_t* pGenericEvent)
{
    switch (pGenericEvent->eventType)
    {   
        case gHostPrivacyStateChanged_c:
        {
            if (!pGenericEvent->eventData.newHostPrivacyState)
            {
                if(gPrivacyStateChangedByUser)
                {
                    gPrivacyStateChangedByUser = FALSE;
                    /* Host privacy disabled because a bond was removed
                       or added. Enable privacy. */
                    (void)BleConnManager_EnablePrivacy();
                }
            }
        }
        break;
        
        case gControllerPrivacyStateChanged_c:
        {
            if (!pGenericEvent->eventData.newControllerPrivacyState)
            {
                if(gPrivacyStateChangedByUser)
                {
                    gPrivacyStateChangedByUser = FALSE;
                    /* Controller privacy disabled because a bond was removed
                       or added. Enable privacy. */
                    (void)BleConnManager_EnablePrivacy();
                }
            }
        }
        break;
        
        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles BLE Advertising callback from host stack.
*
* \param[in]    pAdvertisingEvent    Pointer to gapAdvertisingEvent_t.
********************************************************************************** */
static void BleApp_AdvertisingCallback (gapAdvertisingEvent_t *pAdvertisingEvent)
{
    switch (pAdvertisingEvent->eventType)
    {
        case gAdvertisingStateChanged_c:
        {
            mAdvState.advOn = !mAdvState.advOn;
            if (mAdvState.advOn)
            {
                TRACE_INFO("Advertising...");
                Led2On();
            }
            else
            {
                TRACE_INFO("Advertising stopped.");
                if (maPeerInformation[mCurrentPeerId].appState == mAppPreIdle_c)
                {
                    BleApp_StateMachineHandler(mCurrentPeerId, mAppEvt_SwitchRole);
                }
                else
                {
                    /* No action required */
                }
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d > 0))
                shell_cmd_finished();
#endif
            }
        }
        break;

        case gAdvertisingCommandFailed_c:
        {
            panic(0, 0, 0, 0);
        }
        break;

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles BLE Scanning callback from host stack.
*
* \param[in]    pScanningEvent    Pointer to gapScanningEvent_t.
********************************************************************************** */
static void BleApp_ScanningCallback (gapScanningEvent_t* pScanningEvent)
{
    switch (pScanningEvent->eventType)
    {
        case gDeviceScanned_c:
        {
            /* Check if the scanned device implements the DK Service */
            if( FALSE == mFoundDeviceToConnect )
            {
                /* Advertising for Passive Entry has an empty payload - if we have a bond with the device, connect */
                /* Only check the payload if we do not have a bond */
                if (pScanningEvent->eventData.scannedDevice.advertisingAddressResolved == FALSE)
                {
                    mFoundDeviceToConnect = CheckScanEventLegacy(&pScanningEvent->eventData.scannedDevice);
                }

                if (mFoundDeviceToConnect || (pScanningEvent->eventData.scannedDevice.advertisingAddressResolved == TRUE))
                {
                	mFoundDeviceToConnect = TRUE;
                    /* Set connection parameters and stop scanning. Connect on gScanStateChanged_c. */
                    gConnReqParams.peerAddressType = pScanningEvent->eventData.scannedDevice.addressType;
                    FLib_MemCpy(gConnReqParams.peerAddress,
                                pScanningEvent->eventData.scannedDevice.aAddress,
                                sizeof(bleDeviceAddress_t));

                    (void)Gap_StopScanning();
#if defined(gAppUsePrivacy_d) && (gAppUsePrivacy_d)
                    gConnReqParams.usePeerIdentityAddress = pScanningEvent->eventData.scannedDevice.advertisingAddressResolved;
#endif
                }
            }
        }
        break;

        case gExtDeviceScanned_c:
        {
            /* Check if the scanned device implements the DK Service */
            if( FALSE == mFoundDeviceToConnect )
            {
                /* Advertising for Passive Entry has an empty payload - if we have a bond with the device, connect */
                /* Only check the payload if we do not have a bond */
                if (pScanningEvent->eventData.extScannedDevice.advertisingAddressResolved == FALSE)
                {
                    mFoundDeviceToConnect = CheckScanEventExtended(&pScanningEvent->eventData.extScannedDevice);
                }

                if (mFoundDeviceToConnect || (pScanningEvent->eventData.extScannedDevice.advertisingAddressResolved == TRUE))
                {
                    mFoundDeviceToConnect = TRUE;
                    /* Set connection parameters and stop scanning. Connect on gScanStateChanged_c. */
                    gConnReqParams.peerAddressType = pScanningEvent->eventData.extScannedDevice.addressType;
                    FLib_MemCpy(gConnReqParams.peerAddress,
                                pScanningEvent->eventData.extScannedDevice.aAddress,
                                sizeof(bleDeviceAddress_t));

                    (void)Gap_StopScanning();
#if defined(gAppUsePrivacy_d) && (gAppUsePrivacy_d)
                    gConnReqParams.usePeerIdentityAddress = pScanningEvent->eventData.extScannedDevice.advertisingAddressResolved;
#endif
                }
            }
        }
        break;

        case gScanStateChanged_c:
        {
            mScanningOn = !mScanningOn;

            /* Node starts scanning */
            if (mScanningOn)
            {
                mFoundDeviceToConnect = FALSE;

                TRACE_INFO("Scanning...");

#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
                /* Start scanning timer */
                (void)TM_InstallCallback((timer_handle_t)mAppTimerId, ScanningTimeoutTimerCallback, NULL);
                (void)TM_Start((timer_handle_t)mAppTimerId, (uint8_t)kTimerModeSetSecondTimer | (uint8_t)kTimerModeLowPowerTimer, gScanningTime_c);
                Led1On();
#else
                Led2On();
#endif /* #if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode) */
            }
            /* Node is not scanning */
            else
            {
#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
                timer_status_t status = TM_Stop((timer_handle_t)mAppTimerId);
                if (status != kStatus_TimerSuccess)
                {
                    panic(0, (uint32_t)BleApp_ScanningCallback, 0, 0);
                }
#endif

                TRACE_INFO("Scan stopped.");
                
                Led2Off();

                /* Connect with the previously scanned peer device */
                if (mFoundDeviceToConnect)
                {
                	KEYFOB_MGR_notify(KEYFOB_EVENT_CONNECTION_REQUEST);
                }
                else
                {
#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
                    Led1Off();
#endif
                    if (maPeerInformation[mCurrentPeerId].appState == mAppPreIdle_c)
                    {
                        BleApp_StateMachineHandler(mCurrentPeerId, mAppEvt_SwitchRole);
                    }
                    else
                    {
                        /* No action required */
                    }
                }
            }
        }
        break;

        case gScanCommandFailed_c:
        {
            ; /* No action required */
        }
        break;

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles BLE Connection callback from host stack.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    pConnectionEvent    Pointer to gapConnectionEvent_t.
********************************************************************************** */
static void BleApp_ConnectionCallback (deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent)
{
    /* Connection Manager to handle Host Stack interactions */
    BleConnManager_GapCentralEvent(peerDeviceId, pConnectionEvent);

    switch (pConnectionEvent->eventType)
    {
        case gConnEvtConnected_c:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(appConnectionCallbackEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_ConnectionCallback_ConnEvtConnected_c;
                    pEventData->eventData.pData = pEventData + 1;
                    appConnectionCallbackEventData_t *pConnectionCallbackEventData = pEventData->eventData.pData;
                    pConnectionCallbackEventData->peerDeviceId = peerDeviceId;
                    FLib_MemCpy(&pConnectionCallbackEventData->pConnectedEvent, &pConnectionEvent->eventData.connectedEvent, sizeof(gapConnectedEvent_t));
                    pConnectionCallbackEventData = NULL;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;

        case gConnEvtDisconnected_c:
        {
            BleApp_ConnectionCallback_SignalSimpleEvents(peerDeviceId, mAppEvt_ConnectionCallback_ConnEvtDisconnected_c);
        }
        break;

#if gAppUsePairing_d
        case gConnEvtLeScOobDataRequest_c:
        {
            BleApp_ConnectionCallback_SignalSimpleEvents(peerDeviceId, mAppEvt_ConnectionCallback_ConnEvtLeScOobDataRequest_c);
        }
        break;
        
        case gConnEvtPairingComplete_c:
        {
            /* Notify state machine handler on pairing complete */
            if (pConnectionEvent->eventData.pairingCompleteEvent.pairingSuccessful)
            {
                BleApp_ConnectionCallback_SignalSimpleEvents(peerDeviceId, mAppEvt_ConnectionCallback_ConnEvtPairingComplete_c);
            }
        }
        break;

#if defined(gAppUseBonding_d) && (gAppUseBonding_d)
        case gConnEvtEncryptionChanged_c:
        {
            if( pConnectionEvent->eventData.encryptionChangedEvent.newEncryptionState )
            {
                BleApp_ConnectionCallback_SignalSimpleEvents(peerDeviceId, mAppEvt_ConnectionCallback_ConnEvtEncryptionChanged_c);
            }
        }
        break;

        case gConnEvtAuthenticationRejected_c:
        {
            BleApp_ConnectionCallback_SignalSimpleEvents(peerDeviceId, mAppEvt_ConnectionCallback_ConnEvtAuthenticationRejected_c);
        }
        break;
#endif /* gAppUseBonding_d */
#endif /* gAppUsePairing_d */

        case gConnEvtRssiRead_c:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(appRssiReadCallbackEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_ConnectionCallback_ReadRssiEvtConnected_c;
                    pEventData->eventData.pData = pEventData + 1;
                    appRssiReadCallbackEventData_t *pReadRssiCallbackEventData = pEventData->eventData.pData;
                    pReadRssiCallbackEventData->peerDeviceId = peerDeviceId;
                    pReadRssiCallbackEventData->rssi_dBm = pConnectionEvent->eventData.rssi_dBm;
                    pReadRssiCallbackEventData = NULL;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
        break;

        case gConnEvtPowerReadFailure_c:
            ; /* No action required */
            break;

        default:
            ; /* No action required */
            break;
    }
}


/*! *********************************************************************************
* \brief        Handles signaling of simple events (peerDeviceId and appEvent_t
*               connection event) from BLE Connection callback from host stack to the
*               application.
*
* \param[in]    peerDeviceId                     Peer device ID.
* \param[in]    connectionCallbackEvent          Event type.
********************************************************************************** */
static void BleApp_ConnectionCallback_SignalSimpleEvents
(
    deviceId_t peerDeviceId,
    appEvent_t connectionCallbackEvent
)
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = connectionCallbackEvent;
            pEventData->eventData.peerDeviceId = peerDeviceId;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
}

/*! *********************************************************************************
* \brief        Handles discovered services.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    pEvent              Pointer to servDiscEvent_t.
********************************************************************************** */
static void BleApp_ServiceDiscoveryCallback(deviceId_t peerDeviceId, servDiscEvent_t* pEvent)
{
    switch(pEvent->eventType)
    {
        /* Store the discovered handles for later use. */
        case gServiceDiscovered_c:
        {
            BleApp_StoreServiceHandles(peerDeviceId, pEvent->eventData.pService);
        }
        break;

        /* Service discovery has finished, run the state machine. */
        case gDiscoveryFinished_c:
        {
            if (pEvent->eventData.success)
            {
                if(mpfBleEventHandler != NULL)
                {
                    appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                    if(pEventData != NULL)
                    {
                        pEventData->appEvent = mAppEvt_ServiceDiscoveryCallback_DiscoveryFinishedWithSuccess_c;
                        pEventData->eventData.peerDeviceId = peerDeviceId;
                        if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                        {
                            (void)MEM_BufferFree(pEventData);
                        }
                    }
                }
            }
            else
            {
                if(mpfBleEventHandler != NULL)
                {
                    appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                    if(pEventData != NULL)
                    {
                        pEventData->appEvent = mAppEvt_ServiceDiscoveryCallback_DiscoveryFinishedFailed_c;
                        pEventData->eventData.peerDeviceId = peerDeviceId;
                        if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                        {
                            (void)MEM_BufferFree(pEventData);
                        }
                    }
                }
            }
        }
        break;

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Stores handles for the specified service.
*
* \param[in]    peerDeviceId   Peer device ID.
* \param[in]    pService       Pointer to gattService_t.
********************************************************************************** */
static void BleApp_StoreServiceHandles
(
    deviceId_t      peerDeviceId,
    gattService_t   *pService
)
{
    uint8_t i;

    if ((pService->uuidType == gBleUuidType16_c) &&
        (pService->uuid.uuid16 == gBleSig_CCC_DK_UUID_d))
    {
        /* Found DK Service */
        maPeerInformation[peerDeviceId].customInfo.hDkService = pService->startHandle;
        for (i = 0; i < pService->cNumCharacteristics; i++)
        {
            if (pService->aCharacteristics[i].value.uuidType == gBleUuidType128_c)
            {
                if (FLib_MemCmp(pService->aCharacteristics[i].value.uuid.uuid128, uuid_char_vehicle_psm, 16))
                {
                    /* Found Vehicle PSM Char */
                    maPeerInformation[peerDeviceId].customInfo.hPsmChannelChar = pService->aCharacteristics[i].value.handle;

                    maCharacteristics[mcCharVehiclePsmIndex_c].value.handle = maPeerInformation[peerDeviceId].customInfo.hPsmChannelChar;
                    maCharacteristics[mcCharVehiclePsmIndex_c].value.maxValueLength = mcCharVehiclePsmLength_c;
                    maCharacteristics[mcCharVehiclePsmIndex_c].value.paValue = mValVehiclePsm;
                }
                else if (FLib_MemCmp(pService->aCharacteristics[i].value.uuid.uuid128, uuid_char_antenna_id, 16))
                {
                    /* Found Vehicle Antenna Identifier Char */
                    maPeerInformation[peerDeviceId].customInfo.hAntennaIdChar = pService->aCharacteristics[i].value.handle;

                    maCharacteristics[mcCharVehicleAntennaIdIndex_c].value.handle = maPeerInformation[peerDeviceId].customInfo.hAntennaIdChar;
                    maCharacteristics[mcCharVehicleAntennaIdIndex_c].value.maxValueLength = mcCharVehicleAntennaIdLength_c;
                    maCharacteristics[mcCharVehicleAntennaIdIndex_c].value.paValue = (uint8_t *)&mValVehicleAntennaId;
                }
                else
                {
                    /* MISRA */
                }
            }
            else if (pService->aCharacteristics[i].value.uuidType == gBleUuidType16_c)
            {
                if (pService->aCharacteristics[i].value.uuid.uuid16 == (uint16_t)gBleSig_TxPower_d)
                {
                    /* Found Tx Power Char */
                    maPeerInformation[peerDeviceId].customInfo.hTxPowerChar = pService->aCharacteristics[i].value.handle;

                    maCharacteristics[mcCharTxPowerLevelIndex_c].value.handle = maPeerInformation[peerDeviceId].customInfo.hTxPowerChar;
                    maCharacteristics[mcCharTxPowerLevelIndex_c].value.maxValueLength = mcCharTxPowerLevelLength_c;
                    maCharacteristics[mcCharTxPowerLevelIndex_c].value.paValue = (uint8_t *)&mValTxPower;
                }
            }
            else
            {
                /* MISRA */
            }
        }

        mCurrentCharReadingIndex = mcCharVehiclePsmIndex_c;
        (void)GattClient_ReadCharacteristicValue(peerDeviceId, &maCharacteristics[mcCharVehiclePsmIndex_c], mcCharVehiclePsmLength_c);
    }
}

/*! *********************************************************************************
* \brief        Handles GATT client callback from host stack.
*
* \param[in]    serverDeviceId      GATT Server device ID.
* \param[in]    procedureType       Procedure type.
* \param[in]    procedureResult     Procedure result.
* \param[in]    error               Callback result.
********************************************************************************** */
static void BleApp_GattClientCallback(
    deviceId_t              serverDeviceId,
    gattProcedureType_t     procedureType,
    gattProcedureResult_t   procedureResult,
    bleResult_t             error
)
{
    if (procedureResult == gGattProcError_c)
    {
        attErrorCode_t attError = (attErrorCode_t)(uint8_t)(error);

        if (attError == gAttErrCodeInsufficientEncryption_c     ||
            attError == gAttErrCodeInsufficientAuthorization_c  ||
            attError == gAttErrCodeInsufficientAuthentication_c)
        {
            /* Start Pairing Procedure */
            (void)Gap_Pair(serverDeviceId, &gPairingParameters);
        }

        if(mpfBleEventHandler != NULL)
        {
            appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
            if(pEventData != NULL)
            {
                pEventData->appEvent = mAppEvt_GattClientCallback_GattProcError_c;
                pEventData->eventData.peerDeviceId = serverDeviceId;
                if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                {
                    (void)MEM_BufferFree(pEventData);
                }
            }
        }
    }
    else
    {
        if (procedureResult == gGattProcSuccess_c)
        {
            switch(procedureType)
            {
                case gGattProcReadCharacteristicValue_c:
                {
                    if(mpfBleEventHandler != NULL)
                    {
                        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                        if(pEventData != NULL)
                        {
                            pEventData->appEvent = mAppEvt_GattClientCallback_GattProcReadCharacteristicValue_c;
                            pEventData->eventData.peerDeviceId = serverDeviceId;
                            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                            {
                                (void)MEM_BufferFree(pEventData);
                            }
                        }
                    }
                }
                break;

                case gGattProcReadUsingCharacteristicUuid_c:
                {
                    if(mpfBleEventHandler != NULL)
                    {
                        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                        if(pEventData != NULL)
                        {
                            pEventData->appEvent = mAppEvt_GattClientCallback_GattProcReadUsingCharacteristicUuid_c;
                            pEventData->eventData.peerDeviceId = serverDeviceId;
                            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                            {
                                (void)MEM_BufferFree(pEventData);
                            }
                        }
                    }
                }
                break;

                default:
                {
                    if(mpfBleEventHandler != NULL)
                    {
                        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                        if(pEventData != NULL)
                        {
                            pEventData->appEvent = mAppEvt_GattClientCallback_GattProcComplete_c;
                            pEventData->eventData.peerDeviceId = serverDeviceId;
                            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                            {
                                (void)MEM_BufferFree(pEventData);
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    /* Signal Service Discovery Module */
    BleServDisc_SignalGattClientEvent(serverDeviceId, procedureType, procedureResult, error);
}

/*! *********************************************************************************
 * \brief        Handles GATT server callback from host stack.
 *
 * \param[in]    deviceId           Client peer device ID.
 * \param[in]    pServerEvent       Pointer to gattServerEvent_t.
 ********************************************************************************** */
static void BleApp_GattServerCallback
(
    deviceId_t deviceId,
    gattServerEvent_t *pServerEvent
)
{
    uint16_t tempMtu = 0;
    uint8_t i;
    int32_t newCaracteristicValue;
    //uint8_t newArrayBytes[KEY_MAX_SIZE];
    bleResult_t result;

    switch (pServerEvent->eventType)
    {
        case gEvtAttributeWritten_c:
        {
            for(i=0; i<(sizeof(mCharRegistry)/sizeof(app_ble_chars_metadata)); i++)
            {
                if(mCharRegistry[i].u16Handle == pServerEvent->eventData.attributeWrittenEvent.handle)
                {
                    /*if(mCharRegistry[i].u16Handle != (uint16_t)value_commands_reset)
                    {*/
                    	switch(pServerEvent->eventData.attributeWrittenEvent.cValueLength)
                        {
                        case sizeof(int8_t):
                            newCaracteristicValue = (int32_t) *((int8_t*)pServerEvent->eventData.attributeWrittenEvent.aValue);
                            break;
                        case sizeof(int16_t):
                            newCaracteristicValue = (int32_t) *((int16_t*)pServerEvent->eventData.attributeWrittenEvent.aValue);
                            break;
                        case sizeof(int32_t):
                            newCaracteristicValue = (int32_t) *((int32_t*)pServerEvent->eventData.attributeWrittenEvent.aValue);
                            break;
                        default:
                            break;
                        }
                        mCharRegistry[i].pcfCharWriteCb(mCharRegistry[i].arg, newCaracteristicValue);
                        newCaracteristicValue = ((newCaracteristicValue>>24)&0xff) | // move byte 3 to byte 0
                                                ((newCaracteristicValue<<8)&0xff0000) | // move byte 1 to byte 2
                                                ((newCaracteristicValue>>8)&0xff00) | // move byte 2 to byte 1
                                                ((newCaracteristicValue<<24)&0xff000000); // byte 0 to byte 3
                        result = GattDb_WriteAttribute(pServerEvent->eventData.attributeWrittenEvent.handle,
                                                       pServerEvent->eventData.attributeWrittenEvent.cValueLength,
                                                       (uint8_t *)&newCaracteristicValue);
                    /*}
                    else
                    {
                    	mCharRegistry[i].pcfCharWriteCb(mCharRegistry[i].arg, (int32_t)&pServerEvent->eventData.attributeWrittenEvent.aValue);
                    }*/
                    GattServer_SendAttributeWrittenStatus(deviceId,
                                                          pServerEvent->eventData.attributeWrittenEvent.handle,
                                                          result);
                    break;
                }
            }

            for(i=0; i<(sizeof(mKeysCharRegistry)/sizeof(app_keys_chars_metadata)); i++)
            {
                if(mKeysCharRegistry[i].u16Handle == pServerEvent->eventData.attributeWrittenEvent.handle)
                {
                    uint8_t newArrayBytes[mKeysCharRegistry[i].max_value];
                	FLib_MemCpy(newArrayBytes, pServerEvent->eventData.attributeWrittenEvent.aValue,
                			    pServerEvent->eventData.attributeWrittenEvent.cValueLength);
                	mKeysCharRegistry[i].pcfCharWriteCb(mKeysCharRegistry[i].arg,
                			                            newArrayBytes,
														pServerEvent->eventData.attributeWrittenEvent.cValueLength);
                    result = GattDb_WriteAttribute(pServerEvent->eventData.attributeWrittenEvent.handle,
                                                   pServerEvent->eventData.attributeWrittenEvent.cValueLength,
												   newArrayBytes);
                    GattServer_SendAttributeWrittenStatus(deviceId,
                                                          pServerEvent->eventData.attributeWrittenEvent.handle,
                                                          result);
                    break;
                }
            }
        }
        break;

        case gEvtMtuChanged_c:
        {
            /* update stream length with minimum of  new MTU */
            (void)Gatt_GetMtu(deviceId, &tempMtu);
            tempMtu = gAttMaxWriteDataSize_d(tempMtu);
        }
        break;

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

static void BleApp_PerformReset(int32_t arg, int32_t value)
{
    (void)arg;
    (void)value;

    HAL_ResetMCU();
}

static void BleApp_SetParam(int32_t arg, int32_t value)
{
    App_NvmWriteSystemParam(arg, value);
}

static void BleApp_SetKey(int32_t arg, uint8_t *key_bytes, uint16_t key_size)
{
    App_NvmWriteBleKey(arg, key_bytes, key_size);
}

/*! *********************************************************************************
* \brief        Process scanning events to search for the DK Ranging Service.
*               This function is called from the scanning callback.
*
* \param[in]    pData                   Pointer to gapScannedDevice_t.
*
* \return       TRUE if the scanned device implements the DK Ranging Service,
                FALSE otherwise
********************************************************************************** */
static bool_t CheckScanEventLegacy(gapScannedDevice_t* pData)
{
    uint32_t index = 0;
    bool_t foundMatch = FALSE;
    uint8_t uuid_dk_service[2] = {UuidArray(gBleSig_CCC_DK_UUID_d)};
    while (index < pData->dataLength)
    {
        gapAdStructure_t adElement;

        adElement.length = pData->data[index];
        adElement.adType = (gapAdType_t)pData->data[index + 1U];
        adElement.aData = &pData->data[index + 2U];

         /* Search for DK Ranging Service */
        if ((adElement.adType == gAdIncomplete16bitServiceList_c) ||
          (adElement.adType == gAdComplete16bitServiceList_c))
        {
            foundMatch = BluetoothLEHost_MatchDataInAdvElementList(&adElement, &uuid_dk_service, 2);
        }

        /* Move on to the next AD element type */
        index += (uint32_t)adElement.length + sizeof(uint8_t);
    }

    if (foundMatch)
    {
        /* Update UI */
        TRACE_HEX_LE("Legacy ADV", pData->aAddress, gcBleDeviceAddressSize_c);
    }
    return foundMatch;
}

/*! *********************************************************************************
* \brief        Process scanning events to search for the DK Ranging Service.
*               This function is called from the scanning callback.
*
* \param[in]    pData                   Pointer to gapExtScannedDevice_t.
*
* \return       TRUE if the scanned device implements the DK Ranging Service,
                FALSE otherwise
********************************************************************************** */
static bool_t CheckScanEventExtended(gapExtScannedDevice_t* pData)
{
    uint32_t index = 0;
    bool_t foundMatch = FALSE;
    uint8_t uuid_dk_service[2] = {UuidArray(gBleSig_CCC_DK_UUID_d)};
    while (index < pData->dataLength)
    {
        gapAdStructure_t adElement;

        adElement.length = pData->pData[index];
        adElement.adType = (gapAdType_t)pData->pData[index + 1U];
        adElement.aData = &pData->pData[index + 2U];

         /* Search for DK Ranging Service */
        if ((adElement.adType == gAdIncomplete16bitServiceList_c) ||
          (adElement.adType == gAdComplete16bitServiceList_c))
        {
            foundMatch = BluetoothLEHost_MatchDataInAdvElementList(&adElement, &uuid_dk_service, 2);
        }

        /* Move on to the next AD element type */
        index += (uint32_t)adElement.length + sizeof(uint8_t);
    }

    if (foundMatch)
    {
        /* Update UI */
        TRACE_HEX_LE("Extended LR ADV", pData->aAddress, gcBleDeviceAddressSize_c);
    }
    return foundMatch;
}

/*! *********************************************************************************
* \brief        Stop scanning after a given time (gScanningTime_c).
                Called on timer task.
*
* \param[in]    pParam              not used
********************************************************************************** */
#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
static void ScanningTimeoutTimerCallback(void* pParam)
{
    /* Stop scanning */
    if (mScanningOn)
    {
        (void)Gap_StopScanning();
    }
}
#endif
/*! *********************************************************************************
* \brief        Callback for incoming PSM data.
*
* \param[in]    deviceId        The device ID of the connected peer that sent the data
* \param[in]    lePsm           Channel ID
* \param[in]    pPacket         Pointer to incoming data
* \param[in]    packetLength    Length of incoming data
********************************************************************************** */
static void BleApp_L2capPsmDataCallback (deviceId_t     deviceId,
                                         uint16_t       lePsm,
                                         uint8_t*       pPacket,
                                         uint16_t       packetLength)
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(appEventL2capPsmData_t) + (uint32_t)packetLength);
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_L2capPsmDataCallback_c;
            pEventData->eventData.pData = pEventData + 1;
            appEventL2capPsmData_t *pL2capPsmDataEvent = pEventData->eventData.pData;
            pL2capPsmDataEvent->deviceId = deviceId;
            pL2capPsmDataEvent->lePsm = lePsm;
            pL2capPsmDataEvent->packetLength = packetLength;
            pL2capPsmDataEvent->pPacket = (uint8_t*)(pL2capPsmDataEvent + 1);
            FLib_MemCpy(pL2capPsmDataEvent->pPacket, pPacket, packetLength);
            pL2capPsmDataEvent = NULL;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
}

/*! *********************************************************************************
* \brief        Callback for control messages.
*
* \param[in]    pMessage    Pointer to control message
********************************************************************************** */
static void BleApp_L2capPsmControlCallback(l2capControlMessage_t* pMessage)
{
    switch (pMessage->messageType)
    {
        case gL2ca_LePsmConnectRequest_c:
        {
            /* This message is unexpected, the DK Device sends Conn Req and expects a response */

            break;
        }
        case gL2ca_LePsmConnectionComplete_c:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(l2caLeCbConnectionComplete_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_L2capPsmControlCallback_LePsmConnectionComplete_c;
                    pEventData->eventData.pData = pEventData + 1;
                    FLib_MemCpy(pEventData->eventData.pData, &pMessage->messageData.connectionComplete, sizeof(l2caLeCbConnectionComplete_t));
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
            break;
        }
        case gL2ca_LePsmDisconnectNotification_c:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_L2capPsmControlCallback_LePsmDisconnectNotification_c;
                    pEventData->eventData.peerDeviceId = pMessage->messageData.disconnection.deviceId;
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
            break;
        }
        case gL2ca_NoPeerCredits_c:
        {
            if(mpfBleEventHandler != NULL)
            {
                appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(l2caLeCbNoPeerCredits_t));
                if(pEventData != NULL)
                {
                    pEventData->appEvent = mAppEvt_L2capPsmControlCallback_NoPeerCredits_c;
                    pEventData->eventData.pData = pEventData + 1;
                    FLib_MemCpy(pEventData->eventData.pData, &pMessage->messageData.noPeerCredits, sizeof(l2caLeCbNoPeerCredits_t));
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
            break;
        }
        case gL2ca_Error_c:
        {
            /* Handle error */
            break;
        }
        default:
            ; /* For MISRA compliance */
            break;
    }
}

/*! *********************************************************************************
* \brief  This is the callback for Bluetooth LE Host stack initialization.
********************************************************************************** */
static void BluetoothLEHost_Initialized(void)
{
    uint8_t mPeerId = 0;

    /* Common GAP configuration */
    BleConnManager_GapCommonConfig();

    /* Register for callbacks*/
    (void)App_RegisterGattServerCallback(BleApp_GattServerCallback);
    (void)GattServer_RegisterHandlesForWriteNotifications(NumberOfElements(mSystemParamsValuesHandles), mSystemParamsValuesHandles);
    (void)App_RegisterGattClientProcedureCallback(BleApp_GattClientCallback);
    BleServDisc_RegisterCallback(BleApp_ServiceDiscoveryCallback);

    /* Initialize private variables */
    for (mPeerId = 0; mPeerId < (uint8_t)gAppMaxConnections_c; mPeerId++)
    {
        maPeerInformation[mPeerId].appState = mAppIdle_c;
        maPeerInformation[mPeerId].deviceId = gInvalidDeviceId_c;
    }

    /* By default, always start node as GAP central */
    mGapRole = gGapCentral_c;

    mAdvState.advOn = FALSE;

    mScanningOn = FALSE;
    mFoundDeviceToConnect = FALSE;

#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
    /* Allocate scan timeout timer */
    (void)TM_Open(mAppTimerId);
#endif
    /* Start Services */
    (void)Bks_Start();
    (void)Bps_Start();
    (void)Mss_Start();
    (void)Ups_Start();
    (void)Cs_Start();
    
    /* Register stack callbacks */
    (void)App_RegisterLeCbCallbacks(BleApp_L2capPsmDataCallback, BleApp_L2capPsmControlCallback);
}
/*! *********************************************************************************
* @}
********************************************************************************** */
