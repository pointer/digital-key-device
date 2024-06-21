/*! *********************************************************************************
* \addtogroup Digital Key Device Application
* @{
********************************************************************************** */
/*! *********************************************************************************
* \file shell_digital_key_device.c
*
* Copyright 2021 - 2022 NXP
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
#include "app_preinclude.h"
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
#include "fsl_shell.h"
#endif
#include "fsl_adapter_reset.h"
#include "fsl_component_mem_manager.h"
#include "fsl_component_timer_manager.h"

#include "app.h"

/* BLE Host Stack */
#include "gap_interface.h"

#include "digital_key_device.h"
#include "shell_digital_key_device.h"
#include "app_conn.h"
#include "stdlib.h"
#include "app_nvm.h"

#include "software_version.h"

#include "keyfob_manager.h"

/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/
#define SHELL_CHECK_EQUAL_STRINGS(input_varname, expected_varname)   (0 == strcmp((input_varname), ((expected_varname))))

/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/
#define SHELL_GET_COMMAND                              "get"
#define SHELL_SET_COMMAND                              "set"
#define SHELL_LISTBP_COMMAND                           "listbp"

/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
SHELL_HANDLE_DEFINE(g_shellHandle);

/* Shell */
static shell_status_t ShellReset_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellFactoryReset_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellStartDiscovery_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellStopDiscovery_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellDisconnect_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellSetBondingData_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellListBondedDev_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellRemoveBondedDev_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellRKELock_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellRKEUnlock_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellRKERelease_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellSetSystemParameters_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellGetSystemParameters_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellListSystemParameters_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellGetSoftwareVersion_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellTriggerMotionSensor_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);
static shell_status_t ShellResetAfterDisconnection_Command(shell_handle_t shellHandle, int32_t argc,char* argv[]);
static shell_status_t ShellSwitchGAPRole_Command(shell_handle_t shellHandle, int32_t argc,char* argv[]);
static shell_status_t ShellListBleKeys_Command(shell_handle_t shellHandle, int32_t argc, char * argv[]);


static uint8_t BleApp_ParseHexValue(char* pInput);
static uint32_t BleApp_AsciiToHex(char *pString, uint32_t strLen);

static void ShellResetTimeoutTimerCallback(void* pParam);

static shell_command_t mResetCmd =
{
    .pcCommand = "reset",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellReset_Command,
    .pcHelpString = "\r\n\"reset\": Reset MCU.\r\n",
};

static shell_command_t mFactoryResetCmd =
{
    .pcCommand = "factoryreset",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellFactoryReset_Command,
    .pcHelpString = "\r\n\"factoryreset\": Factory Reset.\r\n",
};

static shell_command_t mSdCmd =
{
    .pcCommand = "sd",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellStartDiscovery_Command,
    .pcHelpString = "\r\n\"sd\": Start Discovery for Owner Pairing or Passive Entry.\r\n",
};

static shell_command_t mSpdCmd =
{
    .pcCommand = "spd",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellStopDiscovery_Command,
    .pcHelpString = "\r\n\"spd\": Stop Discovery.\r\n",
};

static shell_command_t mDcntCmd =
{
    .pcCommand = "dcnt",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellDisconnect_Command,
    .pcHelpString = "\r\n\"dcnt\": Disconnect all peers.\r\n",
};

static shell_command_t mSetBondingDataCmd =
{
    .pcCommand = "setbd",
    .cExpectedNumberOfParameters = SHELL_IGNORE_PARAMETER_COUNT,
    .pFuncCallBack = ShellSetBondingData_Command,
    .pcHelpString = "\r\n\"setbd\": Set bonding data.\r\n",
};

static shell_command_t mListBondedDevCmd =
{
    .pcCommand = "listbd",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellListBondedDev_Command,
    .pcHelpString = "\r\n\"listbd\": List bonded devices.\r\n",
};

static shell_command_t mRemoveBondedDevCmd =
{
    .pcCommand = "removebd",
    .cExpectedNumberOfParameters = SHELL_IGNORE_PARAMETER_COUNT,
    .pFuncCallBack = ShellRemoveBondedDev_Command,
    .pcHelpString = "\r\n\"removebd\": Remove bonded devices.\r\n",
};

