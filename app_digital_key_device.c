/*! *********************************************************************************
* \addtogroup Digital Key Device Application
* @{
********************************************************************************** */
/*! *********************************************************************************
* \file app_digital_key_device.c
*
* Copyright 2021-2023 NXP
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
#include "RNG_Interface.h"
#include "fsl_adapter_reset.h"
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
#include "fsl_shell.h"
#endif
#include "app.h"
#include "app_conn.h"
#include "app_nvm.h"
#include "gap_types.h"

#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
#include "PWR_Interface.h"
#include "PWR_Configuration.h"
#endif

/* BLE Host Stack */
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gap_interface.h"
#include "gatt_db_app_interface.h"
#include "l2ca_cb_interface.h"
#include "gatt_db_handles.h"

/* Profile / Services */
#include "digital_key_interface.h"

#include "ble_conn_manager.h"
#include "ble_service_discovery.h"

#include "digital_key_device.h"
#include "app_digital_key_device.h"
#include "shell_digital_key_device.h"

#include "SecLib.h"

#include "stdio.h"

#include "keyfob_manager.h"

#include "motion_sensor.h"

#include "trace.h"

#include "sensors.h"

#include "PWR_Interface.h"

#include "uwb_manager.h"

#include <phscaEseUtils.h>
#include <phscaEseHal.h>
#include <phscaEseLog.h>
#include <phscaEseDal.h>
#include "command.h"
#include "phscaEseProto7816_Api.h"

#include "fsl_spc.h"
#include "fsl_vbat.h"
#include "fsl_cmc.h"
/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/
#define mcLog                               1
#define mcConnectionwithRealVehicle         1
#define mcScanAfterSwitchToCentralRole      0
#define mcEncryptionKeySize_c              16
#define mcPacketMaxPayloadSize             64
#define mcRssiMaxCounter                   10
#define mcRssiCorrection                   20
#define mcRssiTimeoutInMicroseconds    30000U
#define mcLogTimeoutInSeconds             10U
#define mcTemperatureTimeout               10
#define mcCalculateNewFilteredRssi(rssi_sum)   ((rssi_sum)/(mcRssiMaxCounter))
#define mcMaxSameIntentsCount(timeout_between_same_intents_ms, connection_interval_ms, rssi_max_counter)  (((timeout_between_same_intents_ms) / ((connection_interval_ms) * (rssi_max_counter))))
#define mcMaxTemperatureCount(temprature_count_ms, connection_interval_ms)  ((temprature_count_ms) / (connection_interval_ms))

/************************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
************************************************************************************/
int8_t temperature_value;
uint8_t battery_level;
appPeerInfo_t maPeerInformation[gAppMaxConnections_c];
gattCharacteristic_t maCharacteristics[mcNumCharacteristics_c]; /* Index 0 - Vehicle PSM; Index 1 - Vehicle Antenna ID; Index 2 - Tx Power  */
uint8_t mValVehiclePsm[2];
uint16_t mValVehicleAntennaId;
int8_t mValTxPower;
uint8_t mCurrentCharReadingIndex;
//uint8_t BD_Addr[6] = {0xBC, 0x09, 0x63, 0xAC, 0xB4, 0xB2};

TIMER_MANAGER_HANDLE_DEFINE(rssiTmrId);
TIMER_MANAGER_HANDLE_DEFINE(logTmrId);

/* Which peer are we doing OOB pairing with? */
deviceId_t mCurrentPeerId = gInvalidDeviceId_c;

gapRole_t mGapRole;
extern advState_t mAdvState;
extern bool_t   mFoundDeviceToConnect;
extern bool_t   mScanningOn;
extern uint8_t buf[200];
extern volatile uint32_t step_without_scan;
extern volatile uint32_t step_while_scan;
extern uint32_t total_step_count;
extern uint32_t still_detected_count;
/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/
typedef enum appIntent_tag
{
    App_LowIntent = 0,
    App_MediumIntent,
    App_HighIntent,
    App_Undefined
}appIntent_t;

typedef enum apppacketId_tag
{
    App_Rssi0Id = 0,
	App_FilteredRssiId,
    App_TemperatureId,
    App_BatteryLevelId,
    App_VehiculeStatusId,
    App_Ranger5StatusId,
    App_StepCountId,
    App_BleParametersId
}apppacketId_t;

typedef enum apduId_tag
{
    Select_ApduId = 0,
	Auth0_ApduId,
    Auth1_ApduId,
    CreateRangingKey_ApduId,
	ControlFlow_ApduId,
	Max_ApduId
}apduId_t;

typedef struct apduItem_tag
{
	apduId_t apduId;
    uint16_t apduLength;
}apduItem_t;

static const apduItem_t ApduRegistry[] =
{
    /* ID                               length */
    {Select_ApduId,                     gSelectRespPayloadLength            },
    {Auth0_ApduId,                      gAuthent0RespPayloadLength          },
    {Auth1_ApduId,                      gAuthent1RespPayloadLength          },
    {CreateRangingKey_ApduId,           gCreateRangingKeyRespPayloadLength  },
    {ControlFlow_ApduId,                gControlFlowRespPayloadLength       },
};
/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/
static gVehicleState_t mVehicleState = gStatusLocked_c;
static gUWBState_t mUWBState = gUWBNoRanging_c;

static int8_t mRssi0 = 0;
static int8_t mNewFilteredRssi = 0;
static appIntent_t mLastIntent = App_Undefined;
static uint8_t mRssiCounter = 0;
static int32_t mRssiSum = 0;
static bool_t mValidRssi0 = false;
static uint32_t mSameIntentCount = 0;
static uint32_t mTemperatureCount = 0;
static uint32_t mRssiActiveCount = 0;

#if defined(gAppUseBonding_d) && (gAppUseBonding_d)
static bool_t mRestoringBondedLink = FALSE;
#endif

#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1) 
/* LTK */
static uint8_t gaAppSmpLtk[gcSmpMaxLtkSize_c];

/* RAND*/
static uint8_t gaAppSmpRand[gcSmpMaxRandSize_c];

/* IRK */
static uint8_t gaAppSmpIrk[gcSmpIrkSize_c];

/* Address */
static uint8_t gaAppAddress[gcBleDeviceAddressSize_c];
static gapSmpKeys_t gAppOutKeys = {
    .cLtkSize = mcEncryptionKeySize_c,
    .aLtk = (void *)gaAppSmpLtk,
    .aIrk = (void *)gaAppSmpIrk,
    .aCsrk = NULL,
    .aRand = (void *)gaAppSmpRand,
    .cRandSize = gcSmpMaxRandSize_c,
    .ediv = 0,
    .addressType = 0,
    .aAddress = gaAppAddress
};
static gapSmpKeyFlags_t gAppOutKeyFlags;
static bool_t gAppOutLeSc;
static bool_t gAppOutAuth;
#endif

static uint8_t maOutCharReadBuffer[mCharReadBufferLength_c];
static uint16_t mOutCharReadByteCount;

/* Own address used during discovery. Included in First Approach Response. */
static uint8_t gaAppOwnDiscAddress[gcBleDeviceAddressSize_c];

/* Time Sync UWB Device Time. Demo value. */
static uint64_t mTsUwbDeviceTime = 0U;
/* 
Global used to identify if bond whas added by
shell command or connection with the device 
mBondAddedFromShell = FALSE bond created by connection
mBondAddedFromShell = TRUE  bond added by shell command
*/
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)  
static bool_t mBondAddedFromShell = FALSE;
#endif
/* Get a simulated UWB clock. */
static uint64_t GetUwbClock(void);
/************************************************************************************
*************************************************************************************
* Private functions prototypes
*************************************************************************************
************************************************************************************/
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)  
static void BleApp_Disconnect(void);
static void BleApp_SetBondingData(appEventData_t *pEventData);
static void BleApp_RemoveBondingData(appEventData_t *pEventData);
static void AppPrintLePhyEvent(gapPhyEvent_t* pPhyEvent);
static void PrintLePhyEvent(gapPhyEvent_t* pPhyEvent);
static void BleApp_ListBondingData(void);
static bleResult_t SetBondingData(uint8_t nvmIndex, bleAddressType_t addressType,
                                  uint8_t* ltk, uint8_t* irk, uint8_t* address);
#endif

/* Parsing */
static void BleApp_ParsingVehicleStatusChangedSubEvent(uint8_t *pPacket, uint16_t packetlength);
static void BleApp_ParsingRKERequestSubEvent(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t *pPacket);
static void BleApp_ParsingRKEAuthentication(uint8_t *pPacket, uint16_t packetLength, uintn8_t *pRKEChallenge, uintn8_t RKEChallengeLen);
static void BleApp_ParsingRangingIntentSubEvent(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t *packet);
static void BleApp_ParsingRangingSessionResponse(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t * packet);
static void BleApp_ParsingRangingRecoveryResponse(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t * packet);
static void BleApp_ParsingRangingSuspendResponse(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t * packet);
static void BleApp_ParsingRangingSessionRequest(uint8_t * packet);
static void BleApp_ParsingRangingRecoveryRequest(uint8_t * packet);
static void BleApp_ParsingRangingSuspendRequest(uint8_t * packet);

static void BleApp_ParsingSelectReq(uint8_t * packet);
static void BleApp_ParsingAuthent0Req(uint8_t * packet);
static void BleApp_ParsingAuthent1Req(uint8_t * packet);
static void BleApp_ParsingControlFlowReq(uint8_t * packet);
static void BleApp_ParsingCreateRangingKeyReq(uint8_t * packet);
static void BleApp_ParsingSendRangingCapabilityReq(uint8_t * packet);

/* CCC Time Sync */
static bleResult_t CCC_SendTimeSync(deviceId_t deviceId,
                                    uint64_t *pDevEvtCnt,
                                    uint64_t *pUwbDevTime,
                                    uint8_t success);

/* CCC SubEvents */
static bleResult_t CCC_SendCommandCompleteSubEvent(deviceId_t deviceId,
                                                   dkSubEventCategory_t category,
                                                   dkSubEventCommandCompleteType_t type);

static bleResult_t CCC_SendRangingIntentSubEvent(deviceId_t deviceId,
                                                 dkSubEventCategory_t category,
                                                 dkSubEventDeviceRangingIntentType_t type);

static bleResult_t CCC_SendRKERequestSubEvent(deviceId_t deviceId ,gDkRKEFunctionId_t function, uint8_t action);

/* CCC RKE_Auth_RS */
bleResult_t CCC_SendRKEAuthResponse(deviceId_t deviceId, uintn8_t *pAttestation, uintn8_t attestationLen);

/* CCC Ranging_Session_RS */
static bleResult_t CCC_SendRangingSessionRS(deviceId_t deviceId);

/* CCC Ranging_Recovery_RS */
static bleResult_t CCC_SendRangingRecoveryRS(deviceId_t deviceId);

/* CCC Ranging_Suspend_RS */
static bleResult_t CCC_SendRangingSuspendRS(deviceId_t deviceId);

/* CCC Ranging_Capability_RS */
static bleResult_t CCC_SendRangingCapabilityRS(deviceId_t deviceId);

/* CCC Phase 2 */
static bleResult_t CCCPhase2_SendSPAKEResponse(deviceId_t deviceId, uint8_t *pData, uint16_t dataLen);
static bleResult_t CCCPhase2_SendSPAKEVerify(deviceId_t deviceId, uint8_t *pData, uint16_t dataLen);

static bleResult_t CCC_FirstApproachReq
(
    deviceId_t deviceId,
    uint8_t* pBdAddr,
    gapLeScOobData_t* pOobData
);

static bleResult_t CCC_StandardTransactionReq
(
    deviceId_t deviceId
);

static bleResult_t CCC_SendAPDUResp
(
    deviceId_t deviceId,
	apduId_t Id
);

static void CCC_DeriveArbitraryData(uintn8_t *pRkeChallenge, uintn8_t RkeChallengeLen, uint16_t function, uintn8_t action, uintn8_t *pHashOut, uintn8_t hashOutLen);
static void CCC_SignArbitraryData(uintn8_t *pArbitraryData, uintn8_t arbitraryDataLen, uintn8_t *pAttestationOut);
static void App_HandleKeys(appEventData_t *pEventData);
static void App_HandleRKECommands(appEventData_t *pEventData);
static void App_HandleGattClientCallback(appEventData_t *pEventData);
static void App_HandleGenericCallback(appEventData_t *pEventData);
static void App_HandleConnectionCallback(appEventData_t *pEventData);
static void App_HandleL2capPsmDataCallback(appEventData_t *pEventData);
static void App_HandleL2capPsmControlCallback(appEventData_t *pEventData);

static void BleApp_HandlePreIdleState(deviceId_t peerDeviceId, appEvent_t event);
static void BleApp_HandleIdleState(deviceId_t peerDeviceId, appEvent_t event);
static void BleApp_HandleServiceDiscState(deviceId_t peerDeviceId, appEvent_t event);
static void BleApp_HandlePairState(deviceId_t peerDeviceId, appEvent_t event);

static void BleApp_StartRssiSensing(void);
static void BleApp_RssiTimeoutTimerCallback(void* pParam);
static appIntent_t BleApp_DecideIntentFromRSSI(int8_t newFilteredRssi, int8_t rssi0);

static void BleApp_StartLogTimer(void);
static void BleApp_LogTimeoutTimerCallback(void* pParam);

static void BleApp_SendPacketToCarAnchor(deviceId_t deviceId);

