/*
 * uwb_manager.c
 *
 *  Created on: 10 févr. 2024
 *      Author: yhermi
 */

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
#include "fsm.h"
#include "EmbeddedTypes.h"
#include "fsl_os_abstraction.h"
#include "timers.h"
#include "uwb_manager.h"
#include "trace.h"
#include "gap_interface.h"
#include "ble_constants.h"
#include "ble_conn_manager.h"
#include "digital_key_device.h"
#include "digital_key_interface.h"
#include "app_digital_key_device.h"
#include "app_nvm.h"
#include "phscaUwb.h"
#include "sensors.h"

/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/
typedef enum
{
    UWB_STATE_IDLE = 0,
    UWB_STATE_STANDBY,
    UWB_STATE_ACTIVE,
    UWB_STATE_FREEZE,
    UWB_STATE_MAX
}uwb_state_t;

/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Private functions prototypes
*************************************************************************************
************************************************************************************/
static void _uwb_on_ble_connected(void);
static void _uwb_on_start_ranging(void);
static void _uwb_on_recover_ranging(void);
static void _uwb_on_stop_ranging(void);
static void _uwb_on_ble_disconnected(void);
static void _uwb_on_enter_freeze(void);
static void _uwb_on_exit_freeze(void);

/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/
static uint32_t s_u32uwbState;

static const fsm_entry_t c_tstUwbIdleState[] =
{
    {UWB_EVENT_BLE_CONNECTED,         _uwb_on_ble_connected       },
    {UWB_EVENT_ENTER_FREEZE,          _uwb_on_enter_freeze        },
};

static const fsm_entry_t c_tstUwbStandbyState[] =
{
    {UWB_EVENT_START_RANGING,         _uwb_on_start_ranging       },
    {UWB_EVENT_RECOVER_RANGING,       _uwb_on_recover_ranging     },
    {UWB_EVENT_ENTER_FREEZE,          _uwb_on_enter_freeze        },
    {UWB_EVENT_BLE_DISCONNECTED,      _uwb_on_ble_disconnected    },
};

static const fsm_entry_t c_tstUwbActiveState[] =
{
    {UWB_EVENT_STOP_RANGING,         _uwb_on_stop_ranging         },
    {UWB_EVENT_ENTER_FREEZE,         _uwb_on_enter_freeze         },
    {UWB_EVENT_BLE_DISCONNECTED,     _uwb_on_ble_disconnected     },
};

static const fsm_entry_t c_tstUwbFreezeState[] =
{
    {UWB_EVENT_EXIT_FREEZE,           _uwb_on_exit_freeze          },
};

static const fsm_state_t c_tstUwbFsmStates[] =
{
    /* UWB_STATE_IDLE     */       FSM_STATE_DEF(c_tstUwbIdleState),
    /* UWB_STATE_STANDBY  */       FSM_STATE_DEF(c_tstUwbStandbyState),
    /* UWB_STATE_ACTIVE   */       FSM_STATE_DEF(c_tstUwbActiveState),
    /* UWB_STATE_FREEZE   */       FSM_STATE_DEF(c_tstUwbFreezeState),
};

static const char* c_tszUwbStatesLookupTable[] =
{
    /* UWB_STATE_IDLE           */ "UWB_IDLE",
    /* UWB_STATE_STANDBY        */ "UWB_STANDBY",
    /* UWB_STATE_ACTIVE         */ "UWB_ACTIVE",
    /* UWB_STATE_FREEZE         */ "FREEZE",
};

static const char* c_tszUwbEventsLookupTable[] =
{
    /* UWB_EVENT_BLE_CONNECTED      */    "BLE_CONNECTED",
    /* UWB_EVENT_START_RANGING      */    "START_RANGING",
    /* UWB_EVENT_RECOVER_RANGING    */    "RECOVER_RANGING",
    /* UWB_EVENT_STOP_RANGING       */    "STOP_RANGING",
    /* UWB_EVENT_BLE_DISCONNECTED   */    "BLE_DISCONNECTED",
    /* UWB_EVENT_ENTER_FREEZE       */    "ENTER_FREEZE",
    /* UWB_EVENT_EXIT_FREEZE        */    "EXIT_FREEZE",
};

FSM_DEF(Uwb, s_u32uwbState, c_tstUwbFsmStates);
OSA_MSGQ_HANDLE_DEFINE(s_UwbQueueHandle, 0, 0);

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
 * \brief  This is the Uwb manager that implements the uwb state machine
          according to VALEO specifications.
********************************************************************************** */

void UWB_MGR_init(void)
{
    OSA_MsgQCreate((osa_msgq_handle_t)s_UwbQueueHandle, 10U, sizeof(uint32_t));
    fsm_init(FSM_ID(Uwb), UWB_STATE_IDLE);
    TRACE_DEBUG("Uwb current State: %s", c_tszUwbStatesLookupTable[s_u32uwbState]);
    TRACE_INFO("------------------------------------------------");
}