static shell_command_t mLockCmd =
{
    .pcCommand = "lock",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellRKELock_Command,
    .pcHelpString = "\r\n\"lock\": Lock the car door.\r\n",
};

static shell_command_t mUnlockCmd =
{
    .pcCommand = "unlock",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellRKEUnlock_Command,
    .pcHelpString = "\r\n\"unlock\": Unlock the car door.\r\n",
};

static shell_command_t mReleaseCmd =
{
    .pcCommand = "release",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellRKERelease_Command,
    .pcHelpString = "\r\n\"release\": Release the car trunk.\r\n",
};

static shell_command_t mSetCmd =
{
    .pcCommand = "set",
    .cExpectedNumberOfParameters = 2,
    .pFuncCallBack = ShellSetSystemParameters_Command,
    .pcHelpString = "\r\n\"set\": Set and store system parameters (BMW PoC) into non-volatile memory.\r\n",
};

static shell_command_t mGetCmd =
{
    .pcCommand = "get",
    .cExpectedNumberOfParameters = 1,
    .pFuncCallBack = ShellGetSystemParameters_Command,
    .pcHelpString = "\r\n\"get\": Load and read system parameters (BMW PoC) from non-volatile memory.\r\n",
};

static shell_command_t mListSystemParamsCmd =
{
    .pcCommand = "listbp",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellListSystemParameters_Command,
    .pcHelpString = "\r\n\"listbp\": List all system parameters (BMW PoC) from non-volatile memory.\r\n",
};

static shell_command_t mGetSoftVersionCmd =
{
    .pcCommand = "version",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellGetSoftwareVersion_Command,
    .pcHelpString = "\r\n\"version\": Get the software version\r\n",
};

static shell_command_t mTriggerMotionSensorCmd =
{
    .pcCommand = "ms",
    .cExpectedNumberOfParameters = 1,
    .pFuncCallBack = ShellTriggerMotionSensor_Command,
    .pcHelpString = "\r\n\"ms <evt>\": Trigger simulated motion sensor events (wd: WALK_DETECTED, msd: MOTION_STILL_DETECTED)\r\n",
};

static shell_command_t mRstAfterDcntCmd =
{
    .pcCommand = "DcntRst",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellResetAfterDisconnection_Command,
    .pcHelpString = "\r\n\"DcntRst\": Reset after disconnection\r\n",
};

static shell_command_t mSwitchGAPRoleCmd =
{
    .pcCommand = "switchrole",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellSwitchGAPRole_Command,
    .pcHelpString = "\r\n\"switchrole\": Switch GAP role\r\n",
};

static shell_command_t mListBleKeysCmd =
{
    .pcCommand = "listbk",
    .cExpectedNumberOfParameters = 0,
    .pFuncCallBack = ShellListBleKeys_Command,
    .pcHelpString = "\r\n\"listbk\": List Ble keys (IRK/LTK) from non-volatile memory.\r\n",
};

#endif
/************************************************************************************
*************************************************************************************
* Private functions prototypes
*************************************************************************************
************************************************************************************/


/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief  Initializes the SHELL module .
*
* \param[in]  prompt the string which will be used for command prompt
*
* \remarks
*
********************************************************************************** */
void AppShellInit(char *prompt)
{
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
   /* UI */
    shell_status_t status = kStatus_SHELL_Error;
    (void)status;

    status = SHELL_Init((shell_handle_t)g_shellHandle, (serial_handle_t)gSerMgrIf, prompt);
    assert(kStatus_SHELL_Success == status);

    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mResetCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mFactoryResetCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mSdCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mSpdCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mDcntCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mSetBondingDataCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mListBondedDevCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mRemoveBondedDevCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mLockCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mUnlockCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mReleaseCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mSetCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mGetCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mListSystemParamsCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mGetSoftVersionCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mTriggerMotionSensorCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mRstAfterDcntCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mSwitchGAPRoleCmd);
    assert(kStatus_SHELL_Success == status);
    status = SHELL_RegisterCommand((shell_handle_t)g_shellHandle, &mListBleKeysCmd);
    assert(kStatus_SHELL_Success == status);
#endif
}