static void BleApp_SwitchGapRole(appEventData_t *pEventData);
static void Array_hex_string(uint8_t* array_hex, int len, char array_string[]);
static void Array_string_hex(uint8_t *array_string, size_t input_length, uint8_t *array_hex);
/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/
void SetSePower(appSePowerState_t se_power_state)
{
    /* Power Switch On */
    spc_intergrated_power_switch_config_t smtpwrswActiveConfig;
    if (se_power_state == mAppSePoweredOn_c)
    {
        smtpwrswActiveConfig.wakeup = true;
        smtpwrswActiveConfig.sleep = false;
        SPC_SetActiveModeIntegratedPowerSwitchConfig(SPC0, &smtpwrswActiveConfig);
        /* Verify the proper status of the switch */
        while(SPC_CheckPowerSwitchState(SPC0) != true);
        TRACE_INFO("Vout switch closed --> SE is powered On");
    }
    else
    {
        smtpwrswActiveConfig.wakeup = false;
        smtpwrswActiveConfig.sleep = true;
        SPC_SetActiveModeIntegratedPowerSwitchConfig(SPC0, &smtpwrswActiveConfig);
        /* Verify the proper status of the switch */
        while(SPC_CheckPowerSwitchState(SPC0) != false);
        TRACE_INFO("Vout switch opened --> SE is powered Off");
    }
}
/*! *********************************************************************************
* \brief        State machine handler of the Digital Key Device application.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    event               Event type.
********************************************************************************** */
void BleApp_StateMachineHandler(deviceId_t peerDeviceId, appEvent_t event)
{
    switch (maPeerInformation[peerDeviceId].appState)
    {
        case mAppIdle_c:
        {
            BleApp_HandleIdleState(peerDeviceId, event);
        }
        break;

        case mAppExchangeMtu_c:
        {
            if (event == mAppEvt_GattProcComplete_c)
            {
                bleUuid_t dkServiceUuid;

                /* Moving to Service Discovery State */
                maPeerInformation[peerDeviceId].appState = mAppServiceDisc_c;

                /* Start Service Discovery */
                dkServiceUuid.uuid16 = gBleSig_CCC_DK_UUID_d;
                (void)BleServDisc_FindService(peerDeviceId, gBleUuidType16_c, &dkServiceUuid);
            }
            else if (event == mAppEvt_GattProcError_c)
            {
                (void)Gap_Disconnect(peerDeviceId);
                KEYFOB_MGR_notify(KEYFOB_EVENT_BLE_CONNECTION_FAILURE);
            }
            else if ( event == mAppEvt_PeerDisconnected_c )
            {
                maPeerInformation[peerDeviceId].deviceId = gInvalidDeviceId_c;
                maPeerInformation[peerDeviceId].appState = mAppIdle_c;
                mRssi0 = 0;
                mValidRssi0 = false;
                mRssiCounter = 0;
                mRssiSum = 0;
                mLastIntent = App_Undefined;
                mSameIntentCount = 0;
                mVehicleState = gStatusLocked_c;
                mUWBState = gUWBNoRanging_c;
                KEYFOB_MGR_notify(KEYFOB_EVENT_BLE_DISCONNECTED);
            }
        }
        break;

        case mAppServiceDisc_c:
        {
            BleApp_HandleServiceDiscState(peerDeviceId, event);
        }
        break;

        case mAppRunning_c:
        {
            if ( event == mAppEvt_PeerDisconnected_c )
            {
                maPeerInformation[peerDeviceId].deviceId = gInvalidDeviceId_c;
                maPeerInformation[peerDeviceId].appState = mAppIdle_c;
                mRssi0 = 0;
                mValidRssi0 = false;
                mRssiCounter = 0;
                mRssiSum = 0;
                mLastIntent = App_Undefined;
                mSameIntentCount = 0;
                KEYFOB_MGR_notify(KEYFOB_EVENT_BLE_DISCONNECTED);
            }

            if(mGapRole == gGapCentral_c)
            {
                if ( event == mAppEvt_Read_Rssi_c )
                {
                    Gap_ReadRssi(maPeerInformation[peerDeviceId].deviceId);
                }
            }
        }
        break;

        case mAppGracefulReboot_c:
        {
            if ( event == mAppEvt_PeerDisconnected_c )
            {
                if(mGapRole == gGapCentral_c)
                {
                    HAL_ResetMCU();
                }
                else
                {
                    TRACE_INFO("Reset GAP context");
                    maPeerInformation[peerDeviceId].deviceId = gInvalidDeviceId_c;
                    maPeerInformation[peerDeviceId].appState = mAppIdle_c;
                }
            }
        }
        break;

        case mAppPreIdle_c:
        {
            BleApp_HandlePreIdleState(peerDeviceId, event);
        }
        break;

        case mAppCCCPhase2WaitingForRequest_c:
        {
            if (event == mAppEvt_SentSPAKEResponse_c)
            {
               maPeerInformation[peerDeviceId].appState = mAppCCCPhase2WaitingForVerify_c;
            }
        }
        break;

        case mAppCCCPhase2WaitingForVerify_c:
        {
            if (event == mAppEvt_ReceivedSPAKEVerify_c)
            {
                maPeerInformation[peerDeviceId].appState = mAppCCCWaitingForPairingReady_c;
            }
        }
        break;

        case mAppCCCWaitingForPairingReady_c:
        {
            if (event == mAppEvt_ReceivedPairingReady_c)
            {
                mCurrentPeerId = peerDeviceId;
                (void)Gap_LeScGetLocalOobData();
                maPeerInformation[peerDeviceId].appState = mAppPair;
            }
        }
        break;
        
        case mAppPair:
        {
            BleApp_HandlePairState(peerDeviceId, event);
        }
        break;

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*!*************************************************************************************************
 \fn     uint8_t APP_BleEventHandler(void *pData)
 \brief  This function is used to handle events from BLE

 \param  [in]   pData - pointer to data;
 ***************************************************************************************************/
void APP_BleEventHandler(void *pData)
{
    appEventData_t *pEventData = (appEventData_t *)pData;
    systemParameters_t *pSysParams = NULL;

    App_NvmReadSystemParams(&pSysParams);
    switch(pEventData->appEvent)
    {
        case mAppEvt_Shell_Reset_Command_c:
        {
            HAL_ResetMCU();
        }
        break;

        case mAppEvt_Shell_FactoryReset_Command_c:
        {
            (void)BleApp_FactoryReset();
        }
        break;
        
        case mAppEvt_Shell_ShellStartDiscovery_Command_c:
        {
            BleApp_Start(mGapRole,
                         gScanParamConversionMsToGapUnit((pSysParams->system_params).fields.slow_scan_interval),
                         gScanParamConversionMsToGapUnit((pSysParams->system_params).fields.slow_scan_window));
        }
        break;
        
        case mAppEvt_Shell_StopDiscovery_Command_c:
        {
            (void)Gap_StopScanning();
        }
        break;

#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
        case mAppEvt_Shell_SetBondingData_Command_c:
        {
            BleApp_SetBondingData(pEventData);
        }
        break;
        
        case mAppEvt_Shell_ListBondedDev_Command_c:
        {
            BleApp_ListBondingData();
        }
        break;
        
        case mAppEvt_Shell_RemoveBondedDev_Command_c:
        {
            BleApp_RemoveBondingData(pEventData);
        }
        break;
        
        case mAppEvt_Shell_Disconnect_Command_c:
        {
            BleApp_Disconnect();
        }
        break;

        case mAppEvt_Shell_ResetAfterDisconnection_Command_c:
        {
            maPeerInformation[pEventData->eventData.peerDeviceId].appState = mAppGracefulReboot_c;
            BleApp_Disconnect();
        }
        break;

        case mAppEvt_Shell_SwitchGAPRole_Command_c:
        {
            BleApp_SwitchGapRole(pEventData);
        }
        break;

#endif /* gAppUseShellInApplication_d */

        case mAppEvt_KBD_EventPressPB1_c:
        case mAppEvt_KBD_EventLongPB1_c:
        case mAppEvt_KBD_EventVeryLongPB1_c:
        case mAppEvt_KBD_EventPressPB2_c:
        case mAppEvt_KBD_EventLongPB2_c:
        case mAppEvt_KBD_EventPressPB3_c:
        {
            App_HandleKeys(pEventData);
            break;
        }

        case mAppEvt_GattClientCallback_GattProcError_c:
        case mAppEvt_GattClientCallback_GattProcReadCharacteristicValue_c:
        case mAppEvt_GattClientCallback_GattProcReadUsingCharacteristicUuid_c:
        case mAppEvt_GattClientCallback_GattProcComplete_c:
        {
            App_HandleGattClientCallback(pEventData);
            break;
        }

        case mAppEvt_ServiceDiscoveryCallback_DiscoveryFinishedWithSuccess_c:
        {
            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_ServiceDiscoveryComplete_c);
            break;
        }

        case mAppEvt_ServiceDiscoveryCallback_DiscoveryFinishedFailed_c:
        {
            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_ServiceDiscoveryFailed_c);
            break;
        }

        case mAppEvt_GenericCallback_PeerDisconnected_c:
        {
            /* Factory reset may trigger internal error in some scenarios. */
            BleApp_StateMachineHandler( mCurrentPeerId, mAppEvt_PeerDisconnected_c );
            break;
        }

        case mAppEvt_GenericCallback_LePhyEvent_c:
        case mAppEvt_GenericCallback_LeScLocalOobData_c:
        case mAppEvt_GenericCallback_RandomAddressReady_c:
        case mAppEvt_GenericCallback_CtrlNotifEvent_c:
        case mAppEvt_GenericCallback_BondCreatedEvent_c:
        {
            App_HandleGenericCallback(pEventData);
            break;
        }

        case mAppEvt_ConnectionCallback_ConnEvtConnected_c:
        case mAppEvt_ConnectionCallback_ConnEvtDisconnected_c:
        case mAppEvt_ConnectionCallback_ConnEvtLeScOobDataRequest_c:
        case mAppEvt_ConnectionCallback_ConnEvtPairingComplete_c:
        case mAppEvt_ConnectionCallback_ConnEvtEncryptionChanged_c:
        case mAppEvt_ConnectionCallback_ConnEvtAuthenticationRejected_c:
        case mAppEvt_ConnectionCallback_ReadRssiEvtConnected_c:
        {
            App_HandleConnectionCallback(pEventData);
            break;
        }

        case mAppEvt_L2capPsmDataCallback_c:
        {
            App_HandleL2capPsmDataCallback(pEventData);
            break;
        }

        case mAppEvt_L2capPsmControlCallback_LePsmConnectionComplete_c:
        case mAppEvt_L2capPsmControlCallback_LePsmDisconnectNotification_c:
        case mAppEvt_L2capPsmControlCallback_NoPeerCredits_c:
        {
            App_HandleL2capPsmControlCallback(pEventData);
            break;
        }

        case mAppEvt_Shell_RKELock_Command_c:
        case mAppEvt_Shell_RKEUnlock_Command_c:
        case mAppEvt_Shell_RKERelease_Command_c:
        {
            App_HandleRKECommands(pEventData);
            break;
        }

        case mAppEvt_Read_Rssi_c:
        {
            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_Read_Rssi_c);
            break;
        }

        default:
        {
            ; /* No action required */
        }
        break;
    }
    
    (void)MEM_BufferFree(pData);
    pData = NULL;
    
}

void BleApp_SetBondingDataOfBMWVehicle()
{
    bleResult_t status = gBleSuccess_c;
    IrkLtkKeys_t *pBleKeys = NULL;
    App_NvmReadBleKeys(&pBleKeys);

    status = SetBondingData(0x00, gBleAddrTypePublic_c, ((pBleKeys->ble_keys).vkeys.LTK), ((pBleKeys->ble_keys).vkeys.IRK),
    		                ((pBleKeys->ble_keys).vkeys.BD_ADDR));
    if(status != gBleSuccess_c)
    {
        TRACE_HEX("Bonding failed with status", (uint8_t*)&status, 2);
    }
    else
    {
#if defined(gAppUseBonding_d) && (gAppUseBonding_d == 1)
       (void)Gap_AddDeviceToFilterAcceptList(gBleAddrTypePublic_c, (uint8_t*)((pBleKeys->ble_keys).vkeys.BD_ADDR));
#endif
    }
}
/*!*************************************************************************************************
 \fn     gVehicleState_t GetVehicleState()
 \brief  This function is used to get the car status (locked or unlocked)

 ***************************************************************************************************/
gVehicleState_t GetVehicleState(void)
{
    return mVehicleState;
}

/*!*************************************************************************************************
 \fn     gUWBState_t GetUWBState()
 \brief  This function is used to check if there is an UWB ranging session or not.

 ***************************************************************************************************/
gUWBState_t GetUWBState(void)
{
    /* TODO: Get the real UWB state */
    return mUWBState;
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/
/*! *********************************************************************************
* \brief        Start RSSI reading.
*
************************************************************************************/
static void BleApp_StartRssiSensing(void)
{
    timer_status_t tmrStatus;

    TM_Close(rssiTmrId);
    tmrStatus = TM_Open(rssiTmrId);
    if (tmrStatus == kStatus_TimerSuccess)
    {
        (void)TM_InstallCallback((timer_handle_t)rssiTmrId, BleApp_RssiTimeoutTimerCallback, NULL);
        (void)TM_Start((timer_handle_t)rssiTmrId, (uint8_t)kTimerModeSetMicrosTimer, mcRssiTimeoutInMicroseconds);
    }
}

/*! *********************************************************************************
* \brief        Rssi timer callback.
                Called on timer task.
*
* \param[in]    pParam              not used
********************************************************************************** */
static void BleApp_RssiTimeoutTimerCallback(void* pParam)
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Read_Rssi_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
}

/*! *********************************************************************************
* \brief        Decide new intent according to new rssi, old rssi and old intent.
*
* \param[in]    newFilteredRssi      new filtered rssi value
* \param[in]    Rssi0                first filtered rssi value
********************************************************************************** */
static appIntent_t BleApp_DecideIntentFromRSSI(int8_t newFilteredRssi, int8_t rssi0)
{
    appIntent_t intent;
    systemParameters_t *pSysParams = NULL;

    App_NvmReadSystemParams(&pSysParams);
    if(newFilteredRssi >= (rssi0 + (pSysParams->system_params).fields.delta_rssi_high))
    {
        intent = App_HighIntent;
    }
    else if(newFilteredRssi >= (rssi0 + (pSysParams->system_params).fields.delta_rssi_medium))
    {
        intent = App_MediumIntent;
    }
    else if(newFilteredRssi >= (rssi0 + (pSysParams->system_params).fields.delta_rssi_low))
    {
        intent = App_LowIntent;
    }
    else
    {
        intent = App_Undefined;
    }
    return intent;
}

/*! *********************************************************************************
* \brief        Start sending logs.
*
************************************************************************************/
static void BleApp_StartLogTimer(void)
{
    timer_status_t tmrStatus;

    TM_Close(logTmrId);
    tmrStatus = TM_Open(logTmrId);
    if (tmrStatus == kStatus_TimerSuccess)
    {
        (void)TM_InstallCallback((timer_handle_t)logTmrId, BleApp_LogTimeoutTimerCallback, NULL);
        (void)TM_Start((timer_handle_t)logTmrId, (uint8_t)kTimerModeSetSecondTimer, mcLogTimeoutInSeconds);
    }
}

/*! *********************************************************************************
* \brief        Rssi timer callback.
                Called on timer task.
*
* \param[in]    pParam              not used
********************************************************************************** */
static void BleApp_LogTimeoutTimerCallback(void* pParam)
{
	temperature_value = Get_Ms_Temp_Value();
	battery_level = SENSORS_GetBatteryLevel();
	/************************ONLY FOR TESTING PURPOSES: IT VIOLATES THE CCC STANDARD***********************/
    /* Sending the temperature value */
    BleApp_SendPacketToCarAnchor(mCurrentPeerId);
    /******************************************************************************************************/
}