void UWB_MGR_run(void)
{
    osa_status_t eStatus;
    uint32_t u32Event;
    int iRet;

    while(TRUE)
    {
        eStatus = OSA_MsgQGet(s_UwbQueueHandle, (osa_msg_handle_t) &u32Event, osaWaitForever_c);
        if(KOSA_StatusSuccess == eStatus)
        {
            if(s_u32uwbState < UWB_STATE_MAX)
            {
                TRACE_DEBUG("Uwb Initial State: %s", c_tszUwbStatesLookupTable[s_u32uwbState]);
            }
            if(u32Event < UWB_EVENT_MAX)
            {
                TRACE_DEBUG("Uwb received Event: %s", c_tszUwbEventsLookupTable[u32Event]);
            }
            TRACE_INFO("Actions:");
            iRet = fsm_process(FSM_ID(Uwb), u32Event);
            if(iRet < 0)
            {
                TRACE_WARNING("No action for this event within this state!");
            }
            if(s_u32uwbState < UWB_STATE_MAX)
            {
                TRACE_DEBUG("Uwb current State: %s", c_tszUwbStatesLookupTable[s_u32uwbState]);
            }
            TRACE_INFO("------------------------------------------------");
        }
    }
}

/*! *********************************************************************************
 * \brief  Notify the uwb manager about a new event.
 *
 * \param[in]    u32Event       New event to be sent
********************************************************************************** */
void UWB_MGR_notify(uint32_t u32Event)
{
    OSA_MsgQPut(s_UwbQueueHandle, (osa_msg_handle_t) &u32Event);
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
 * \brief  This is the uwb manager event handler: BLE_CONNECTED.
********************************************************************************** */
static void _uwb_on_ble_connected(void)
{
	s_u32uwbState = UWB_STATE_STANDBY;
}

/*! *********************************************************************************
 * \brief  This is the uwb manager event handler: START_RANGING.
********************************************************************************** */
static void _uwb_on_start_ranging(void)
{
    TRACE_DEBUG("Battery level : %d%%",SENSORS_GetBatteryLevel());
    TRACE_INFO("Start UWB Ranging.");
    phscaUwb_Init();
    s_u32uwbState = UWB_STATE_ACTIVE;
}

/*! *********************************************************************************
 * \brief  This is the uwb manager event handler: RECOVER_RANGING.
********************************************************************************** */
static void _uwb_on_recover_ranging(void)
{
    TRACE_DEBUG("Battery level : %d%%",SENSORS_GetBatteryLevel());
    TRACE_INFO("Recover UWB Ranging.");
    phscaUwb_Resuming();
    s_u32uwbState = UWB_STATE_ACTIVE;
}

/*! *********************************************************************************
 * \brief  This is the uwb manager event handler: STOP_RANGING.
********************************************************************************** */
static void _uwb_on_stop_ranging(void)
{
    TRACE_INFO("Stop UWB Ranging.");
    phscaUwb_Stopping();
    s_u32uwbState = UWB_STATE_STANDBY;
    TRACE_DEBUG("Battery level : %d%%",SENSORS_GetBatteryLevel());
}

/*! *********************************************************************************
 * \brief  This is the uwb manager event handler: BLE_DISCONNECTED.
********************************************************************************** */
static void _uwb_on_ble_disconnected(void)
{
    if (s_u32uwbState == UWB_STATE_ACTIVE)
    {
    	TRACE_INFO("Stop UWB Ranging.");
    	phscaUwb_Stopping();
    }
    s_u32uwbState = UWB_STATE_IDLE;
    TRACE_DEBUG("Battery level : %d%%",SENSORS_GetBatteryLevel());
}

/*! *********************************************************************************
 * \brief  This is the uwb manager event handler: UWB_EVENT_ENTER_FREEZE.
********************************************************************************** */
static void _uwb_on_enter_freeze(void)
{
    if (s_u32uwbState == UWB_STATE_ACTIVE)
    {
        TRACE_INFO("Stop the UWB Ranging.");
        phscaUwb_Stopping();
    }
    s_u32uwbState = UWB_STATE_FREEZE;
    TRACE_DEBUG("Battery level : %d%%",SENSORS_GetBatteryLevel());
}

/*! *********************************************************************************
 * \brief  This is the uwb manager event handler: UWB_EVENT_EXIT_FREEZE.
********************************************************************************** */
static void _uwb_on_exit_freeze(void)
{
    TRACE_INFO("Re-initialize the uwb state machine.");
    s_u32uwbState = UWB_STATE_IDLE;
}


/*! *********************************************************************************
* @}
********************************************************************************** */