#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
/*! *********************************************************************************
* \brief        Prints string of hex values
*
* \param[in]    pHex    pointer to hex value.
* \param[in]    len     hex value length.
********************************************************************************** */
void BleApp_PrintHex(uint8_t *pHex, uint8_t len)
{
    for (uint32_t i = 0; i<len; i++)
    {
        (void)SHELL_PrintfSynchronization((shell_handle_t)g_shellHandle, (char const*)FORMAT_Hex2Ascii(pHex[i]));
    }
}

/*! *********************************************************************************
* \brief        Prints string of hex values in reversed order
*
* \param[in]    pHex    pointer to hex LE value.
* \param[in]    len     hex value length.
********************************************************************************** */
void BleApp_PrintHexLe(uint8_t *pHex, uint8_t len)
{
    for (uint32_t i = 0; i<len; i++)
    {
        (void)SHELL_PrintfSynchronization((shell_handle_t)g_shellHandle, (char const*)FORMAT_Hex2Ascii(pHex[((uint32_t)len - 1U) - i]));
    }
}
#endif 

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/
#if defined(gAppUseShellInApplication_d) && (gAppUseShellInApplication_d == 1)
/*! *********************************************************************************
* \brief        Save Bonding Data on device.
*
* \param[in]    argc           Number of arguments
* \param[in]    argv           Pointer to arguments
*
* \return       shell_status_t  Returns the command processing status
********************************************************************************** */
static shell_status_t ShellSetBondingData_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if (argc == 6)
    {
        if(mpfBleEventHandler != NULL)
        {
            appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t) + sizeof(appBondingData_t));
            if(pEventData != NULL)
            {
                pEventData->appEvent = mAppEvt_Shell_SetBondingData_Command_c;
                pEventData->eventData.pData = pEventData + 1;
                appBondingData_t *pAppBondingEventData = pEventData->eventData.pData;
                if ( sizeof(uint8_t) == BleApp_ParseHexValue(argv[1]) )
                {
                    pAppBondingEventData->nvmIndex = (uint8_t)*argv[1];
                }
                
                if ( sizeof(bleAddressType_t) == BleApp_ParseHexValue(argv[2]) )
                {
                    pAppBondingEventData->addrType = (uint8_t)*argv[2];
                }
                
                if ( gcBleDeviceAddressSize_c ==  BleApp_ParseHexValue(argv[3]) )
                {
                    FLib_MemCpy(pAppBondingEventData->deviceAddr, argv[3], gcBleDeviceAddressSize_c);
                }
                
                if ( gcSmpMaxLtkSize_c == BleApp_ParseHexValue(argv[4]) )
                {
                    FLib_MemCpy(pAppBondingEventData->aLtk, argv[4], gcSmpMaxLtkSize_c);
                }
                
                if ( gcSmpIrkSize_c ==  BleApp_ParseHexValue(argv[5]) )
                {
                    FLib_MemCpy(pAppBondingEventData->aIrk, argv[5], gcSmpIrkSize_c);
                }

                if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                {
                    (void)MEM_BufferFree(pEventData);
                }
            }
        }
    }
    else
    {
        shell_write("\r\nUsage: \
                    \r\nsetbd nvm_index addr_type peer_device_address ltk irk  \
                    \r\n");
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
* \brief        List bonded devices.
*
* \param[in]    argc           Number of arguments
* \param[in]    argv           Pointer to arguments
*
* \return       shell_status_t  Returns the command processing status
********************************************************************************** */
static shell_status_t ShellListBondedDev_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_ListBondedDev_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
* \brief        remove bonded devices.
*
* \param[in]    argc           Number of arguments
* \param[in]    argv           Pointer to arguments
*
* \return       shell_status_t  Returns the command processing status
********************************************************************************** */
static shell_status_t ShellRemoveBondedDev_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if (argc == 2)
    {
        if(mpfBleEventHandler != NULL)
        {
            appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
            if(pEventData != NULL)
            {
                pEventData->appEvent = mAppEvt_Shell_RemoveBondedDev_Command_c;
                if ( sizeof(uint8_t) == BleApp_ParseHexValue(argv[1]) )
                {
                    /* Store nvm index to be removed in eventData.peerDeviceId  */
                    pEventData->eventData.peerDeviceId = (uint8_t)*argv[1];
                    if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
                    {
                        (void)MEM_BufferFree(pEventData);
                    }
                }
            }
        }
    }
    else
    {
        shell_write("\r\nUsage: \
                    \r\nremovebd nvm_index \
                    \r\n");
    }
    
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Reset MCU.
 *
 ********************************************************************************** */
static shell_status_t ShellReset_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    shell_status_t status = kStatus_SHELL_Success;
    static TIMER_MANAGER_HANDLE_DEFINE(resetTmrId);
    timer_status_t tmrStatus;
    
    tmrStatus = TM_Open(resetTmrId);
    
    if (tmrStatus != kStatus_TimerSuccess)
    {
        status = kStatus_SHELL_Error;
    }
    else
    {
        /* Start 10ms timer before reset to allow for shell prompt to be 
        sent over the serial interface. */
        (void)TM_InstallCallback((timer_handle_t)resetTmrId, ShellResetTimeoutTimerCallback, NULL);
        (void)TM_Start((timer_handle_t)resetTmrId, (uint8_t)kTimerModeSingleShot, 10U);
    }

    return status;
}