/*! *********************************************************************************
* \brief        Sends packet to car anchor.
*
* \param[in]    deviceId      Device ID
* \param[in]    packetId      PacketID
* \param[in]    packet        Packet value
********************************************************************************** */
static void BleApp_SendPacketToCarAnchor(deviceId_t deviceId)
{
    uint8_t pPacket[mcPacketMaxPayloadSize];
    uint8_t packetLength = 0;
    systemParameters_t *pSysParams = NULL;

    App_NvmReadSystemParams(&pSysParams);
    pPacket[packetLength++] = gDKMessageTypeLogMessage_c;
    pPacket[packetLength++] = App_Rssi0Id;
    pPacket[packetLength++] = sizeof(mRssi0);
    pPacket[packetLength++] = mRssi0;
    pPacket[packetLength++] = App_FilteredRssiId;
    pPacket[packetLength++] = sizeof(mNewFilteredRssi);
    pPacket[packetLength++] = mNewFilteredRssi;
    pPacket[packetLength++] = App_TemperatureId;
    pPacket[packetLength++] = sizeof(temperature_value);
    pPacket[packetLength++] = temperature_value;
    pPacket[packetLength++] = App_BatteryLevelId;
    pPacket[packetLength++] = sizeof(battery_level);
    pPacket[packetLength++] = battery_level;
    pPacket[packetLength++] = App_VehiculeStatusId;
    pPacket[packetLength++] = sizeof(mVehicleState);
    pPacket[packetLength++] = mVehicleState;
    pPacket[packetLength++] = App_Ranger5StatusId;
    pPacket[packetLength++] = sizeof(mUWBState);
    pPacket[packetLength++] = mUWBState;
    pPacket[packetLength++] = App_StepCountId;
    pPacket[packetLength++] = sizeof(total_step_count);
    pPacket[packetLength++] = total_step_count;
    pPacket[packetLength++] = App_BleParametersId;
    pPacket[packetLength++] = sizeof(uint32_t);
    memcpy(&pPacket[packetLength++], pSysParams->system_params.buffer, 40);
    L2ca_SendLeCbData(deviceId,
                      maPeerInformation[deviceId].customInfo.psmChannelId,
					  pPacket,
					  mcPacketMaxPayloadSize);
}

/*! *********************************************************************************
* \brief        Switch GAP Role.
********************************************************************************** */
static void BleApp_SwitchGapRole(appEventData_t *pEventData)
{
    mCurrentPeerId = pEventData->eventData.peerDeviceId;
    /* Switch current role */
    if (mGapRole == gGapCentral_c)
    {
    	App_NvmWriteSystemParam(MsStepInScanID, step_while_scan);
    	App_NvmWriteSystemParam(MsStepOutScanID, step_without_scan);
    	App_NvmWriteSystemParam(MsTotalStepID, total_step_count);
    	App_NvmWriteSystemParam(MsStillDetectedID, still_detected_count);
    	App_NvmWriteSystemParam(TemperatureID, Get_Ms_Temp_Value());
    	App_NvmWriteSystemParam(BatteryLevelID, SENSORS_GetBatteryLevel());
    	Bps_Start();
    	/* Switch from CENTRAL to PERIPHERAL */
        if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppRunning_c)
        {
            /* Disconnect before switching Gap role */
            maPeerInformation[pEventData->eventData.peerDeviceId].appState = mAppPreIdle_c;
            (void)Gap_Disconnect(maPeerInformation[pEventData->eventData.peerDeviceId].deviceId);
        }
        else
        {
            if(mScanningOn)
            {
                maPeerInformation[pEventData->eventData.peerDeviceId].appState = mAppPreIdle_c;
                (void)Gap_StopScanning();
            }
            else
            {
                TRACE_INFO("Switch role to GAP Peripheral.");
                /* Tell the STATE MACHINE task to freeze since we will switch to PERIPHERAL role */
                KEYFOB_MGR_notify(KEYFOB_EVENT_ENTER_FREEZE);
                UWB_MGR_notify(UWB_EVENT_ENTER_FREEZE);
                BleApp_Start(gGapPeripheral_c, 0, 0);
            }
        }
        mAdvState.advOn = false;
        mGapRole = gGapPeripheral_c;
    }
    else
    {
        /* Disconnect before switching Gap role */
        if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppRunning_c)
        {
            maPeerInformation[pEventData->eventData.peerDeviceId].appState = mAppPreIdle_c;
            (void)Gap_Disconnect(maPeerInformation[pEventData->eventData.peerDeviceId].deviceId);
        }
        else
        {
            if(mAdvState.advOn)
            {
                maPeerInformation[pEventData->eventData.peerDeviceId].appState = mAppPreIdle_c;
                (void)Gap_StopAdvertising();
                Led2Off();
            }
            else
            {
                TRACE_INFO("Switch role to GAP Central.");
                /* Tell the STATE MACHINE task to unfreeze since we will switch back to CENTRAL role */
                KEYFOB_MGR_notify(KEYFOB_EVENT_EXIT_FREEZE);
                UWB_MGR_notify(UWB_EVENT_EXIT_FREEZE);
            }
        }
        mScanningOn = false;
        mFoundDeviceToConnect = FALSE;
        mGapRole = gGapCentral_c;
    }
}

/*! *********************************************************************************
* \brief        Covert an hex array to char array.
********************************************************************************** */
static void Array_hex_string(uint8_t* array_hex, int len, char array_string[])
{
    for (int i = 0; i < len; i++)
    {
        sprintf(&array_string[i * 2], "%02x", array_hex[i]);
    }

    array_string[len * 2] = '\0';
}

/*! *********************************************************************************
* \brief        Covert an char array to hex array.
********************************************************************************** */
static void Array_string_hex(uint8_t *array_string, size_t input_length, uint8_t *array_hex)
{
    for (size_t i = 0; i < input_length; i+=2)
    {
        char hex[3] = {array_string[i], array_string[i+1], '\0'};
        array_hex[i/2] = strtol(hex, NULL, 16);
    }
}

/*! *********************************************************************************
* \brief        Handler of the mAppPreIdle_c state for BleApp_StateMachineHandler.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    event               Event type.
************************************************************************************/
static void BleApp_HandlePreIdleState(deviceId_t peerDeviceId, appEvent_t event)
{
    if ( event == mAppEvt_SwitchRole || event == mAppEvt_PeerDisconnected_c)
    {
        if (mGapRole == gGapCentral_c)
        {
            TRACE_INFO("Switch role to GAP Central.");
            /* Tell the STATE MACHINE task to unfreeze since we will switch back to CENTRAL role */
            KEYFOB_MGR_notify(KEYFOB_EVENT_EXIT_FREEZE);
            UWB_MGR_notify(UWB_EVENT_EXIT_FREEZE);
        }
        else
        {
            TRACE_INFO("Switch role to GAP Peripheral.");
            /* Tell the STATE MACHINE task to freeze since we will switch to PERIPHERAL role */
            KEYFOB_MGR_notify(KEYFOB_EVENT_ENTER_FREEZE);
            UWB_MGR_notify(UWB_EVENT_ENTER_FREEZE);
            BleApp_Start(mGapRole, 0, 0);
        }
        if(event == mAppEvt_PeerDisconnected_c)
        {
            maPeerInformation[peerDeviceId].deviceId = gInvalidDeviceId_c;
            mRssi0 = 0;
            mValidRssi0 = false;
            mRssiCounter = 0;
            mRssiSum = 0;
            mLastIntent = App_Undefined;
            mSameIntentCount = 0;
        }
        /* Re-initialise the ble state machine in the mAppIdle_c state */
        maPeerInformation[peerDeviceId].appState = mAppIdle_c;
    }
    else
    {
        /* For MISRA compliance */
    }
}

/*! *********************************************************************************
* \brief        Handler of the mAppIdle_c state for BleApp_StateMachineHandler.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    event               Event type.
************************************************************************************/
static void BleApp_HandleIdleState(deviceId_t peerDeviceId, appEvent_t event)
{
    uint16_t tempMtu = 0;
        union
        {
            uint8_t     *pUuidArray;
            bleUuid_t   *pUuidObj;
        } temp; /* MISRA rule 11.3 */

        temp.pUuidArray = uuid_service_BLE_params;

    if (event == mAppEvt_PeerConnected_c)
    {
        if (mGapRole == gGapCentral_c)
        {
            if (maPeerInformation[peerDeviceId].customInfo.hPsmChannelChar == gGattDbInvalidHandle_d)
            {
                /* Moving to Exchange MTU State */
                maPeerInformation[peerDeviceId].appState = mAppExchangeMtu_c;
                (void)GattClient_ExchangeMtu(peerDeviceId, gAttMaxMtu_c);
            }
        }
        else
        {
            /* Moving to Service Discovery State*/
            maPeerInformation[peerDeviceId].appState = mAppServiceDisc_c;

            /* Start Service Discovery*/
            (void)BleServDisc_FindService(peerDeviceId,
                                          gBleUuidType128_c,
                                          temp.pUuidObj);
        }
    }
    else if ( event == mAppEvt_EncryptionChanged_c )
    {
    	TRACE_INFO("------------------------> mAppEvt_EncryptionChanged_c");
        bleUuid_t psmCharUuid;
        gattHandleRange_t handleRange;
        
        /* Moving to Service Discovery State*/
        maPeerInformation[peerDeviceId].appState = mAppServiceDisc_c;
        
        /* Read SPSM from vehicle. */
        FLib_MemCpy(psmCharUuid.uuid128, uuid_char_vehicle_psm, gcBleLongUuidSize_c);
        handleRange.startHandle = 0x0001U;
        handleRange.endHandle = 0xFFFFU;
        mCurrentCharReadingIndex = mcCharVehiclePsmIndex_c;
        (void)GattClient_ReadUsingCharacteristicUuid(peerDeviceId,
                                                    gBleUuidType128_c,
                                                    &psmCharUuid,
                                                    &handleRange,
                                                    maOutCharReadBuffer,
                                                    mCharReadBufferLength_c,
                                                    &mOutCharReadByteCount);
    }
    else if ( event == mAppEvt_AuthenticationRejected_c )
    {
        /* Something went wrong - peer has likely lost the bond.
           Must pair again, move to Exchange MTU */
    	TRACE_INFO("------------------------> mAppEvt_AuthenticationRejected_c");
        maPeerInformation[peerDeviceId].appState = mAppExchangeMtu_c;
        maPeerInformation[peerDeviceId].isBonded = FALSE;
        (void)GattClient_ExchangeMtu(peerDeviceId, gAttMaxMtu_c);
    }
    else
    {
        /* For MISRA compliance */
    }
}

/*! *********************************************************************************
* \brief        Handler of the mAppServiceDisc_c state for BleApp_StateMachineHandler.            
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    event               Event type.
************************************************************************************/
static void BleApp_HandleServiceDiscState(deviceId_t peerDeviceId, appEvent_t event)
{
    if (event == mAppEvt_ReadCharacteristicValueComplete_c)
    {
        (void)L2ca_ConnectLePsm((uint16_t)Utils_BeExtractTwoByteValue(maCharacteristics[mcCharVehiclePsmIndex_c].value.paValue),
                                peerDeviceId, mAppLeCbInitialCredits_c);
    }

    if (event == mAppEvt_PsmChannelCreated_c)
    {
/*#if defined(mcConnectionwithRealVehicle) && (mcConnectionwithRealVehicle == 1)
    	maPeerInformation[peerDeviceId].isBonded = TRUE;
#endif*/
    	if (maPeerInformation[peerDeviceId].isBonded)
        {
            uint64_t devEvtCnt = 0U;
            /* Update UI */
            Led1On();
            /* Send Time Sync */
            //(void)CCC_SendTimeSync(peerDeviceId, &devEvtCnt, &mTsUwbDeviceTime, 1U);
#ifdef BMW_KEYFOB_EVK_BOARD
    /* No functions required */
#else
            TRACE_DEBUG("Temperature measured : %d°C",Get_Ms_Temp_Value());
#endif /* BMW_KEYFOB_EVK_BOARD */
            TRACE_DEBUG("Battery level : %d%%",SENSORS_GetBatteryLevel());
            maPeerInformation[peerDeviceId].appState = mAppRunning_c;
            /* Update connection interval to CCC recommended value */
            systemParameters_t *pSysParams = NULL;
            App_NvmReadSystemParams(&pSysParams);

            (void)Gap_UpdateConnectionParameters(peerDeviceId,
                                                 gConnectionParamConversionMsToGapUnit((pSysParams->system_params).fields.connection_interval),
                                                 gConnectionParamConversionMsToGapUnit((pSysParams->system_params).fields.connection_interval),
                                                 gConnReqParams.connLatency,
                                                 gConnReqParams.supervisionTimeout,
                                                 gConnReqParams.connEventLengthMin,
                                                 gConnReqParams.connEventLengthMax);
            KEYFOB_MGR_notify(KEYFOB_EVENT_BLE_CONNECTION_SUCCESS);
            UWB_MGR_notify(UWB_EVENT_BLE_CONNECTED);
            /* send standard transaction request */
            CCC_StandardTransactionReq(peerDeviceId);
            /* Power_on the SE */
            SetSePower(mAppSePoweredOn_c);
            (void)phscaEseDal_Platform_Init();
            doCommands();
            if((pSysParams->system_params).fields.rssi_on_duration != 0)
            {
                BleApp_StartRssiSensing();
            }
        }
        else
        {
            bleResult_t status = gBleUnexpectedError_c;
            /* Send request_owner_pairing */
            TRACE_INFO("Sending Command Complete SubEvent: Request_owner_pairing");
            status =  CCC_SendCommandCompleteSubEvent(peerDeviceId, gCommandComplete_c, gRequestOwnerPairing_c);
            if (status == gBleSuccess_c)
            {
                maPeerInformation[peerDeviceId].appState = mAppCCCPhase2WaitingForRequest_c;
            }
        }
    }
    if (event == mAppEvt_ServiceDiscoveryComplete_c)
    {
        if(mGapRole == gGapCentral_c)
        {
            maPeerInformation[peerDeviceId].appState = mAppServiceDisc_c;
        }
        else
        {
            maPeerInformation[peerDeviceId].appState = mAppRunning_c;
        }
    }
    if (event == mAppEvt_ServiceDiscoveryFailed_c)
    {
        (void)Gap_Disconnect(peerDeviceId);
        KEYFOB_MGR_notify(KEYFOB_EVENT_BLE_CONNECTION_FAILURE);
    }
}

/*! *********************************************************************************
* \brief        Handler of the mAppPair state for BleApp_StateMachineHandler.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    event               Event type.
************************************************************************************/
static void BleApp_HandlePairState(deviceId_t peerDeviceId, appEvent_t event)
{
    if ( event == mAppEvt_PairingLocalOobData_c )
    {
        TRACE_INFO("Sending First_Approach_RQ");
        (void)CCC_FirstApproachReq(peerDeviceId, gaAppOwnDiscAddress, &maPeerInformation[peerDeviceId].oobData);
    }
    else if ( event == mAppEvt_PairingPeerOobDataRcv_c )
    {
#if defined(mcConnectionwithRealVehicle) && (mcConnectionwithRealVehicle == 0)
        TRACE_INFO("Received First_Approach_RS.");
#endif
        TRACE_INFO("Pairing...");
        (void)Gap_Pair(peerDeviceId, &gPairingParameters);
    }
    else if ( event == mAppEvt_PairingPeerOobDataReq_c )
    {
        (void)Gap_LeScSetPeerOobData(peerDeviceId, &maPeerInformation[peerDeviceId].peerOobData);
    }
    else if ( event == mAppEvt_PairingComplete_c )
    {
        uint64_t devEvtCnt = 0U;
        systemParameters_t *pSysParams = NULL;
        App_NvmReadSystemParams(&pSysParams);
        FLib_MemSet(&maPeerInformation[peerDeviceId].oobData, 0x00, sizeof(gapLeScOobData_t));
        FLib_MemSet(&maPeerInformation[peerDeviceId].peerOobData, 0x00, sizeof(gapLeScOobData_t));
        maPeerInformation[peerDeviceId].appState = mAppRunning_c;
        if((pSysParams->system_params).fields.rssi_on_duration != 0)
        {
            BleApp_StartRssiSensing();
        }
        TRACE_INFO("Pairing successful.");
        /* Update UI */
        Led1On();
        /* Send Time Sync */
        (void)CCC_SendTimeSync(peerDeviceId, &devEvtCnt, &mTsUwbDeviceTime, 1U);
#ifdef BMW_KEYFOB_EVK_BOARD
    /* No functions required */
#else
        TRACE_DEBUG("Temperature measured : %d°C",Get_Ms_Temp_Value());
#endif /* BMW_KEYFOB_EVK_BOARD */
        TRACE_DEBUG("Battery level : %d%%",SENSORS_GetBatteryLevel());
#if defined(gAppUseBonding_d) && (gAppUseBonding_d)
        /* Write data in NVM */
        (void)Gap_SaveCustomPeerInformation(peerDeviceId,
                                            (void *)&maPeerInformation[peerDeviceId].customInfo, 0,
                                            (uint16_t)sizeof(appCustomInfo_t));
#endif
        /* Update connection interval to CCC recommended value */
        (void)Gap_UpdateConnectionParameters(peerDeviceId,
                                             gConnectionParamConversionMsToGapUnit((pSysParams->system_params).fields.connection_interval),
                                             gConnectionParamConversionMsToGapUnit((pSysParams->system_params).fields.connection_interval),
                                             gConnReqParams.connLatency,
                                             gConnReqParams.supervisionTimeout,
                                             gConnReqParams.connEventLengthMin,
                                             gConnReqParams.connEventLengthMax);
    }
    else
    {
        /* For MISRA compliance */
    }
}

/*! *********************************************************************************
* \brief        Handles L2capPsmControl events.
*
* \param[in]    pEventData    pointer to appEventData_t.
********************************************************************************** */
static void App_HandleL2capPsmControlCallback(appEventData_t *pEventData)
{
    switch(pEventData->appEvent)
    {
        case mAppEvt_L2capPsmControlCallback_LePsmConnectionComplete_c:
        {
            l2caLeCbConnectionComplete_t *pConnComplete = pEventData->eventData.pData;

            if (pConnComplete->result == gSuccessful_c)
            {
                /* Handle Conn Complete */
                TRACE_INFO("L2CAP PSM Connection Complete.");

                maPeerInformation[pConnComplete->deviceId].customInfo.psmChannelId = pConnComplete->cId;
                /* Move to Time Sync */
                BleApp_StateMachineHandler(maPeerInformation[pConnComplete->deviceId].deviceId, mAppEvt_PsmChannelCreated_c);
            }
            break;
        }

        case mAppEvt_L2capPsmControlCallback_LePsmDisconnectNotification_c:
        {
            TRACE_INFO("L2CAP PSM disconnected. Reconnecting...");
            (void)L2ca_ConnectLePsm((uint16_t)Utils_BeExtractTwoByteValue(maCharacteristics[mcCharVehiclePsmIndex_c].value.paValue),
                                        pEventData->eventData.peerDeviceId, mAppLeCbInitialCredits_c);
            break;
        }

        case mAppEvt_L2capPsmControlCallback_NoPeerCredits_c:
        {
            l2caLeCbNoPeerCredits_t *pCbNoPeerCredits = pEventData->eventData.pData;
            
            (void)L2ca_SendLeCredit (pCbNoPeerCredits->deviceId,
                               pCbNoPeerCredits->cId,
                               mAppLeCbInitialCredits_c);
            break;
        }

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles keyboard events.
*
* \param[in]    pEventData    pointer to appEventData_t.
********************************************************************************** */
static void App_HandleKeys(appEventData_t *pEventData)
{
    switch(pEventData->appEvent)
    {
        case mAppEvt_KBD_EventPressPB1_c:
        {
            if (mGapRole == gGapCentral_c)
            {
                if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppRunning_c)
                {
                    TRACE_INFO("Locking ...");
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.functionId = gCentralLocking_c;
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.actionId = gLock_c;
                    CCC_SendRKERequestSubEvent(pEventData->eventData.peerDeviceId, gCentralLocking_c, gLock_c);
                }
                else
                {
                    KEYFOB_MGR_notify(KEYFOB_EVENT_WALK_DETECTED);
                }
            }
            else
            {
                if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppIdle_c)
                {
                    /* Start advertising */
                    BleApp_Start(mGapRole, 0, 0);
                }
                else
                {
                    /* No action required */
                }
            }
            break;
        }
        
        case mAppEvt_KBD_EventLongPB1_c:
        {
            maPeerInformation[pEventData->eventData.peerDeviceId].appState = mAppGracefulReboot_c;
            BleApp_Disconnect();
            break;
        }
        

        case mAppEvt_KBD_EventVeryLongPB1_c:
        {
            break;
        }

        case mAppEvt_KBD_EventPressPB2_c:
        {
            if (mGapRole == gGapCentral_c)
            {
                if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppRunning_c)
                {
                    TRACE_INFO("Unlocking ...");
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.functionId = gCentralLocking_c;
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.actionId = gUnlock_c;
                    CCC_SendRKERequestSubEvent(pEventData->eventData.peerDeviceId, gCentralLocking_c, gUnlock_c);
                }
                else
                {
                    KEYFOB_MGR_notify(KEYFOB_EVENT_BUTTON_ACTIVATED);
                }
            }
            else
            {
            /* no action required */
            }
            break;
        }

        case mAppEvt_KBD_EventLongPB2_c:
        {
            BleApp_SwitchGapRole(pEventData);
            break;
        }

        case mAppEvt_KBD_EventPressPB3_c:
        {
            if (mGapRole == gGapCentral_c)
            {
                if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppRunning_c)
                {
                    TRACE_INFO("Releasing ...");
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.functionId = gManualTrunkControl_c;
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.actionId = gRelease_c;
                    CCC_SendRKERequestSubEvent(pEventData->eventData.peerDeviceId, gManualTrunkControl_c, gRelease_c);
                }
                else
                {
                    KEYFOB_MGR_notify(KEYFOB_EVENT_BUTTON_ACTIVATED);
                }
            }
            else
            {
                /* no action required */
            }
            break;
        }

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles RKE commands.
*
* \param[in]    pEventData    pointer to appEventData_t.
********************************************************************************** */
static void App_HandleRKECommands(appEventData_t *pEventData)
{
    switch(pEventData->appEvent)
    {
        case mAppEvt_Shell_RKELock_Command_c:
        {
            if (mGapRole == gGapCentral_c)
            {
                if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppRunning_c)
                {
                    TRACE_INFO("Locking ...");
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.functionId = gCentralLocking_c;
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.actionId = gLock_c;
                    CCC_SendRKERequestSubEvent(pEventData->eventData.peerDeviceId, gCentralLocking_c, gLock_c);
                }
                else
                {
                    /* No action required */
                }
            }
            else
            {
                /* No action required */
            }
            break;
        }
        
        case mAppEvt_Shell_RKEUnlock_Command_c:
        {
            if (mGapRole == gGapCentral_c)
            {
                if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppRunning_c)
                {
                    TRACE_INFO("Unlocking ...");
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.functionId = gCentralLocking_c;
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.actionId = gUnlock_c;
                    CCC_SendRKERequestSubEvent(pEventData->eventData.peerDeviceId, gCentralLocking_c, gUnlock_c);
                }
                else
                {
                    /* No action required */
                }
            }
            else
            {
                /* No action required */
            }
            break;
        }
        

        case mAppEvt_Shell_RKERelease_Command_c:
        {
            if (mGapRole == gGapCentral_c)
            {
                if (maPeerInformation[pEventData->eventData.peerDeviceId].appState == mAppRunning_c)
                {
                    TRACE_INFO("Releasing ...");
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.functionId = gManualTrunkControl_c;
                    maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.actionId = gRelease_c;
                    CCC_SendRKERequestSubEvent(pEventData->eventData.peerDeviceId, gManualTrunkControl_c, gRelease_c);
                }
                else
                {
                    /* No action required */
                }
            }
            else
            {
                /* No action required */
            }
            break;
        }

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles GattClientCallback events.
*
* \param[in]    pEventData    pointer to appEventData_t.
********************************************************************************** */
static void App_HandleGattClientCallback(appEventData_t *pEventData)
{
    switch(pEventData->appEvent)
    {

        case mAppEvt_GattClientCallback_GattProcError_c:
        {
            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_GattProcError_c);
            break;
        }

        case mAppEvt_GattClientCallback_GattProcReadCharacteristicValue_c:
        {
            if (mCurrentCharReadingIndex == mcCharVehiclePsmIndex_c)
            {
                /* The SPSM value shall use big-endian byte order so reverse it when received */
                maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.lePsmValue = (uint16_t)Utils_BeExtractTwoByteValue(mValVehiclePsm);
                /* Register DK L2CAP PSM */
                (void)L2ca_RegisterLePsm(maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.lePsmValue, gDKMessageMaxLength_c);
                BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_ReadCharacteristicValueComplete_c);

                /* Read next char */
                mCurrentCharReadingIndex = mcCharVehicleAntennaIdIndex_c;
                (void)GattClient_ReadCharacteristicValue(pEventData->eventData.peerDeviceId, &maCharacteristics[mcCharVehicleAntennaIdIndex_c], mcCharVehicleAntennaIdLength_c);
            }
            else if (mCurrentCharReadingIndex == mcCharVehicleAntennaIdIndex_c)
            {
                /* Read next char */
                mCurrentCharReadingIndex = mcCharTxPowerLevelIndex_c;
                (void)GattClient_ReadCharacteristicValue(pEventData->eventData.peerDeviceId, &maCharacteristics[mcCharTxPowerLevelIndex_c], mcCharTxPowerLevelLength_c);
            }
            else
            {
                /* All chars read - reset index */
                mCurrentCharReadingIndex = mcCharVehiclePsmIndex_c;
            }
            break;
        }

        case mAppEvt_GattClientCallback_GattProcReadUsingCharacteristicUuid_c:
        {
            gattHandleRange_t handleRange;
            bleUuid_t charUuid;
            handleRange.startHandle = 0x0001U;
            handleRange.endHandle = 0xFFFFU;

            if (mCurrentCharReadingIndex == mcCharVehiclePsmIndex_c)
            {
                maCharacteristics[mcCharVehiclePsmIndex_c].value.paValue = mValVehiclePsm;
                /* length 1 octet, handle 2 octets, value(psm) 2 octets */
                maCharacteristics[mcCharVehiclePsmIndex_c].value.handle = (((uint16_t)maOutCharReadBuffer[2]) << 8) | (uint16_t)maOutCharReadBuffer[1];
                maCharacteristics[mcCharVehiclePsmIndex_c].value.paValue[0] = maOutCharReadBuffer[3];
                maCharacteristics[mcCharVehiclePsmIndex_c].value.paValue[1] = maOutCharReadBuffer[4];
                maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.lePsmValue = (uint16_t)Utils_BeExtractTwoByteValue(mValVehiclePsm);
                /* Register DK L2CAP PSM */
                (void)L2ca_RegisterLePsm(maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.lePsmValue, gDKMessageMaxLength_c);
                BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_ReadCharacteristicValueComplete_c);

                /* Read next char if present */
                if (maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.hAntennaIdChar != 0x0U)
                {
                    mCurrentCharReadingIndex = mcCharVehicleAntennaIdIndex_c;
                    FLib_MemCpy(charUuid.uuid128, uuid_char_antenna_id, gcBleLongUuidSize_c);

                    (void)GattClient_ReadUsingCharacteristicUuid(pEventData->eventData.peerDeviceId,
                                                                 gBleUuidType128_c,
                                                                 &charUuid,
                                                                 &handleRange,
                                                                 maOutCharReadBuffer,
                                                                 mCharReadBufferLength_c,
                                                                 &mOutCharReadByteCount);
                }
                else
                {
                    /* Read next char if present */
                    if (maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.hTxPowerChar != 0x0U)
                    {
                        mCurrentCharReadingIndex = mcCharTxPowerLevelIndex_c;
                        charUuid.uuid16 = (uint16_t)gBleSig_TxPower_d;

                        (void)GattClient_ReadUsingCharacteristicUuid(pEventData->eventData.peerDeviceId,
                                                                     gBleUuidType16_c,
                                                                     &charUuid,
                                                                     &handleRange,
                                                                     maOutCharReadBuffer,
                                                                     mCharReadBufferLength_c,
                                                                     &mOutCharReadByteCount);
                    }
                }
            }
            else if (mCurrentCharReadingIndex == mcCharVehicleAntennaIdIndex_c)
            {
                maCharacteristics[mcCharVehicleAntennaIdIndex_c].value.paValue = (uint8_t *)&mValVehicleAntennaId;
                /* length 1 octet, handle 2 octets, value 2 octets */
                maCharacteristics[mcCharVehicleAntennaIdIndex_c].value.handle = (((uint16_t)maOutCharReadBuffer[2]) << 8) | (uint16_t)maOutCharReadBuffer[1];
                maCharacteristics[mcCharVehicleAntennaIdIndex_c].value.paValue[0] = maOutCharReadBuffer[3];
                maCharacteristics[mcCharVehicleAntennaIdIndex_c].value.paValue[1] = maOutCharReadBuffer[4];

                /* Read next char if present */
                if (maPeerInformation[pEventData->eventData.peerDeviceId].customInfo.hTxPowerChar != 0x0U)
                {
                    mCurrentCharReadingIndex = mcCharTxPowerLevelIndex_c;
                    charUuid.uuid16 = (uint16_t)gBleSig_TxPower_d;

                    (void)GattClient_ReadUsingCharacteristicUuid(pEventData->eventData.peerDeviceId,
                                                                 gBleUuidType16_c,
                                                                 &charUuid,
                                                                 &handleRange,
                                                                 maOutCharReadBuffer,
                                                                 mCharReadBufferLength_c,
                                                                 &mOutCharReadByteCount);
                }
            }
            else
            {
                maCharacteristics[mcCharTxPowerLevelIndex_c].value.paValue = (uint8_t *)&mValTxPower;
                /* length 1 octet, handle 2 octets, value 1 octet */
                maCharacteristics[mcCharTxPowerLevelIndex_c].value.handle = (((uint16_t)maOutCharReadBuffer[2]) << 8) | (uint16_t)maOutCharReadBuffer[1];
                maCharacteristics[mcCharTxPowerLevelIndex_c].value.paValue[0] = maOutCharReadBuffer[3];

                /* All chars read - reset index */
                mCurrentCharReadingIndex = mcCharVehiclePsmIndex_c;
            }
            break;
        }

        case mAppEvt_GattClientCallback_GattProcComplete_c:
        {
            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_GattProcComplete_c);
            break;
        }

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles GenericCallback events.
*
* \param[in]    pEventData    pointer to appEventData_t.
********************************************************************************** */
static void App_HandleGenericCallback(appEventData_t *pEventData)
{
    switch(pEventData->appEvent)
    {

        case mAppEvt_GenericCallback_PeerDisconnected_c:
        {
            /* Factory reset may trigger internal error in some scenarios. */
            BleApp_StateMachineHandler( mCurrentPeerId, mAppEvt_PeerDisconnected_c );
            break;
        }
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
        case mAppEvt_GenericCallback_LePhyEvent_c:
        {
            gapPhyEvent_t *pPhyEvent = (gapPhyEvent_t *)pEventData->eventData.pData;
            if(pPhyEvent->phyEventType == gPhyUpdateComplete_c )
            {
                AppPrintLePhyEvent(pPhyEvent);
            }

            pPhyEvent = NULL;
            break;
        }
#endif

        case mAppEvt_GenericCallback_LeScLocalOobData_c:
        {
            if (mCurrentPeerId != gInvalidDeviceId_c)
            {
                FLib_MemCpy(&maPeerInformation[mCurrentPeerId].oobData, pEventData->eventData.pData, sizeof(gapLeScOobData_t));
                BleApp_StateMachineHandler(mCurrentPeerId, mAppEvt_PairingLocalOobData_c);
            }
            break;
        }

        case mAppEvt_GenericCallback_RandomAddressReady_c:
        {
            FLib_MemCpy(gaAppOwnDiscAddress, pEventData->eventData.pData, gcBleDeviceAddressSize_c);
            break;
        }

        case mAppEvt_GenericCallback_CtrlNotifEvent_c:
        {
            bleNotificationEvent_t *pCtrlNotifEvent = (bleNotificationEvent_t *)pEventData->eventData.pData;
            
            if ((pCtrlNotifEvent->eventType & ((uint16_t)gNotifConnCreated_c | (uint16_t)gNotifPhyUpdateInd_c)) > 0U )
            {
                /* Compute event timestamp. */
                uint64_t bleTime = TM_GetTimestamp();
                /* Get current timestamp */
                mTsUwbDeviceTime = GetUwbClock();
                /* Subtract event delay */

                if (bleTime >= pCtrlNotifEvent->timestamp)
                {
                    mTsUwbDeviceTime = mTsUwbDeviceTime - (bleTime - (uint64_t)pCtrlNotifEvent->timestamp);
                }
                else
                {
                    mTsUwbDeviceTime = mTsUwbDeviceTime - ((0x00000000FFFFFFFFU - (uint64_t)pCtrlNotifEvent->timestamp) + bleTime);
                }

                if ((pCtrlNotifEvent->eventType & (uint16_t)gNotifPhyUpdateInd_c) > 0U)
                {
                    /* Send Time Sync. */
                    uint64_t devEvtCnt = 0xFFFFFFFFFFFFFFFFU;
                    
                    if (maPeerInformation[pCtrlNotifEvent->deviceId].appState == mAppRunning_c)
                    {
                        (void)CCC_SendTimeSync(pCtrlNotifEvent->deviceId, &devEvtCnt, &mTsUwbDeviceTime, 1U);
                    }
                }

            }
            break;
        }
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
        case mAppEvt_GenericCallback_BondCreatedEvent_c:
        {
            bleBondCreatedEvent_t *pBondEventData = (bleBondCreatedEvent_t *)pEventData->eventData.pData;
            bleResult_t status;
            
            status = Gap_LoadKeys(pBondEventData->nvmIndex,
                                  &gAppOutKeys, &gAppOutKeyFlags, &gAppOutLeSc,
                                  &gAppOutAuth);
            
            if ( status == gBleSuccess_c)
            {
                /* address type, address, ltk, irk */
                TRACE_INFO("BondingData printed below");
                TRACE_HEX("Address type", (uint8_t*)&gAppOutKeys.addressType, 1);
                TRACE_HEX("Address", gAppOutKeys.aAddress, 6);
                TRACE_HEX("LTK", (uint8_t*)gAppOutKeys.aLtk, 16);
                TRACE_HEX("IRK", (uint8_t*)gAppOutKeys.aIrk, 16);
            }
            
            if (mBondAddedFromShell == TRUE)
            {
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d > 0))
                shell_cmd_finished();
#endif
                mBondAddedFromShell = FALSE;
                gPrivacyStateChangedByUser = TRUE;
                (void)BleConnManager_DisablePrivacy();
            }
            break;
        }