/*! *********************************************************************************
* \brief        Reset timer callback.
                Called on timer task.
*
* \param[in]    pParam              not used
********************************************************************************** */
static void ShellResetTimeoutTimerCallback(void* pParam)
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_Reset_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
}

/*! *********************************************************************************
 * \brief        Factory Reset.
 *
 ********************************************************************************** */
static shell_status_t ShellFactoryReset_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_FactoryReset_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Initialize Owner Pairing or Passive Entry.
 *
 ********************************************************************************** */
static shell_status_t ShellStartDiscovery_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_ShellStartDiscovery_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Stop discovery, if active.
 *
 ********************************************************************************** */
static shell_status_t ShellStopDiscovery_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_StopDiscovery_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Disconnect device.
 *
 ********************************************************************************** */
static shell_status_t ShellDisconnect_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_Disconnect_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Lock the door.
 *
 ********************************************************************************** */
static shell_status_t ShellRKELock_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_RKELock_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}


/*! *********************************************************************************
 * \brief        Unlock the door.
 *
 ********************************************************************************** */
static shell_status_t ShellRKEUnlock_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_RKEUnlock_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Release the car trunk.
 *
 ********************************************************************************** */
static shell_status_t ShellRKERelease_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_RKERelease_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Set and store system parameters into NVM.
 *
 ********************************************************************************** */
static shell_status_t ShellSetSystemParameters_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    shell_status_t retval = kStatus_SHELL_Success;
    systemParameters_t *pSysParams = NULL;
    int32_t newValue;
    systemParamID_t i;
    bool_t found = FALSE;

    if(argc == 3)
    {
        /* Check the command stored in argv[0] */
        if((0 == strncmp(argv[0], SHELL_SET_COMMAND, MIN(sizeof(argv[0]), sizeof(SHELL_SET_COMMAND)))))
        {
            for(i=0; i<ParamMaxID; i++)
            {
                /* Check the varname stored in argv[1] */
                if(SHELL_CHECK_EQUAL_STRINGS(argv[1], SystemParamsRegistry[i].name))
                {
                    /* Check if the value stored in argv[2] is within the range */
                    newValue = atoi(argv[2]);
                    if(gBleSuccess_c != App_NvmWriteSystemParam(SystemParamsRegistry[i].eID, newValue))
                    {
                        retval = kStatus_SHELL_Error;
                    }
                    else
                    {
                    	SHELL_Printf((shell_handle_t)g_shellHandle, "Set %s to %d", SystemParamsRegistry[i].name, newValue);
                    }
                    found = TRUE;
                    break;
                }
            }
            if(FALSE == found)
            {
                retval = kStatus_SHELL_Error;
            }
        }
        else
        {
            retval = kStatus_SHELL_Error;
        }
    }
    else
    {
        retval = kStatus_SHELL_Error;
    }
    if(kStatus_SHELL_Error == retval)
    {
    	shell_write("ERROR");
    }
    return retval;
}

/*! *********************************************************************************
 * \brief        Get system parameters.
 *
 ********************************************************************************** */