#endif
        
        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles ConnectionCallback events.
*
* \param[in]    pEventData    pointer to appEventData_t.
********************************************************************************** */
static void App_HandleConnectionCallback(appEventData_t *pEventData)
{
    appIntent_t intent;

    switch(pEventData->appEvent)
    {
        case mAppEvt_ConnectionCallback_ConnEvtConnected_c:
        {
        	appConnectionCallbackEventData_t *pConnectedEventData = (appConnectionCallbackEventData_t *)pEventData->eventData.pData;
            /* Check MISRA directive 4.14 */
            if(pConnectedEventData->peerDeviceId >= gAppMaxConnections_c)
            {
                 /* peerDeviceId bigger then gAppMaxConnections_c */ 
                 panic(0, (uint32_t)App_HandleConnectionCallback, 0, 0);
            }
            
            /* Save address used during discovery if controller privacy was used. */
            if (pConnectedEventData->pConnectedEvent.localRpaUsed)
            {
                FLib_MemCpy(gaAppOwnDiscAddress, pConnectedEventData->pConnectedEvent.localRpa, gcBleDeviceAddressSize_c);
            }

            mCurrentPeerId = pConnectedEventData->peerDeviceId;
            if(mGapRole == gGapPeripheral_c)
            {
                Led2Off();
                Led1On();
                //mCurrentPeerId = pConnectedEventData->peerDeviceId;
                maPeerInformation[mCurrentPeerId].appState = mAppRunning_c;
            }

            /* Advertising stops when connected */
            mAdvState.advOn = FALSE;

            /* Subscribe client*/
            (void)Bks_Subscribe(pConnectedEventData->peerDeviceId);
            (void)Bps_Subscribe(pConnectedEventData->peerDeviceId);
            (void)Mss_Subscribe(pConnectedEventData->peerDeviceId);
            (void)Ups_Subscribe(pConnectedEventData->peerDeviceId);
            (void)Cs_Subscribe(pConnectedEventData->peerDeviceId);

            TRACE_INFO("Connected as %s.", (mGapRole == gGapCentral_c)?"central":"peripheral");
            PWR_DisallowDeviceToSleep();

            maPeerInformation[pConnectedEventData->peerDeviceId].gapRole = mGapRole;
            maPeerInformation[pConnectedEventData->peerDeviceId].deviceId = pConnectedEventData->peerDeviceId;
            maPeerInformation[pConnectedEventData->peerDeviceId].isBonded = FALSE;

            /* Set low power mode */
#if defined(cPWR_UsePowerDownMode) && (cPWR_UsePowerDownMode)
            (void)PWR_ChangeDeepSleepMode(gAppDeepSleepMode_c);
            PWR_AllowDeviceToSleep();
#endif

#if defined(gAppUseBonding_d) && (gAppUseBonding_d)
            (void)Gap_CheckIfBonded(pConnectedEventData->peerDeviceId, &maPeerInformation[pConnectedEventData->peerDeviceId].isBonded, NULL);
            if (maPeerInformation[pConnectedEventData->peerDeviceId].isBonded) /*&&
                (gBleSuccess_c == Gap_LoadCustomPeerInformation(pConnectedEventData->peerDeviceId,
                (void*)&maPeerInformation[pConnectedEventData->peerDeviceId].customInfo, 0, (uint16_t)sizeof (appCustomInfo_t))))*/
            {
                mRestoringBondedLink = TRUE;
                volatile bleResult_t result1 = gBleSuccess_c;
                /* Restored custom connection information. Encrypt link */
                result1 = Gap_EncryptLink(pConnectedEventData->peerDeviceId);
            }
#endif
            BleApp_StateMachineHandler(pConnectedEventData->peerDeviceId, mAppEvt_PeerConnected_c);
            break;
        }

        case mAppEvt_ConnectionCallback_ConnEvtDisconnected_c:
        {
        	TM_Close(logTmrId);
            /* Reset Service Discovery to be sure*/
            BleServDisc_Stop(pEventData->eventData.peerDeviceId);
            mCurrentPeerId = gInvalidDeviceId_c;

            TRACE_INFO("Disconnected!");

            /* Turn off the connection LED */
            Led1Off();

            /* Unsubscribe client */
            (void)Bks_Unsubscribe();
            (void)Bps_Unsubscribe();
            (void)Mss_Unsubscribe();
            (void)Ups_Unsubscribe();
            (void)Cs_Unsubscribe();

            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_PeerDisconnected_c);
            PWR_AllowDeviceToSleep();
            break;
        }

        case mAppEvt_ConnectionCallback_ConnEvtLeScOobDataRequest_c:
        {
            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_PairingPeerOobDataReq_c);
            break;
        }

        case mAppEvt_ConnectionCallback_ConnEvtPairingComplete_c:
        {
            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_PairingComplete_c);
            break;
        }

        case mAppEvt_ConnectionCallback_ConnEvtEncryptionChanged_c:
        {
            if( mRestoringBondedLink )
            {
                mRestoringBondedLink = FALSE;
                BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_EncryptionChanged_c);
            }
            break;
        }

        case mAppEvt_ConnectionCallback_ConnEvtAuthenticationRejected_c:
        {
            BleApp_StateMachineHandler(pEventData->eventData.peerDeviceId, mAppEvt_AuthenticationRejected_c);
            break;
        }

        case mAppEvt_ConnectionCallback_ReadRssiEvtConnected_c:
        {
            appRssiReadCallbackEventData_t *pReadRssiCallbackEventData = (appRssiReadCallbackEventData_t *)pEventData->eventData.pData;
            systemParameters_t *pSysParams = NULL;
            dkSubEventDeviceRangingIntentType_t eRangingType;
            bool_t bSendIntent = false;

            App_NvmReadSystemParams(&pSysParams);
            mTemperatureCount++;
            mRssiActiveCount++;
            if((mRssiActiveCount >= mcMaxTemperatureCount((pSysParams->system_params).fields.rssi_on_duration * 1000,
               (pSysParams->system_params).fields.connection_interval)))
            {
            	TM_Close(rssiTmrId);
            	mRssiActiveCount = 0;
            }
            if(pReadRssiCallbackEventData->rssi_dBm != gGapRssiNotAvailable_d)
            {
                mRssiSum = mRssiSum + (pReadRssiCallbackEventData->rssi_dBm);
                mRssiCounter++;
            }
            else
            {
                /* For MISRA compliance */
            }

            if(mRssiCounter == mcRssiMaxCounter)
            {
                mNewFilteredRssi = mcCalculateNewFilteredRssi(mRssiSum);
                if(!mValidRssi0)
                {
                    if(mNewFilteredRssi >= (pSysParams->system_params).fields.rssi_intent_high)
                    {
                        mRssi0 = (pSysParams->system_params).fields.rssi_intent_high - mcRssiCorrection;
                        CCC_SendRangingIntentSubEvent(pReadRssiCallbackEventData->peerDeviceId,
                                                      gDeviceRangingIntent_c,
                                                      gHighApproachConfidence_c);
                        intent = App_HighIntent;
                        mSameIntentCount = 0;
                        TRACE_DEBUG("RSSI_0 = %d dBm", mRssi0);
                    }
                    else
                    {
                        mRssi0 = mNewFilteredRssi;
                    }
                    mValidRssi0 = true;
                }
                else
                {
                    intent = BleApp_DecideIntentFromRSSI(mNewFilteredRssi, mRssi0);
                    if(intent != App_Undefined)
                    {
                        /* Check wether we have to send the intent according to intent, mLastIntent and timeout_between_same_intents */
                        if(0 == (pSysParams->system_params).fields.timeout_between_same_intents)
                        {
                            /* timeout_between_same_intents = 0 */
                            bSendIntent = (intent != mLastIntent);
                        }
                        else
                        {
                            /* timeout_between_same_intents != 0 */
                            if(intent != mLastIntent)
                            {
                                bSendIntent = true;
                                mSameIntentCount = 0;
                            }
                            else
                            {
                                if(mSameIntentCount >= mcMaxSameIntentsCount((pSysParams->system_params).fields.timeout_between_same_intents * 1000,
                                                                             (pSysParams->system_params).fields.connection_interval,
                                                                              mcRssiMaxCounter))
                                {
                                    bSendIntent = true;
                                    mSameIntentCount = 0;
                                }
                                else
                                {
                                    bSendIntent = false;
                                    mSameIntentCount++;
                                }
                            }
                        }
                        if(bSendIntent)
                        {
                            switch(intent)
                            {
                            case App_LowIntent:
                                eRangingType = gLowApproachConfidence_c;
                                break;

                            case App_MediumIntent:
                                eRangingType = gMediumApproachConfidence_c;
                                break;

                            case App_HighIntent:
                                eRangingType = gHighApproachConfidence_c;
                                break;

                            default:
                                /* No action required */
                                break;
                            }
                            CCC_SendRangingIntentSubEvent(pReadRssiCallbackEventData->peerDeviceId,
                                                          gDeviceRangingIntent_c,
                                                          eRangingType);
                            TRACE_DEBUG("RSSI_0 = %d dBm", mRssi0);
                            TRACE_DEBUG("RSSI Measured = %d dBm", mNewFilteredRssi);
                        }
                        else
                        {
                            /* For MISRA compliance */
                        }
                    }
                    else
                    {
                        /* For MISRA compliance */
                    }
                }
                mLastIntent = intent;
                mRssiCounter = 0;
                mRssiSum = 0;
            }
            else
            {
                /* For MISRA compliance */
            }
            break;
        }

        default:
        {
            ; /* No action required */
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles L2capPsmDataCallback events.
*
* \param[in]    pEventData    pointer to appEventData_t.
********************************************************************************** */
static void App_HandleL2capPsmDataCallback(appEventData_t *pEventData)
{
   
    appEventL2capPsmData_t *l2capDataEvent = (appEventL2capPsmData_t *)pEventData->eventData.pData;
    deviceId_t deviceId = l2capDataEvent->deviceId;
    uint16_t packetLength = l2capDataEvent->packetLength;
    uint8_t* pPacket = l2capDataEvent->pPacket;
    
    if (packetLength > (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c))
    {
        dkMessageType_t protocol = (dkMessageType_t)pPacket[0];
        rangingMsgId_t msgId = (rangingMsgId_t)pPacket[1];
        uint16_t length = 0;
        FLib_MemCpyReverseOrder(&length, &pPacket[2], gLengthFieldSize_c);

        switch (protocol)
        {
            case gDKMessageTypeFrameworkMessage_c:
            {
                if (msgId == gDkApduRQ_c)
                {
                    if (maPeerInformation[deviceId].appState == mAppCCCPhase2WaitingForRequest_c)
                    {
                        TRACE_INFO("SPAKE Request received.");
                        bleResult_t result = CCCPhase2_SendSPAKEResponse(deviceId, &pPacket[4], length);
                        if (result == gBleSuccess_c)
                        {
                            BleApp_StateMachineHandler(deviceId, mAppEvt_SentSPAKEResponse_c);
                        }
                    }
                    else if (maPeerInformation[deviceId].appState == mAppCCCPhase2WaitingForVerify_c)
                    {
                        TRACE_INFO("SPAKE Verify received.");
                        bleResult_t result = CCCPhase2_SendSPAKEVerify(deviceId, &pPacket[4], length);
                        if (result == gBleSuccess_c)
                        {
                            BleApp_StateMachineHandler(deviceId, mAppEvt_ReceivedSPAKEVerify_c);
                        }
                    }
                    else
                    {
                        /* For MISRA compliance */
                    }
                }
            }
            break;
            
            case gDKMessageTypeSupplementaryServiceMessage_c:
            {
                if ( (msgId == gFirstApproachRQ_c) || (msgId == gFirstApproachRS_c ) )
                {
                    if ( packetLength == (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + gFirstApproachReqRspPayloadLength) )
                    {
                        uint8_t *pData = &pPacket[gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c];
                        
                        /* BD address not used. */
                        pData += gcBleDeviceAddressSize_c;
                        /* Confirm Value */
                        FLib_MemCpy(&maPeerInformation[deviceId].peerOobData.confirmValue, pData, gSmpLeScRandomConfirmValueSize_c);
                        pData += gSmpLeScRandomConfirmValueSize_c;
                        /* Random Value */
                        FLib_MemCpy(&maPeerInformation[deviceId].peerOobData.randomValue, pData, gSmpLeScRandomValueSize_c);
                        
#if defined(mcConnectionwithRealVehicle) && (mcConnectionwithRealVehicle == 1)
                            /* send standard transaction request */
                            TRACE_INFO("Received First_Approach_RS.");
                            CCC_StandardTransactionReq(deviceId);
#else
                            /* Send event to the application state machine. */
                            BleApp_StateMachineHandler(deviceId, mAppEvt_PairingPeerOobDataRcv_c);
#endif

                    }
                    else
                    {
                        TRACE_ERROR("Invalid length for FirstApproach message.");
                    }
                }
                if (msgId == gRKEAuthRQ_c)
                {
                    uint8_t RkeChallengeTab[gRKEChallengeLength_c] = {0};
                    uint8_t hashBuffer[SHA256_HASH_SIZE] = {0};
                    uint8_t attestationData[SHA256_HASH_SIZE] = {0};
                    bool_t dataToBeSigned = FALSE;

                    TRACE_HEX("Received RKE_Auth_RQ", pPacket, packetLength);
                    BleApp_ParsingRKEAuthentication(pPacket, packetLength, RkeChallengeTab, gRKEChallengeLength_c);
                    if((gCentralLocking_c == maPeerInformation[deviceId].customInfo.functionId) && 
                       (gLock_c == maPeerInformation[deviceId].customInfo.actionId))
                    {
                        CCC_DeriveArbitraryData(RkeChallengeTab,
                                                gRKEChallengeLength_c,
                                                gCentralLocking_c,
                                                gLock_c,
                                                hashBuffer,
                                                sizeof(hashBuffer));
                        dataToBeSigned = TRUE;
                    }
                    else if((gCentralLocking_c == maPeerInformation[deviceId].customInfo.functionId) && 
                            (gUnlock_c == maPeerInformation[deviceId].customInfo.actionId))
                    {
                        CCC_DeriveArbitraryData(RkeChallengeTab,
                                                gRKEChallengeLength_c,
                                                gCentralLocking_c,
                                                gUnlock_c,
                                                hashBuffer,
                                                sizeof(hashBuffer));
                        dataToBeSigned = TRUE;
                    }
                    else if((gManualTrunkControl_c == maPeerInformation[deviceId].customInfo.functionId) && 
                            (gRelease_c == maPeerInformation[deviceId].customInfo.actionId))
                    {
                        CCC_DeriveArbitraryData(RkeChallengeTab,
                                                gRKEChallengeLength_c,
                                                gManualTrunkControl_c,
                                                gRelease_c,
                                                hashBuffer,
                                                sizeof(hashBuffer));
                        dataToBeSigned = TRUE;
                    }
                    else
                    {
                        dataToBeSigned = FALSE;
                    }
                    if(dataToBeSigned)
                    {
                        CCC_SignArbitraryData(hashBuffer, sizeof(hashBuffer), attestationData);
                        CCC_SendRKEAuthResponse(deviceId, attestationData, sizeof(attestationData));
                    }
                }
            }
            break;

            case gDKMessageTypeDKEventNotification_c:
            {
                if ( msgId == gDkEventNotification_c )
                {
                    if ( packetLength == (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + gCommandCompleteSubEventPayloadLength_c) )
                    {
                        dkSubEventCategory_t category = (dkSubEventCategory_t)pPacket[gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c];
                        if ( category == gCommandComplete_c)
                        {
                            dkSubEventCommandCompleteType_t type = (dkSubEventCommandCompleteType_t)pPacket[gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + sizeof(category)];
                            switch (type)
                            {
                                case gBlePairingReady_c:
                                {
                                    TRACE_INFO("Received Command Complete SubEvent: BLE_pairing_ready");
                                    BleApp_StateMachineHandler(deviceId, mAppEvt_ReceivedPairingReady_c);
                                }
                                break;
#if defined(mcConnectionwithRealVehicle) && (mcConnectionwithRealVehicle == 1)
                                case gDeselectSE_c:
                                {
                                	uint64_t devEvtCnt = 0U;
                                    TRACE_INFO("Received Command Complete SubEvent: Deselect SE");
                                    //BleApp_StateMachineHandler(deviceId, mAppEvt_PairingPeerOobDataRcv_c);
                                    /* Power_off the SE */
                                    SetSePower(mAppSePoweredOff_c);
                                    (void)CCC_SendTimeSync(deviceId, &devEvtCnt, &mTsUwbDeviceTime, 1U);
                                }
                                break;
#endif

                                default:
                                {
                                    ; /* For MISRA compliance */
                                }
                                break;
                            }
                        }
                    }
                    else if (packetLength == (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + gCommandVehicleStatusChangedSubEventPayloadLength_c))
                    {
                        TRACE_INFO();
                        TRACE_HEX("Received Vehicle Status Changed SubEvent", (uint8_t *)pPacket, packetLength);

                        /* parsing */
                        BleApp_ParsingVehicleStatusChangedSubEvent(pPacket,packetLength);
                    }
                }
            }
            break;
            case gDKMessageTypeUWBRangingServiceMessage_c:
            {
                if(msgId == gRangingSessionRQ_c )
                {
                    TRACE_HEX("Received Ranging Session Request", (uint8_t *)pPacket, packetLength);
                    BleApp_ParsingRangingSessionRequest(pPacket);
                    CCC_SendRangingSessionRS(deviceId);
                    UWB_MGR_notify(UWB_EVENT_START_RANGING);
                    mUWBState = gUWBRanging_c;
                }
                else if(msgId == gRangingRecoveryRQ_c)
                {
                    TRACE_HEX("Received Ranging Recovery Request", (uint8_t *)pPacket, packetLength);
                    BleApp_ParsingRangingRecoveryRequest(pPacket);
                    CCC_SendRangingRecoveryRS(deviceId);
                    UWB_MGR_notify(UWB_EVENT_RECOVER_RANGING);
                    mUWBState = gUWBRanging_c;
                }
                else if(msgId == gRangingSuspendRQ_c)
                {
                    TRACE_HEX("Received Ranging Suspend Request", (uint8_t *)pPacket, packetLength);
                    BleApp_ParsingRangingSuspendRequest(pPacket);
                    CCC_SendRangingSuspendRS(deviceId);
                    UWB_MGR_notify(UWB_EVENT_STOP_RANGING);
                    mUWBState = gUWBNoRanging_c;
                }
                else if(msgId == gRangingCapabilityRQ_c)
                {
                	TRACE_INFO("Ranging Capability Request Received");
                	BleApp_ParsingSendRangingCapabilityReq(pPacket);
                	CCC_SendRangingCapabilityRS(deviceId);
                    KEYFOB_MGR_notify(KEYFOB_EVENT_BLE_CONNECTION_SUCCESS);
                    UWB_MGR_notify(UWB_EVENT_BLE_CONNECTED);
#if defined(mcLog) && (mcLog == 1)
                    BleApp_StartLogTimer();
#endif
                }
                else
                {

                }
            }
            break;

#if defined(mcConnectionwithRealVehicle) && (mcConnectionwithRealVehicle == 1)
            case gDKMessageTypeSEMessage_c:
            {
            	uint8_t result = 1u;
            	uint16_t status;
            	char buf[(packetLength-4)*2+1];
            	apduId_t Id;
                if(msgId == gDkApduRQ_c)
                {
                    if ( packetLength == (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + gSelectReqPayloadLength) )
                    {
                        TRACE_INFO("Select Request received");
                        BleApp_ParsingSelectReq(pPacket);
                        Id = Select_ApduId;

                    }
                    else if ( packetLength == (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + gAuthent0ReqPayloadLength) )
                    {
                        TRACE_INFO("Authent0 Request received");
                        BleApp_ParsingAuthent0Req(pPacket);
                        Id = Auth0_ApduId;
                    }
                    else if ( packetLength == (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + gAuthent1ReqPayloadLength) )
                    {
                        TRACE_INFO("Authent1 Request received");
                        BleApp_ParsingAuthent1Req(pPacket);
                        Id = Auth1_ApduId;
                    }
                    else if (( packetLength == (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + gControlFlowReqPayloadLength) )&&(pPacket[5]==0x3C))
                    {
                        TRACE_INFO("Control Flow Request received");
                        BleApp_ParsingControlFlowReq(pPacket);
                        Id = ControlFlow_ApduId;
                    }
                    else if (( packetLength == (gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c + gCreateRangingKeyReqPayloadLength) )&&(pPacket[5] != 0x3C))
                    {
                    	TRACE_INFO("Create Ranging Key Request received");
                        BleApp_ParsingCreateRangingKeyReq(pPacket);
                        Id = CreateRangingKey_ApduId;
                    }
                    else
                    {
                        /**/
                    }
#ifdef BMW_KEYFOB_EVK_BOARD
    /* No functions required */
#else
                    /* send payload value to secure element */
                    Array_hex_string((uint8_t*)pPacket+4, packetLength-4, buf);
                    result = Custom_APDU(&status, buf, 2*(packetLength-4));
#endif /* BMW_KEYFOB_EVK_BOARD */
                    /* Get response from SE and send it to car anchor */
                    CCC_SendAPDUResp(deviceId, Id);
                }
            }
            break;
#endif
            default:
            break;
        }
    }
}

#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
/*! *********************************************************************************
* \brief    Set bonding data.
*
********************************************************************************** */
static void BleApp_SetBondingData(appEventData_t *pEventData)
{
    /* Set address type, LTK, IRK and pear device address  */
    appBondingData_t *pAppBondingData = (appBondingData_t *)pEventData->eventData.pData;

    bleResult_t status = gBleSuccess_c;
    status = SetBondingData(pAppBondingData->nvmIndex, pAppBondingData->addrType, pAppBondingData->aLtk,
                            pAppBondingData->aIrk, pAppBondingData->deviceAddr);
    if ( status != gBleSuccess_c )
    {
        TRACE_HEX("setbd failed with status", (uint8_t*)&status, 2);
    }
    else
    {
#if defined(gAppUseBonding_d) && (gAppUseBonding_d == 1)
       (void)Gap_AddDeviceToFilterAcceptList(pAppBondingData->addrType, pAppBondingData->deviceAddr);
#endif
        mBondAddedFromShell = TRUE;
    }
}

/*! *********************************************************************************
* \brief    Remove bonding data.
*
********************************************************************************** */
static void BleApp_RemoveBondingData(appEventData_t *pEventData)
{
#if (defined(gAppUseBonding_d) && (gAppUseBonding_d == 1U))
    bleResult_t result = gGapSuccess_c;
    result = Gap_LoadKeys(pEventData->eventData.peerDeviceId, &gAppOutKeys, &gAppOutKeyFlags, &gAppOutLeSc, &gAppOutAuth);
    if(result == gBleSuccess_c)
    {
        if(gBleSuccess_c == Gap_RemoveDeviceFromFilterAcceptList(gAppOutKeys.addressType, gAppOutKeys.aAddress))
        {
            /* Remove bond based on nvm index stored in eventData.peerDeviceId */
            if ((Gap_RemoveBond(pEventData->eventData.peerDeviceId) == gBleSuccess_c))
            {
                gcBondedDevices--;
                gPrivacyStateChangedByUser = TRUE;
                (void)BleConnManager_DisablePrivacy();
                TRACE_INFO("Bond removed!");
            }
            else
            {
                TRACE_ERROR("Operation failed!");
            }
        }
        else
        {
             TRACE_ERROR("Operation failed!");
        }
    }
    else
    {
        TRACE_ERROR("Removed bond failed because unable to load the keys from the bond.");
    }
#endif
}

/*! *********************************************************************************
* \brief    List bonding data.
*
********************************************************************************** */
static void BleApp_ListBondingData(void)
{
    gapIdentityInformation_t aIdentity[gMaxBondedDevices_c];
    uint8_t nrBondedDevices = 0;
    uint8_t foundBondedDevices = 0;
    bleResult_t result = Gap_GetBondedDevicesIdentityInformation(aIdentity, gMaxBondedDevices_c, &nrBondedDevices);
    if (gBleSuccess_c == result && nrBondedDevices > 0U)
    {
        for (uint8_t i = 0; i < (uint8_t)gMaxBondedDevices_c; i++)
        {
            result = Gap_LoadKeys((uint8_t)i, &gAppOutKeys, &gAppOutKeyFlags, &gAppOutLeSc, &gAppOutAuth);
            if (gBleSuccess_c == result && nrBondedDevices > 0U)
            {
                /* address type, address, ltk, irk */
                TRACE_HEX("NVMIndex", (uint8_t*)&i, 1);
                TRACE_HEX("BondingData", (uint8_t*)&gAppOutKeys.addressType, 1);
                TRACE_HEX("Address", (uint8_t*)gAppOutKeys.aAddress, 6);
                TRACE_HEX("LTK", (uint8_t*)gAppOutKeys.aLtk, 16);
                TRACE_HEX("IRK", (uint8_t*)gAppOutKeys.aIrk, 16);
                foundBondedDevices++;
            }
            if(foundBondedDevices == nrBondedDevices)
            {
                TRACE_INFO("");
                break;
            }
        }
    }
}

/*! *********************************************************************************
* \brief    Parsing RKE Request SubEvent Templates.
*
********************************************************************************** */
static void BleApp_ParsingVehicleStatusChangedSubEvent(uint8_t *pPacket, uint16_t packetLength)
{
    TRACE_DEBUG("Message Type : 0x%02x", pPacket[0]);
    TRACE_DEBUG("Message Id : 0x%02x", pPacket[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", pPacket[2], pPacket[3]);
    TRACE_HEX("Payload data", pPacket + 4, packetLength - 4);
    TRACE_DEBUG("Payload Category : 0x%02x", pPacket[4]);
    TRACE_DEBUG("Template : 0x%02x%02x", pPacket[5],pPacket[6]);
    TRACE_DEBUG("FunctionID : 0x%02x%02x", pPacket[10], pPacket[11]);
    TRACE_DEBUG("ActionID : 0x%02x", pPacket[14]);
    TRACE_DEBUG("Status : 0x%02x", pPacket[17]);

    if((pPacket[10] == 0x00)&&(pPacket[11] == 0x01))
    {
        mVehicleState = (pPacket[14] == 0x00)?gStatusLocked_c:gStatusUnlocked_c;
    }
    else
    {
        mVehicleState = gStatusUnlocked_c;
    }
}

/*! *********************************************************************************
* \brief    Parsing RKE_Auth payloads.
*
********************************************************************************** */
static void BleApp_ParsingRKEAuthentication(uint8_t *pPacket, uint16_t packetLength, uintn8_t *pRKEChallenge, uintn8_t RKEChallengeLen)
{
    if((packetLength >= 20) && (pRKEChallenge) && (RKEChallengeLen))
    {
        TRACE_DEBUG("Message Type : 0x%02x", pPacket[0]);
        TRACE_DEBUG("Message Id : 0x%02x", pPacket[1]);
        TRACE_DEBUG("Payload length : 0x%02x%02x", pPacket[2], pPacket[3]);
        RKEChallengeLen = MIN(RKEChallengeLen, packetLength - 4);
        FLib_MemCpy(pRKEChallenge, &pPacket[4], RKEChallengeLen);
        TRACE_HEX("RkeChallenge", pRKEChallenge, RKEChallengeLen);
    }
}

/*! *********************************************************************************
* \brief    Parsing RKE Request SubEvent Templates.
*
********************************************************************************** */
static void BleApp_ParsingRKERequestSubEvent(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t *pPacket)
{
    TRACE_DEBUG("Message Type : 0x%02x", MessageType);
    TRACE_DEBUG("Message Id : 0x%02x", MessageId);
    TRACE_DEBUG("Payload length : 0x%04x", gCommandRKERequestSubEventPayloadLength_c);
    TRACE_HEX("Payload data", (uint8_t *)pPacket, gCommandRKERequestSubEventPayloadLength_c);
    TRACE_DEBUG("Payload Category : 0x%02x", pPacket[0]);
    TRACE_DEBUG("Template : 0x%02x%02x", pPacket[1], pPacket[2]);
    TRACE_DEBUG("FunctionID : 0x%02x%02x", pPacket[6], pPacket[7]);
    TRACE_DEBUG("ActionID : 0x%02x", pPacket[10]);
}

/*! *********************************************************************************
* \brief    Parsing Ranging Intent SubEvent Templates.
*
********************************************************************************** */
static void BleApp_ParsingRangingIntentSubEvent(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", MessageType);
    TRACE_DEBUG("Message Id : 0x%02x", MessageId);
    TRACE_DEBUG("Payload length : 0x%04x", gRangingIntentSubEventPayloadLength_c);
    TRACE_DEBUG("Payload Category : 0x%02x", packet[0]);
    TRACE_DEBUG("Payload Type : 0x%02x", packet[1]);
    if(packet[1] == 0x01)
    {
        TRACE_INFO( "Intent : LOW");
    }
    else if(packet[1] == 0x02)
    {
        TRACE_INFO( "Intent : MEDIUM");
    }
    else
    {
        TRACE_INFO( "Intent : HIGH");
    }
}

/*! *********************************************************************************
* \brief    Parsing Ranging Session Response message.
*
********************************************************************************** */
static void BleApp_ParsingRangingSessionResponse(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", MessageType);
    TRACE_DEBUG("Message Id : 0x%02x", MessageId);
    TRACE_DEBUG("Payload length : 0x%04x", gRangingSessionResponsePayloadLength_c);
    TRACE_DEBUG("RAN_Multiplier : 0x%02x", packet[0]);
    TRACE_DEBUG("Slot_BitMask : 0x%02x", packet[1]);
    TRACE_DEBUG("SYNC_Code_Index_BitMask : 0x%02x%02x%02x%02x", packet[2], packet[3], packet[4], packet[5]);
    TRACE_DEBUG("Selected_UWB_Channel : 0x%02x", packet[6]);
    TRACE_DEBUG("Hopping_Config_Bitmask : 0x%02x", packet[7]);
}

/*! *********************************************************************************
* \brief    Parsing Ranging Recovery Response message.
*
********************************************************************************** */
static void BleApp_ParsingRangingRecoveryResponse(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", MessageType);
    TRACE_DEBUG("Message Id : 0x%02x", MessageId);
    TRACE_DEBUG("Payload length : 0x%04x", gRangingRecoveryResponsePayloadLength_c);
    TRACE_DEBUG("STS_Index0 : 0x%02x%02x%02x%02x", packet[0], packet[1], packet[2], packet[3]);
    TRACE_DEBUG("UWB_Time0 : 0x%02x%02x%02x%02x%02x%02x%02x%02x", packet[4], packet[5], packet[6], packet[7], packet[8], packet[9], packet[10], packet[11]);
}

/*! *********************************************************************************
* \brief    Parsing Ranging suspend Response message.
*
********************************************************************************** */
static void BleApp_ParsingRangingSuspendResponse(dkMessageType_t MessageType, rangingMsgId_t MessageId, uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", MessageType);
    TRACE_DEBUG("Message Id : 0x%02x", MessageId);
    TRACE_DEBUG("Payload length : 0x%04x", gRangingSuspendResponsePayloadLength_c);
    TRACE_DEBUG("Suspend_Response : 0x%02x", packet[0]);
}

/*! *********************************************************************************
* \brief    Parsing Ranging Session Request message.
*
********************************************************************************** */
static void BleApp_ParsingRangingSessionRequest(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_DEBUG("Selected_DK_Protocol_Version : 0x%02x%02x", packet[4], packet[5]);
    TRACE_DEBUG("Selected_UWB_Config_id : 0x%02x%02x", packet[6], packet[7]);
    TRACE_DEBUG("UWB_Session_Id : 0x%02x%02x%02x%02x", packet[8], packet[9], packet[10], packet[11]);
    TRACE_DEBUG("Selected_PulseShape_Combo : 0x%02x",  packet[12]);
    TRACE_DEBUG("Channel_Bitmask : 0x%02x",  packet[13]);
}

/*! *********************************************************************************
* \brief    Parsing Ranging Recovery Request message.
*
********************************************************************************** */
static void BleApp_ParsingRangingRecoveryRequest(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_DEBUG("UWB_Session_Id : 0x%02x%02x%02x%02x", packet[4], packet[5], packet[6], packet[7]);
}

/*! *********************************************************************************
* \brief    Parsing Ranging Suspend Request message.
*
********************************************************************************** */
static void BleApp_ParsingRangingSuspendRequest(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_DEBUG("UWB_Session_Id : 0x%02x%02x%02x%02x", packet[4], packet[5], packet[6], packet[7]);
}


/*! *********************************************************************************
* \brief    Parsing Select Request message.
*
********************************************************************************** */
static void BleApp_ParsingSelectReq(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_HEX("Payload ", (uint8_t *)packet + gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c, gSelectReqPayloadLength);
}

/*! *********************************************************************************
* \brief    Parsing Authent0 Request message.
*
********************************************************************************** */
static void BleApp_ParsingAuthent0Req(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_HEX("Payload ", (uint8_t *)packet + gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c, gAuthent0ReqPayloadLength);
}

/*! *********************************************************************************
* \brief    Parsing Authent1 Request message.
*
********************************************************************************** */
static void BleApp_ParsingAuthent1Req(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_HEX("Payload ", (uint8_t *)packet + gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c, gAuthent1ReqPayloadLength);
}

/*! *********************************************************************************
* \brief    Parsing Control Flow Request message.
*
********************************************************************************** */
static void BleApp_ParsingControlFlowReq(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_HEX("Payload ", (uint8_t *)packet + gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c, gControlFlowReqPayloadLength);
}

/*! *********************************************************************************
* \brief    Parsing Create Ranging Key Request message.
*
********************************************************************************** */
static void BleApp_ParsingCreateRangingKeyReq(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_HEX("Payload ", (uint8_t *)packet + gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c, gCreateRangingKeyReqPayloadLength);
}

/*! *********************************************************************************
* \brief    Parsing Ranging Capability Request message.
*
********************************************************************************** */
static void BleApp_ParsingSendRangingCapabilityReq(uint8_t * packet)
{
    TRACE_DEBUG("Message Type : 0x%02x", packet[0]);
    TRACE_DEBUG("Message Id : 0x%02x",  packet[1]);
    TRACE_DEBUG("Payload length : 0x%02x%02x", packet[2], packet[3]);
    TRACE_HEX("Payload ", (uint8_t *)packet + gMessageHeaderSize_c + gPayloadHeaderSize_c + gLengthFieldSize_c, gRangingCapabilityRequestPayloadLength_c);
}

/*! *********************************************************************************
* \brief    Set Bonding Data on the BLE application.
*
********************************************************************************** */
static bleResult_t SetBondingData(uint8_t nvmIndex, bleAddressType_t addressType,
                                  uint8_t* ltk, uint8_t* irk, uint8_t* address)
{
    bleResult_t status;
    
    gAppOutKeys.addressType = addressType;
    FLib_MemCpy(gAppOutKeys.aAddress, address, gcBleDeviceAddressSize_c);
    FLib_MemCpy(gAppOutKeys.aLtk, ltk, gcSmpMaxLtkSize_c);
    FLib_MemCpy(gAppOutKeys.aIrk, irk, gcSmpIrkSize_c);

    status = Gap_SaveKeys(nvmIndex, &gAppOutKeys, TRUE, FALSE);
    
    return status;
}

/*! *********************************************************************************
* \brief    Disconnect received from the BLE application.
*
********************************************************************************** */
static void BleApp_Disconnect(void)
{
    uint8_t peerId;
    
    for (peerId = 0; peerId < (uint8_t)gAppMaxConnections_c; peerId++)
    {
        if (maPeerInformation[peerId].deviceId != gInvalidDeviceId_c)
        {
            (void)Gap_Disconnect(maPeerInformation[peerId].deviceId);
        }
    }
}

/*! *********************************************************************************
* \brief        Prints phy event.
*
********************************************************************************** */
static void AppPrintLePhyEvent(gapPhyEvent_t* pPhyEvent)
{
    PrintLePhyEvent(pPhyEvent);
}

/*! *********************************************************************************
* \brief        Prints phy event.
*
********************************************************************************** */
static void PrintLePhyEvent(gapPhyEvent_t* pPhyEvent)
{
    /* String dictionary corresponding to gapLePhyMode_t */
    static const char* mLePhyModeStrings[] =
    {
        "Invalid",
        "1M",
        "2M",
        "Coded",
    };    

    uint8_t txPhy = ((gapLePhyMode_tag)(pPhyEvent->txPhy) <= gLePhyCoded_c) ? pPhyEvent->txPhy : 0U;
    uint8_t rxPhy = ((gapLePhyMode_tag)(pPhyEvent->rxPhy) <= gLePhyCoded_c) ? pPhyEvent->rxPhy : 0U;
    TRACE_INFO("Phy Update Complete.");
    TRACE_INFO("TxPhy ");
    TRACE_DEBUG("%s", mLePhyModeStrings[txPhy]);
    TRACE_INFO("RxPhy ");
    TRACE_DEBUG("%s", mLePhyModeStrings[rxPhy]);
}
#endif

/*! *********************************************************************************
 * \brief        Owner Pairing Certificate Exchange - step 1, SPAKE2+ Response Command
 *
 ********************************************************************************** */
static bleResult_t CCCPhase2_SendSPAKEResponse(deviceId_t deviceId, uint8_t *pData, uint16_t dataLen)
{
    bleResult_t result = gBleSuccess_c;

    /* Dummy data - replace with calls to get actual data */
    uint16_t payloadLen = gDummyPayloadLength_c;
    uint8_t payload[gDummyPayloadLength_c] = gDummyPayload_c;

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            gDKMessageTypeFrameworkMessage_c,
                            gDkApduRS_c,
                            payloadLen,
                            payload);
    TRACE_INFO("SPAKE Response sent.");
    return result;
}

/*! *********************************************************************************
 * \brief        Owner Pairing Certificate Exchange - step 2, SPAKE2+ Verify Command
 *
 ********************************************************************************** */
static bleResult_t CCCPhase2_SendSPAKEVerify(deviceId_t deviceId, uint8_t *pData, uint16_t dataLen)
{
    bleResult_t result = gBleSuccess_c;

    /* Dummy data - replace with calls to get actual data */
    uint16_t payloadLen = gDummyPayloadLength_c;
    uint8_t payload[gDummyPayloadLength_c] = gDummyPayload_c;

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            gDKMessageTypeFrameworkMessage_c,
                            gDkApduRS_c,
                            payloadLen,
                            payload);
    TRACE_INFO("SPAKE Verify sent.");
    return result;
}

/*! *********************************************************************************
 * \brief        Sends DK Command Complete SubEvents to Car Anchor.
 *
 ********************************************************************************** */
static bleResult_t CCC_SendCommandCompleteSubEvent(deviceId_t deviceId,
                                                   dkSubEventCategory_t category,
                                                   dkSubEventCommandCompleteType_t type)
{
    bleResult_t result = gBleSuccess_c;

    uint8_t payload[gCommandCompleteSubEventPayloadLength_c] = {0}; /* SubEvent Category + SubEvent Type */
    payload[0] = (uint8_t)category;
    payload[1] = (uint8_t)type;

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            gDKMessageTypeDKEventNotification_c,
                            gDkEventNotification_c,
                            gCommandCompleteSubEventPayloadLength_c,
                            payload);

    return result;
}

/*! *********************************************************************************
 * \brief        Sends Ranging Intent SubEvents to Car Anchor.
 *
 ********************************************************************************** */
static bleResult_t CCC_SendRangingIntentSubEvent(deviceId_t deviceId,
                                                 dkSubEventCategory_t category,
                                                 dkSubEventDeviceRangingIntentType_t type)
{
    bleResult_t result = gBleSuccess_c;

    uint8_t payload[gRangingIntentSubEventPayloadLength_c] = {0}; /* SubEvent Category + SubEvent Type */
    dkMessageType_t MessageType = gDKMessageTypeDKEventNotification_c;
    rangingMsgId_t MessageId = gDkEventNotification_c;
    payload[0] = (uint8_t)category;
    payload[1] = (uint8_t)type;

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            MessageType,
                            MessageId,
                            gRangingIntentSubEventPayloadLength_c,
                            payload);

    TRACE_INFO("Device Ranging Intent SubEvent sent");
    /* Parsing */
    BleApp_ParsingRangingIntentSubEvent(MessageType, MessageId, payload);
    return result;
}

/*! *********************************************************************************
 * \brief        Sends DK RKE Request SubEvent to Car Anchor.
 *
 ********************************************************************************** */
static bleResult_t CCC_SendRKERequestSubEvent(deviceId_t deviceId, gDkRKEFunctionId_t function, uint8_t action)
{
    bleResult_t result;
    bool Sanity_check = false;
    uint8_t payload[gCommandRKERequestSubEventPayloadLength_c] = {0};
    uint8_t len = 0;
    dkMessageType_t MessageType = gDKMessageTypeDKEventNotification_c;
    rangingMsgId_t MessageId = gDkEventNotification_c;
    switch(function)
    {
        /* Lock or unlock door */
        case gCentralLocking_c :
          {
              if ((action == gLock_c) || (action == gUnlock_c))
              {
                  Sanity_check = true;
              }
              else
              {
                  ;/* No action required*/
              }
          }
          break;
        /* Release trunk */
        case gManualTrunkControl_c :
          {
              if (action == gRelease_c)
              {
                  Sanity_check = true;
              }
              else
              {
                  ;/* No action required*/
              }
          }
          break;
        default :
          {
              ; /* No action required*/
          }
          break;
    }
    if(Sanity_check)
    {
        payload[len++] = gRKERequest_c;
        payload[len++] = 0x7F;
        payload[len++] = 0x70;
        payload[len++] = gRequestRKEActionTagPayloadLength_c;
        payload[len++] = gFunctionID_c;
        payload[len++] = gFunctionIdPayloadLength_c;
        payload[len++] = function >> 8;
        payload[len++] = function;
        payload[len++] = gActionID_c;
        payload[len++] = gActionIdPayloadLength_c;
        payload[len++] = action;

        result = DK_SendMessage(deviceId,
                                maPeerInformation[deviceId].customInfo.psmChannelId,
                                MessageType,
                                MessageId,
                                len,
                                payload);
        TRACE_DEBUG("RKE Request SubEvent sent");
        TRACE_HEX("Message type", &MessageType, gMessageHeaderSize_c);
        TRACE_HEX("Message ID", &MessageId, gPayloadHeaderSize_c);
        TRACE_HEX("Payload", (uint8_t *)payload, len);
        /* Parsing */
        BleApp_ParsingRKERequestSubEvent(MessageType, MessageId, payload);
    }
    else
    {
        TRACE_ERROR("Invalid parameters for RKE Request SubEvent message.");
    }
    return result;
}