static shell_status_t ShellGetSystemParameters_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    shell_status_t retval = kStatus_SHELL_Success;
    systemParameters_t *pSysParams = NULL;
    uint8_t i;
    bool_t found = FALSE;

    if(argc == 2)
    {
        /* Check the command stored in argv[0] */
        if((0 == strncmp(argv[0], SHELL_GET_COMMAND, MIN(sizeof(argv[0]), sizeof(SHELL_GET_COMMAND)))))
        {
            for(i=0; i<ParamMaxID; i++)
            {
                /* Check the varname stored in argv[1] */
                if(SHELL_CHECK_EQUAL_STRINGS(argv[1], SystemParamsRegistry[i].name))
                {
                    /* Read value */
                	App_NvmReadSystemParams(&pSysParams);
                    SHELL_Printf((shell_handle_t)g_shellHandle, "%d\r\n", pSysParams->system_params.buffer[i]);
                    found = TRUE;
                    break;
                }
            }
            if(FALSE == found)
            {
                retval = kStatus_SHELL_Error;
            }
        }
        else
        {
            retval = kStatus_SHELL_Error;
        }
    }
    else
    {
        retval = kStatus_SHELL_Error;
    }
    if(kStatus_SHELL_Error == retval)
    {
        shell_write("ERROR\n\r");
    }
    return retval;
}

/*! *********************************************************************************
 * \brief        List all System parameters.
 *
 ********************************************************************************** */
static shell_status_t ShellListSystemParameters_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    shell_status_t retval = kStatus_SHELL_Success;
    systemParameters_t *pSysParams = NULL;
    uint8_t i;

    if(argc == 1)
    {
        /* Check the command stored in argv[0] */
        if((0 == strncmp(argv[0], SHELL_LISTBP_COMMAND, MIN(sizeof(argv[0]), sizeof(SHELL_LISTBP_COMMAND)))))
        {
        	App_NvmReadSystemParams(&pSysParams);
            for(i=0; i<ParamMaxID; i++)
            {
                /* Read value */
                SHELL_Printf((shell_handle_t)g_shellHandle, "%s = %d\r\n", SystemParamsRegistry[i].name, pSysParams->system_params.buffer[i]);
            }
        }
        else
        {
            retval = kStatus_SHELL_Error;
        }
    }
    else
    {
        retval = kStatus_SHELL_Error;
    }
    if(kStatus_SHELL_Error == retval)
    {
        shell_write("ERROR\n\r");
    }
    return retval;
}

/*! *********************************************************************************
 * \brief        Get the software version.
 *
 ********************************************************************************** */
static shell_status_t ShellGetSoftwareVersion_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    SHELL_Printf((shell_handle_t)g_shellHandle, "BMW Keyfob PoC - Device  v%s\r\n", SOFTWARE_VERSION);
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Trigger simulated motion sensor events.
 *
 ********************************************************************************** */
static shell_status_t ShellTriggerMotionSensor_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(argv && argv[1])
    {
        if(SHELL_CHECK_EQUAL_STRINGS(argv[1], "wd"))
        {
            KEYFOB_MGR_notify(KEYFOB_EVENT_WALK_DETECTED);
        }
        else if(SHELL_CHECK_EQUAL_STRINGS(argv[1], "msd"))
        {
            KEYFOB_MGR_notify(KEYFOB_EVENT_MOTION_STILL_DETECTED);
        }
        else if(SHELL_CHECK_EQUAL_STRINGS(argv[1], "nad"))
        {
            KEYFOB_MGR_notify(KEYFOB_EVENT_NO_ACC_DETECTED_AFTER_10_MIN);
        }
        else
        {
            /* Do nothing */
        }
        return kStatus_SHELL_Success;
    }
    else
    {
        return kStatus_SHELL_Error;
    }
}

static uint8_t BleApp_ParseHexValue(char* pInput)
{
    uint8_t i, length = (uint8_t)strlen(pInput);
    uint32_t value;
    uint8_t result = 0U;

    /* If the hex misses a 0, return error. Process single character */
    if ((length == 1U) || (length % 2U) == 0U)
    {
        if(0 == strncmp(pInput, "0x", 2))
        {
            length -= 2U;

            /* Save as little endian hex value */
            value = BleApp_AsciiToHex(pInput + 2, FLib_StrLen(pInput+2));

            FLib_MemCpy(pInput, &value, sizeof(uint32_t));

            result = length/2U;
        }
        else if (length > 1U)
        {
            char octet[2];

            /* Save as big endian hex */
            for(i=0U;i < length / 2U; i++)
            {
                FLib_MemCpy(octet, &pInput[i*2U], 2U);

                pInput[i] = (char)BleApp_AsciiToHex(octet, 2U);
            }
            result = length/2U;
        }
        else
        {
            /* Convert single character from ASCII to hex */
            pInput[0] = (char)BleApp_AsciiToHex(pInput, length);
            result = length;
        }
    }

    return result;
}