/*! *********************************************************************************
* \brief    sends a CCC DK message.
*
********************************************************************************** */
bleResult_t CCC_SendRKEAuthResponse(deviceId_t deviceId, uintn8_t *pAttestation, uintn8_t attestationLen)
{
    dkMessageType_t MessageType = gDKMessageTypeSupplementaryServiceMessage_c;
    rangingMsgId_t MessageId = gRKEAuthRS_c;
    bleResult_t result;

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            MessageType,
                            MessageId,
                            attestationLen,
                            pAttestation);

    TRACE_INFO("RKE_Auth_RS sent");
    TRACE_HEX("Message type", &MessageType, gMessageHeaderSize_c);
    TRACE_HEX("Message ID", &MessageId, gPayloadHeaderSize_c);
    TRACE_HEX("Payload", (uint8_t *)pAttestation, attestationLen);

    /* Parsing */
    TRACE_DEBUG("Message Type: 0x%02x", MessageType);
    TRACE_DEBUG("Message Id: 0x%02x", MessageId);
    TRACE_DEBUG("Payload length: 0x%02x", attestationLen);
    TRACE_HEX("Arbitrary Data Attestation", (uint8_t *)pAttestation, attestationLen);
    return result;
}

/*! *********************************************************************************
* \brief    sends a Ranging session response message.
*
********************************************************************************** */
static bleResult_t CCC_SendRangingSessionRS(deviceId_t deviceId)
{
    bleResult_t result = gBleSuccess_c;
    uint8_t payload[gRangingSessionResponsePayloadLength_c] = {0};
    dkMessageType_t MessageType = gDKMessageTypeUWBRangingServiceMessage_c;
    rangingMsgId_t MessageId = gRangingSessionRS_c;

    /* TODO: payload fill */

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            MessageType,
                            MessageId,
                            gRangingSessionResponsePayloadLength_c,
                            payload);

    TRACE_INFO("Device Ranging Session Response sent");
    /* Parsing */
    BleApp_ParsingRangingSessionResponse(MessageType, MessageId, payload);
    return result;
}