/*! *********************************************************************************
 * \brief        Reset after disconnection.
 *
 ********************************************************************************** */
static shell_status_t ShellResetAfterDisconnection_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_ResetAfterDisconnection_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        Switch the current GAP Role.
 *
 ********************************************************************************** */
static shell_status_t ShellSwitchGAPRole_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    if(mpfBleEventHandler != NULL)
    {
        appEventData_t *pEventData = MEM_BufferAlloc(sizeof(appEventData_t));
        if(pEventData != NULL)
        {
            pEventData->appEvent = mAppEvt_Shell_SwitchGAPRole_Command_c;
            if (gBleSuccess_c != App_PostCallbackMessage(mpfBleEventHandler, pEventData))
            {
                (void)MEM_BufferFree(pEventData);
            }
        }
    }
    return kStatus_SHELL_Success;
}

/*! *********************************************************************************
 * \brief        List IRK/LTK keys.
 *
 ********************************************************************************** */
static shell_status_t ShellListBleKeys_Command(shell_handle_t shellHandle, int32_t argc, char * argv[])
{
    shell_status_t retval = kStatus_SHELL_Success;
    IrkLtkKeys_t *pBleKeys = NULL;
    uint8_t i,j;

    if(argc == 1)
    {
        /* Check the command stored in argv[0] */
        if((0 == strncmp(argv[0], "listbk", MIN(sizeof(argv[0]), sizeof("listbk")))))
        {
        	App_NvmReadBleKeys(&pBleKeys);
            for(i=0; i<KeyMaxID; i++)
            {
                SHELL_Printf((shell_handle_t)g_shellHandle, "%s = ", BleKeysRegistry[i].name);
                for(j=0; j<BleKeysRegistry[i].max_size; j++)
                {
                	SHELL_Printf((shell_handle_t)g_shellHandle, "0x%02x, ", (pBleKeys->ble_keys.keys[i]).key[j]);
                }
                shell_write("\r\n");
            }
        }
        else
        {
            retval = kStatus_SHELL_Error;
        }
    }
    else
    {
        retval = kStatus_SHELL_Error;
    }
    if(kStatus_SHELL_Error == retval)
    {
        shell_write("ERROR\n\r");
    }
    return retval;
}

/*!*************************************************************************************************
 *  \brief  Converts a string into hex.
 *
 *  \param  [in]    pString     pointer to string
 *  \param  [in]    strLen      string length
 *
 * \return uint32_t value in hex
 **************************************************************************************************/
static uint32_t BleApp_AsciiToHex(char *pString, uint32_t strLen)
{
    uint32_t length = strLen;
    uint32_t retValue = 0U;
    int32_t hexDig = 0;
    bool_t validChar;

    /* Loop until reaching the end of the string or the given length */
    while ((length != 0U) && (pString != NULL))
    {
        hexDig = 0;
        validChar = FALSE;

        /* digit 0-9 */
        if (*pString >= '0' && *pString <= '9')
        {
            hexDig = *pString - '0';
            validChar = TRUE;
        }

        /* character 'a' - 'f' */
        if (*pString >= 'a' && *pString <= 'f')
        {
            hexDig = *pString - 'a' + 10;
            validChar = TRUE;
        }

        /* character 'A' - 'B' */
        if (*pString >= 'A' && *pString <= 'F')
        {
            hexDig = *pString - 'A' + 10;
            validChar = TRUE;
        }

        /* a hex digit is 4 bits */
        if (validChar == TRUE)
        {
            retValue = (uint32_t)((retValue << 4U) ^ (uint32_t)hexDig);
        }

        /* Increment position */
        pString++;
        length--;
    }

    return retValue;
}
#endif