/*! *********************************************************************************
* \brief    sends a Ranging recovery response message.
*
********************************************************************************** */
static bleResult_t CCC_SendRangingRecoveryRS(deviceId_t deviceId)
{
    bleResult_t result = gBleSuccess_c;
    uint8_t payload[gRangingRecoveryResponsePayloadLength_c] = {0};
    dkMessageType_t MessageType = gDKMessageTypeUWBRangingServiceMessage_c;
    rangingMsgId_t MessageId = gRangingRecoveryRS_c;

    /* TODO: payload fill */

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            MessageType,
                            MessageId,
                            gRangingRecoveryResponsePayloadLength_c,
                            payload);

    TRACE_INFO("Device Ranging Recovery Response sent");
    /* Parsing */
    BleApp_ParsingRangingRecoveryResponse(MessageType, MessageId, payload);
    return result;
}

/*! *********************************************************************************
* \brief    sends a Ranging suspend response message.
*
********************************************************************************** */
static bleResult_t CCC_SendRangingSuspendRS(deviceId_t deviceId)
{
    bleResult_t result = gBleSuccess_c;
    uint8_t payload[gRangingSuspendResponsePayloadLength_c] = {0};
    dkMessageType_t MessageType = gDKMessageTypeUWBRangingServiceMessage_c;
    rangingMsgId_t MessageId = gRangingSuspendRS_c;

    /* TODO: payload fill */

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            MessageType,
                            MessageId,
                            gRangingSuspendResponsePayloadLength_c,
                            payload);

    TRACE_INFO("Device Ranging Suspend Response sent");
    /* Parsing */
    BleApp_ParsingRangingSuspendResponse(MessageType, MessageId, payload);
    return result;
}

/*! *********************************************************************************
* \brief    sends a Ranging capability response message.
*
********************************************************************************** */
static bleResult_t CCC_SendRangingCapabilityRS(deviceId_t deviceId)
{
    bleResult_t result = gBleSuccess_c;
    uint8_t payload[gRangingCapabilityResponsePayloadLength_c] = {0};
    dkMessageType_t MessageType = gDKMessageTypeUWBRangingServiceMessage_c;
    rangingMsgId_t MessageId = gRangingCapabilityRS_c;
    uint8_t len = 0;

    payload[len++]=0x01;
    payload[len++]=0x00;
    payload[len++]=0x00;
    payload[len++]=0x00;
    payload[len++]=0x00;

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            MessageType,
                            MessageId,
                            gRangingCapabilityResponsePayloadLength_c,
                            payload);

    TRACE_INFO("Device Ranging Capability Response sent");
    TRACE_DEBUG("Message type : 0x%02x", MessageType);
    TRACE_DEBUG("Message ID : 0x%02x", MessageId);
    TRACE_DEBUG("Payload length : 0x%04x", gRangingCapabilityResponsePayloadLength_c);
    TRACE_HEX("Payload", (uint8_t *)payload, gRangingCapabilityResponsePayloadLength_c);
    return result;
}

/*! *********************************************************************************
 * \brief        SHA256(RkeChallenge || function id || action id).
 *
 ********************************************************************************** */
static void CCC_DeriveArbitraryData(uintn8_t *pRkeChallenge,
                                    uintn8_t RkeChallengeLen,
                                    uint16_t function,
                                    uintn8_t action,
                                    uintn8_t *pHashOut,
                                    uintn8_t hashOutLen)
{
    uint8_t buffer[gRKEChallengeLength_c + 3] = {0};
    uint8_t len = gRKEChallengeLength_c;

    if(pRkeChallenge && (gRKEChallengeLength_c == RkeChallengeLen) && pHashOut && (SHA256_HASH_SIZE == hashOutLen))
    {
        FLib_MemCpy(buffer, pRkeChallenge, RkeChallengeLen);
        buffer[len++] = function >> 8;
        buffer[len++] = function;
        buffer[len++] = action;
        SHA256_Hash(buffer, len, pHashOut);
    }
}

/*! *********************************************************************************
 * \brief        Sign the arbitraryData with private key
 *
 ********************************************************************************** */
static void CCC_SignArbitraryData(uintn8_t *pArbitraryData, uintn8_t arbitraryDataLen, uintn8_t *pAttestationOut)
{
    /* TODO: How to sign the arbitrary data? */
    uint8_t pKey[16] = {0};
    if(pArbitraryData && arbitraryDataLen && pAttestationOut)
    {
        AES_128_ECB_Encrypt(pArbitraryData, arbitraryDataLen, pKey, pAttestationOut);
        TRACE_HEX("Arbitrary Data signed", pAttestationOut, arbitraryDataLen);
    }
}

/*! *********************************************************************************
 * \brief        Device sends Time Sync to Car Anchor.
 *
 ********************************************************************************** */
static bleResult_t CCC_SendTimeSync(deviceId_t deviceId,
                                    uint64_t *pDevEvtCnt,
                                    uint64_t *pUwbDevTime,
                                    uint8_t success)
{
    bleResult_t result = gBleSuccess_c;

    uint8_t payload[gTimeSyncPayloadLength_c] = {0};
    uint8_t *pPtr = payload;
    
    /* Add DeviceEventCount */
    FLib_MemCpy(pPtr, pDevEvtCnt, sizeof(uint64_t));
    pPtr += sizeof(uint64_t);
    /* Add UWB_Device_Time */
    FLib_MemCpy(pPtr, pUwbDevTime, sizeof(uint64_t));
    pPtr += sizeof(uint64_t);
    /* Skip UWB_Device_Time_Uncertainty */
    pPtr++;
    /* Skip UWB_Clock_Skew_Measurement_available */
    pPtr++;
    /* Skip Device_max_PPM */
    pPtr += sizeof(uint16_t);
    /* Add Success */
    *pPtr = success;
    /* Skip RetryDelay */
    pPtr += sizeof(uint16_t);
    
    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
                            gDKMessageTypeSupplementaryServiceMessage_c,
                            gTimeSync_c,
                            gTimeSyncPayloadLength_c,
                            payload);
    TRACE_INFO("Time Sync sent with UWB Device Time");
    TRACE_HEX_LE("UWB Dev Time", (uint8_t*)pUwbDevTime, (uint8_t)sizeof(uint64_t));
    return result;
}

/*! *********************************************************************************
 * \brief        First Approach messages enables BLE OOB Secure LE pairing for both owner and friend devices
 *
 ********************************************************************************** */
static bleResult_t CCC_FirstApproachReq(deviceId_t deviceId, uint8_t* pBdAddr, gapLeScOobData_t* pOobData)
{
    bleResult_t result = gBleSuccess_c;
    uint8_t aPayload[gFirstApproachReqRspPayloadLength] = {0}; 
    uint8_t *pData = aPayload;
    
    if (FLib_MemCmpToVal(pOobData, 0x00, sizeof(gapLeScOobData_t)))
    {
        result = gBleInvalidParameter_c;
    }
    else
    {
        FLib_MemCpy(pData, pBdAddr, gcBleDeviceAddressSize_c);
        pData += gcBleDeviceAddressSize_c;
        FLib_MemCpy(pData, pOobData->confirmValue, gSmpLeScRandomConfirmValueSize_c);
        pData += gSmpLeScRandomConfirmValueSize_c;
        FLib_MemCpy(pData, pOobData->randomValue, gSmpLeScRandomValueSize_c);
        
        TRACE_INFO("OOB Data printed below:");
        TRACE_HEX("Address", pBdAddr, gcBleDeviceAddressSize_c);
        TRACE_HEX("Confirm", pOobData->confirmValue, gSmpLeScRandomConfirmValueSize_c);
        TRACE_HEX("Random", pOobData->randomValue, gSmpLeScRandomConfirmValueSize_c);
        result = DK_SendMessage(deviceId,
                                maPeerInformation[deviceId].customInfo.psmChannelId,
                                gDKMessageTypeSupplementaryServiceMessage_c,
                                gFirstApproachRQ_c,
                                gFirstApproachReqRspPayloadLength,
                                aPayload);
    }
    
    return result;
}

/*! *********************************************************************************
 * \brief        Send Standard Transaction Request to car anchor
 *
 ********************************************************************************** */
static bleResult_t CCC_StandardTransactionReq(deviceId_t deviceId)
{
    bleResult_t result = gBleSuccess_c;
    dkMessageType_t MessageType = gDKMessageTypeDKEventNotification_c;
    rangingMsgId_t MessageId = gDkEventNotification_c;
    uint8_t aPayload[gStandardTransactionReqPayloadLength] = {0};
    uint8_t len = 0;

    aPayload[len++] = gCommandComplete_c;
    aPayload[len++] = gRequestStandardTransaction_c;
    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
							MessageType,
							MessageId,
							gStandardTransactionReqPayloadLength,
                            aPayload);

    TRACE_INFO("Request_Standard_Transaction sent");
    TRACE_DEBUG("Message type : 0x%02x", MessageType);
    TRACE_DEBUG("Message ID : 0x%02x", MessageId);
    TRACE_DEBUG("Payload length : 0x%04x", gStandardTransactionReqPayloadLength);
    TRACE_HEX("Payload", (uint8_t *)aPayload, gStandardTransactionReqPayloadLength);

    return result;
}

/*! *********************************************************************************
 * \brief        Get APDU response from SE and Send it to car anchor
 *
 ********************************************************************************** */
static bleResult_t CCC_SendAPDUResp(deviceId_t deviceId, apduId_t Id)
{
    bleResult_t result = gBleSuccess_c;
    dkMessageType_t MessageType = gDKMessageTypeSEMessage_c;
    rangingMsgId_t MessageId = gDkApduRS_c;
    uint16_t ApduLength = 0;
    for(int i=0; i<Max_ApduId; i++)
    {
    	if(Id == ApduRegistry[i].apduId)
    	{
    		ApduLength = ApduRegistry[i].apduLength;
    	}
    }
    uint8_t aPayload[ApduLength];

    /* get Create ranging key response payload from secure element */
    Array_string_hex(buf, 2*ApduLength, aPayload);

    result = DK_SendMessage(deviceId,
                            maPeerInformation[deviceId].customInfo.psmChannelId,
							MessageType,
							MessageId,
							ApduLength,
                            aPayload);

    TRACE_INFO("Create Ranging Key Response sent");
    TRACE_DEBUG("Message type : 0x%02x", MessageType);
    TRACE_DEBUG("Message ID : 0x%02x", MessageId);
    TRACE_DEBUG("Payload length : 0x%04x", ApduLength);
    TRACE_HEX("Payload", (uint8_t *)aPayload, ApduLength);

    return result;
}

/*! *********************************************************************************
 * \brief        Returns UWB clock. 
 ********************************************************************************** */
static uint64_t GetUwbClock(void)
{
    uint32_t randomNo;
    RNG_GetRandomNo (&randomNo);
    /* Get a simulated UWB clock */
    return TM_GetTimestamp() + (uint8_t)randomNo;

}
/*! *********************************************************************************
* @}
********************************************************************************** */
