/*! *********************************************************************************
* \file keyfob_manager.c
*
* Copyright 2023 Sofiatech - Jawhar KEBEILI
* All rights reserved.
*
********************************************************************************** */

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
#include "fsm.h"
#include "EmbeddedTypes.h"
#include "fsl_os_abstraction.h"
#include "timers.h"
#include "keyfob_manager.h"
#include "uwb_manager.h"
#include "trace.h"
#include "gap_interface.h"
#include "ble_constants.h"
#include "ble_conn_manager.h"
#include "digital_key_device.h"
#include "digital_key_interface.h"
#include "app_digital_key_device.h"
#include "app_nvm.h"
#include "sensors.h"
#include "app.h"
#include "board_comp.h"
#include "pin_mux.h"
#include "motion_sensor.h"
#include "app_preinclude.h"

/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/
typedef enum
{
  KEYFOB_STATE_INIT = 0,
  KEYFOB_STATE_DEEP_SLEEP,
  KEYFOB_STATE_SCANNING_SLOW,
  KEYFOB_STATE_STAND_BY,
  KEYFOB_STATE_BLE_CONNECTION_AUTHENTICATION,
  KEYFOB_STATE_SCANNING_FAST,
  KEYFOB_STATE_CONNECTED,
  KEYFOB_STATE_FREEZE,
  KEYFOB_STATE_MAX
}keyfob_state_t;

typedef enum
{
  ONESHOT_TIMER = 0,
  PERIODIC_TIMER
}keyfob_timer_type;

/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/
#define KEYFOB_BATTERY_LEVEL_THRESHOLD             60 /* 60% = 2.7V */
#define KEYFOB_VBAT_CHECK_TIMEOUT_MS               1000 /* 1 second */
#define KEYFOB_SLOW_SCAN_TIMEOUT_MS                (5*60*1000) /* 5 minutes */
#define KEYFOB_FAST_SCAN_TIMEOUT_MS                180 /* 180 milliseconds */
#define KEYFOB_CONNECT_TIMEOUT_MS                  5000 /* 5 seconds */

/************************************************************************************
*************************************************************************************
* Private functions prototypes
*************************************************************************************
************************************************************************************/
static void _keyfob_timer_handler(void const *argument);
static TimerHandle_t _keyfob_create_timer(const osa_time_def_t *timer_def, keyfob_timer_type type, void *argument);
static void _keyfob_start_timer(TimerHandle_t timer, uint32_t millisec);
static void _keyfob_stop_timer(TimerHandle_t timer);

static void Led3Blinking();
static void _keyfob_on_init_finished(void);
static void _keyfob_on_walk_detected(void);
static void _keyfob_on_button_activated(void);
static void _keyfob_on_motion_still_detected(void);
static void _keyfob_on_slow_scan_timeout(void);
static void _keyfob_on_ble_connection_request(void);
static void _keyfob_on_motion_still_detected(void);
static void _keyfob_on_fast_scan_timeout(void);
static void _keyfob_on_ble_connection_success(void);
static void _keyfob_on_ble_connection_failure(void);
static void _keyfob_on_ble_disconnected(void);
static void _keyfob_on_no_acc_detected_after_10_min(void);
static void _keyfob_on_enter_freeze(void);
static void _keyfob_on_exit_freeze(void);

static void _keyfob_apply_default_setup(void);
static void _keyfob_start_slow_scan(void);
static void _keyfob_start_fast_scan(void);

/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/
extern volatile bool_t bBleDisconnectionDuringStill;
static uint32_t s_u32KeyfobState;
static bool_t s_bNoAccelerationAfter10Min = FALSE;

static const fsm_entry_t c_tstKeyfobInitState[] =
{
    {KEYFOB_EVENT_INIT_FINISHED,         _keyfob_on_init_finished        },
};

static const fsm_entry_t c_tstKeyfobDeepSleepState[] =
{
    {KEYFOB_EVENT_WALK_DETECTED,         _keyfob_on_walk_detected        },
    {KEYFOB_EVENT_BUTTON_ACTIVATED,      _keyfob_on_button_activated     },
    {KEYFOB_EVENT_ENTER_FREEZE,          _keyfob_on_enter_freeze         },
};

static const fsm_entry_t c_tstKeyfobScanningSlowState[] =
{
    {KEYFOB_EVENT_MOTION_STILL_DETECTED,         _keyfob_on_motion_still_detected              },
    {KEYFOB_EVENT_SLOW_SCAN_TIMEOUT,             _keyfob_on_slow_scan_timeout                  },
    {KEYFOB_EVENT_CONNECTION_REQUEST,            _keyfob_on_ble_connection_request             },
    {KEYFOB_EVENT_ENTER_FREEZE,                  _keyfob_on_enter_freeze                       },
};

static const fsm_entry_t c_tstKeyfobStandbyState[] =
{
    {KEYFOB_EVENT_BUTTON_ACTIVATED,      _keyfob_on_button_activated     },
    {KEYFOB_EVENT_MOTION_STILL_DETECTED, _keyfob_on_motion_still_detected},
    {KEYFOB_EVENT_ENTER_FREEZE,          _keyfob_on_enter_freeze         },
};

static const fsm_entry_t c_tstKeyfobBleConnectionAuthentication[] =
{
    {KEYFOB_EVENT_BLE_CONNECTION_SUCCESS,     _keyfob_on_ble_connection_success       },
    {KEYFOB_EVENT_BLE_CONNECTION_FAILURE,     _keyfob_on_ble_connection_failure       },
    {KEYFOB_EVENT_BLE_CONNECTION_TIMEOUT,     _keyfob_on_ble_connection_failure       },
    {KEYFOB_EVENT_ENTER_FREEZE,               _keyfob_on_enter_freeze                 },
};

static const fsm_entry_t c_tstKeyfobScanningFastState[] =
{
    {KEYFOB_EVENT_FAST_SCAN_TIMEOUT,          _keyfob_on_fast_scan_timeout             },
    {KEYFOB_EVENT_CONNECTION_REQUEST,         _keyfob_on_ble_connection_request        },
    {KEYFOB_EVENT_ENTER_FREEZE,               _keyfob_on_enter_freeze                  },
};

static const fsm_entry_t c_tstKeyfobConnectedState[] =
{
    {KEYFOB_EVENT_BLE_DISCONNECTED,             _keyfob_on_ble_disconnected            },
    {KEYFOB_EVENT_ENTER_FREEZE,                 _keyfob_on_enter_freeze                },
    {KEYFOB_EVENT_NO_ACC_DETECTED_AFTER_10_MIN, _keyfob_on_no_acc_detected_after_10_min},
};

static const fsm_entry_t c_tstKeyfobFreezeState[] =
{
    {KEYFOB_EVENT_EXIT_FREEZE,           _keyfob_on_exit_freeze          },
};

static const fsm_state_t c_tstKeyfobFsmStates[] =
{
    /* KEYFOB_STATE_INIT                          */ FSM_STATE_DEF(c_tstKeyfobInitState),
    /* KEYFOB_STATE_DEEP_SLEEP                    */ FSM_STATE_DEF(c_tstKeyfobDeepSleepState),
    /* KEYFOB_STATE_SCANNING_SLOW                 */ FSM_STATE_DEF(c_tstKeyfobScanningSlowState),
    /* KEYFOB_STATE_STAND_BY                      */ FSM_STATE_DEF(c_tstKeyfobStandbyState),
    /* KEYFOB_STATE_CONNECTION_AUTHENTICATION     */ FSM_STATE_DEF(c_tstKeyfobBleConnectionAuthentication),
    /* KEYFOB_STATE_SCANNING_FAST                 */ FSM_STATE_DEF(c_tstKeyfobScanningFastState),
    /* KEYFOB_STATE_CONNECTED                     */ FSM_STATE_DEF(c_tstKeyfobConnectedState),
    /* KEYFOB_STATE_FREEZE                        */ FSM_STATE_DEF(c_tstKeyfobFreezeState),
};

static const char* c_tszKeyfobStatesLookupTable[] =
{
    /* KEYFOB_STATE_INIT                          */ "INIT",
    /* KEYFOB_STATE_DEEP_SLEEP                    */ "DEEP_SLEEP",
    /* KEYFOB_STATE_SCANNING_SLOW                 */ "SCANNING_SLOW",
    /* KEYFOB_STATE_STAND_BY                      */ "STAND_BY",
	/* KEYFOB_STATE_CONNECTION_AUTHENTICATION     */ "CONNECTION_AUTHENTICATION",
    /* KEYFOB_STATE_SCANNING_FAST                 */ "SCANNING_FAST",
    /* KEYFOB_STATE_CONNECTED                     */ "CONNECTED",
    /* KEYFOB_STATE_FREEZE                        */ "FREEZE",
};

static const char* c_tszKeyfobEventsLookupTable[] =
{
   /* KEYFOB_EVENT_INIT_FINISHED                    */    "INIT_FINISHED",
   /* KEYFOB_EVENT_WALK_DETECTED                    */    "WALK_DETECTED",
   /* KEYFOB_EVENT_MOTION_STILL_DETECTED            */    "MOTION_STILL_DETECTED",
   /* KEYFOB_EVENT_BUTTON_ACTIVATED                 */    "BUTTON_ACTIVATED",
   /* KEYFOB_EVENT_SLOW_SCAN_TIMEOUT                */    "SLOW_SCAN_TIMEOUT",
   /* KEYFOB_EVENT_CONNECTION_REQUEST               */    "CONNECTION_REQUEST",
   /* KEYFOB_EVENT_FAST_SCAN_TIMEOUT                */    "FAST_SCAN_TIMEOUT",
   /* KEYFOB_EVENT_NO_ACC_DETECTED_AFTER_10_MIN     */    "NO_ACC_DETECTED_AFTER_10_MINUTES",
   /* KEYFOB_EVENT_BLE_CONNECTION_SUCCESS           */    "BLE_CONNECTION_SUCCESS",
   /* KEYFOB_EVENT_BLE_CONNECTION_FAILURE           */    "BLE_CONNECTION_FAILURE",
   /* KEYFOB_EVENT_BLE_CONNECTION_TIMEOUT           */    "BLE_CONNECTION_TIMEOUT",
   /* KEYFOB_EVENT_BLE_DISCONNECTED                 */    "BLE_DISCONNECTED",
   /* KEYFOB_EVENT_ENTER_FREEZE                     */    "ENTER_FREEZE",
   /* KEYFOB_EVENT_EXIT_FREEZE                      */    "EXIT_FREEZE",
   "BMA400 INT1 DETECTED",
};

FSM_DEF(keyfob, s_u32KeyfobState, c_tstKeyfobFsmStates);
OSA_MSGQ_HANDLE_DEFINE(s_KeyfobQueueHandle, 0, 0);
static TimerHandle_t s_KeyfobTimerHandle;
OSA_TIMER_DEF(keyfob, _keyfob_timer_handler);
uint8_t first_entry = 0;

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/


/*! *********************************************************************************
 * \brief  This is the keyfob Init module to Sleep
********************************************************************************** */
void _keyfob_go_to_sleep(void)
{
	if(first_entry == 0)
	{
		first_entry = 1;
		OSA_DisableIRQGlobal();
		LPSPI0_DeinitPins();
		//BOARD_UnInitUWBRangingLed(gBoardLedUWBHdl);
		BOARD_UnInitSoftOnLed(gBoardLedSoftOnHdl);
		if(s_u32KeyfobState == KEYFOB_STATE_DEEP_SLEEP)
		{
#ifdef BMW_KEYFOB_EVK_BOARD
    /* No Pin init required */
#else
			BOARD_UnInitMsInterrupt2Pin((hal_gpio_handle_t)g_GpioHandle[1]);
#endif
		}
		BOARD_UnInitMsWalkDetectedLed(gBoardLedMsWdHdl);
		BOARD_UnInitMsStillDetectedLed(gBoardLedMsWdHdl);
		BOARD_UnInitPinButton2();
		BOARD_UnInitMonochromeLed(gBoardLedMonochromeHdl);
		BOARD_UnInitRgbLed(gBoardLedRgbHdl);
		OSA_EnableIRQGlobal();
	}

}

/*! *********************************************************************************
 * \brief  This is the keyfob Init module to wake up when MS interrupt or Button press
********************************************************************************** */
void KEYFOB_wake_up(void)
{
	if(first_entry == 1)
	{
		first_entry = 0;
		OSA_EnableIRQGlobal();
		LPSPI0_InitPins();
		//BOARD_InitUWBRangingLed(gBoardLedUWBHdl);
		BOARD_InitSoftOnLed(gBoardLedSoftOnHdl);
		if(s_u32KeyfobState == KEYFOB_STATE_DEEP_SLEEP)
		{
#ifdef BMW_KEYFOB_EVK_BOARD
    /* No Pin init required */
#else
			BOARD_InitMsInterrupt2Pin((hal_gpio_handle_t)g_GpioHandle[1]);
#endif
		}
		BOARD_InitMsWalkDetectedLed(gBoardLedMsWdHdl);
		BOARD_InitMsStillDetectedLed(gBoardLedMsWdHdl);
		//BOARD_InitButton2((button_handle_t)g_buttonHandle[2]);
		BOARD_InitMonochromeLed(gBoardLedMonochromeHdl);

		BOARD_InitRgbLed(gBoardLedRgbHdl);

	}
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager that implements the system state machine
          according to VALEO specifications.
********************************************************************************** */
void KEYFOB_MGR_init(void)
{
    OSA_MsgQCreate((osa_msgq_handle_t)s_KeyfobQueueHandle, 10U, sizeof(uint32_t));
    s_KeyfobTimerHandle = _keyfob_create_timer(OSA_TIMER(keyfob), ONESHOT_TIMER, NULL);
    fsm_init(FSM_ID(keyfob), KEYFOB_STATE_INIT);
    KEYFOB_MGR_notify(KEYFOB_EVENT_INIT_FINISHED);
}

void KEYFOB_MGR_run(void)
{
    osa_status_t eStatus;
    uint32_t u32Event;
    int iRet;

    while(TRUE)
    {
        eStatus = OSA_MsgQGet(s_KeyfobQueueHandle, (osa_msg_handle_t) &u32Event, osaWaitForever_c);
        if(KOSA_StatusSuccess == eStatus)
        {
            TRACE_INFO("------------------------------------------------");
            if(s_u32KeyfobState < KEYFOB_STATE_MAX)
            {
                TRACE_DEBUG("Keyfob initial State: %s", c_tszKeyfobStatesLookupTable[s_u32KeyfobState]);
            }
            if(u32Event < KEYFOB_EVENT_MAX)
            {
                TRACE_DEBUG("Keyfob received Event: %s", c_tszKeyfobEventsLookupTable[u32Event]);
            }
            TRACE_INFO("Actions:");
            iRet = fsm_process(FSM_ID(keyfob), u32Event);
            if(iRet < 0)
            {
                TRACE_WARNING("No action for this event within this state!");
            }
            if(s_u32KeyfobState < KEYFOB_STATE_MAX)
            {
                TRACE_DEBUG("Keyfob current State: %s", c_tszKeyfobStatesLookupTable[s_u32KeyfobState]);
            }
            TRACE_INFO("------------------------------------------------");
        }
    }
}

/*! *********************************************************************************
 * \brief  Notify the keyfob manager about a new event.
 *
 * \param[in]    u32Event       New event to be sent
********************************************************************************** */
void KEYFOB_MGR_notify(uint32_t u32Event)
{
    OSA_MsgQPut(s_KeyfobQueueHandle, (osa_msg_handle_t) &u32Event);
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
 * \brief  Create a software timer for keyfob manager.
 *
 * \param[in]    timer       Timer handle
 * \param[in]    type        Timer type
 * \param[in]    argument    Timer argument
********************************************************************************** */
static TimerHandle_t _keyfob_create_timer(const osa_time_def_t *timer, keyfob_timer_type type, void *argument)
{
    return xTimerCreate((const char *)"",
                        1, /* period should be filled when starting the Timer using osTimerStart */
                        (type == PERIODIC_TIMER),
                        (void *) argument,
                        (TimerCallbackFunction_t)timer->pfCallback);
}

/*! *********************************************************************************
 * \brief  Start a software timer for keyfob manager.
 *
 * \param[in]    timer       Timer handle
 * \param[in]    millisec    Timer period
********************************************************************************** */
static void _keyfob_start_timer(TimerHandle_t timer, uint32_t millisec)
{
    TickType_t ticks = millisec / portTICK_PERIOD_MS;

    if (ticks == 0)
    {
        ticks = 1;
    }
    xTimerChangePeriod(timer, ticks, 0);
}

/*! *********************************************************************************
 * \brief  Stop a software timer for keyfob manager.
 *
 * \param[in]    timer       Timer handle
********************************************************************************** */
static void _keyfob_stop_timer(TimerHandle_t timer)
{
    xTimerStop(timer, 0);
}

/*! *********************************************************************************
 * \brief  This is the keyfob timer handler.
********************************************************************************** */
static void _keyfob_timer_handler(void const *argument)
{
    (void) argument;

    switch(s_u32KeyfobState)
    {
    case KEYFOB_STATE_INIT:
      KEYFOB_MGR_notify(KEYFOB_EVENT_INIT_FINISHED);
      break;
    case KEYFOB_STATE_SCANNING_SLOW:
      KEYFOB_MGR_notify(KEYFOB_EVENT_SLOW_SCAN_TIMEOUT);
      break;
    case KEYFOB_STATE_SCANNING_FAST:
      KEYFOB_MGR_notify(KEYFOB_EVENT_FAST_SCAN_TIMEOUT);
      break;
    case KEYFOB_STATE_BLE_CONNECTION_AUTHENTICATION:
      KEYFOB_MGR_notify(KEYFOB_EVENT_BLE_CONNECTION_TIMEOUT);
      break;
    default:
      /* Do nothing */
      break;
    }
}

static uint32_t _keyfob_read_vbat(void)
{
    uint32_t u32BatteryLevel;

    u32BatteryLevel = SENSORS_GetBatteryLevel();
    TRACE_DEBUG("Battery level = %0.1fV", conversion_percent_to_voltage(u32BatteryLevel));
    return u32BatteryLevel;
}

static void _keyfob_apply_default_setup(void)
{
    /* TODO: Enable motion sensor */
    TRACE_INFO("Enable Motion Sensor.");
    /* TODO: Turn BLE OFF */
    TRACE_INFO("Turn BLE OFF.");
    Gap_StopScanning();
    /* TODO: Turn UWB OFF */
    TRACE_INFO("Turn UWB OFF.");
    /* TODO: Put MCU into DPD1 mode */
    TRACE_INFO("Put MCU into DPD1 mode.");
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d == 0))
    _keyfob_go_to_sleep();
#endif
}

static void _keyfob_start_slow_scan(void)
{
    systemParameters_t *pstSysParams = NULL;

    App_NvmReadSystemParams(&pstSysParams);
    BleApp_Start(gGapCentral_c,
                 gScanParamConversionMsToGapUnit((pstSysParams->system_params).fields.slow_scan_interval),
                 gScanParamConversionMsToGapUnit((pstSysParams->system_params).fields.slow_scan_window));
}

static void _keyfob_start_fast_scan(void)
{
    systemParameters_t *pstSysParams = NULL;

    App_NvmReadSystemParams(&pstSysParams);
    BleApp_Start(gGapCentral_c,
                 gScanParamConversionMsToGapUnit((pstSysParams->system_params).fields.fast_scan_interval),
                 gScanParamConversionMsToGapUnit((pstSysParams->system_params).fields.fast_scan_window));
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: INIT_FINISHED.
********************************************************************************** */
static void _keyfob_on_init_finished(void)
{
    uint32_t u32BatteryLevel;

    u32BatteryLevel = _keyfob_read_vbat();
    if(u32BatteryLevel >= KEYFOB_BATTERY_LEVEL_THRESHOLD)
    {
        TRACE_INFO("Battery level is above threshold. Initialize the system.");
        _keyfob_apply_default_setup();
        s_u32KeyfobState = KEYFOB_STATE_DEEP_SLEEP;
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d > 0))
        Led3On();
#endif
    }
    else
    {
        TRACE_WARNING("Battery level is under threshold! Start a timer to re-check the VBat.");
        _keyfob_start_timer(s_KeyfobTimerHandle, KEYFOB_VBAT_CHECK_TIMEOUT_MS);
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d > 0))
        Led3Blinking();
#endif
    }
}
/*! *********************************************************************************
 * \brief  Blinking the Soft_on led.
********************************************************************************** */
static void Led3Blinking()
{
    Led3On();
    for(int u32DelayLoop = 0; u32DelayLoop < 400000; u32DelayLoop++)
    {
        __asm volatile ("nop");
    }
    Led3Off();
    for(int u32DelayLoop = 0; u32DelayLoop < 400000; u32DelayLoop++)
    {
        __asm volatile ("nop");
    }
}
/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: WALK_DETECTED.
********************************************************************************** */
static void _keyfob_on_walk_detected(void)
{
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d > 0))
    if((s_u32KeyfobState == KEYFOB_STATE_DEEP_SLEEP)||(s_u32KeyfobState == KEYFOB_STATE_STAND_BY))
    {
        KEYFOB_wake_up();
    }
#endif
    TRACE_INFO("Start BLE slow scan.");
    _keyfob_start_slow_scan();
    TRACE_INFO("Restart the BLE scan timer.");
    _keyfob_start_timer(s_KeyfobTimerHandle, KEYFOB_SLOW_SCAN_TIMEOUT_MS);
    s_u32KeyfobState = KEYFOB_STATE_SCANNING_SLOW;
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: BUTTON_ACTIVATED.
********************************************************************************** */
static void _keyfob_on_button_activated(void)
{
#if (defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d > 0))
    if((s_u32KeyfobState == KEYFOB_STATE_DEEP_SLEEP)||(s_u32KeyfobState == KEYFOB_STATE_STAND_BY))
    {
        KEYFOB_wake_up();
    }
#endif
    TRACE_INFO("Start BLE fast scan.");
    _keyfob_start_fast_scan();
    TRACE_INFO("Restart the BLE scan timer.");
    _keyfob_start_timer(s_KeyfobTimerHandle, KEYFOB_FAST_SCAN_TIMEOUT_MS);
    s_u32KeyfobState = KEYFOB_STATE_SCANNING_FAST;
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: MOTION_STILL_DETECTED.
********************************************************************************** */
static void _keyfob_on_motion_still_detected(void)
{
  if(KEYFOB_STATE_SCANNING_SLOW == s_u32KeyfobState)
  {
      TRACE_INFO("Stop the BLE scan timer.");
      _keyfob_stop_timer(s_KeyfobTimerHandle);
      TRACE_INFO("Re-initialize the system.");
      _keyfob_apply_default_setup();
      s_u32KeyfobState = KEYFOB_STATE_DEEP_SLEEP;
  }
  else if(KEYFOB_STATE_STAND_BY == s_u32KeyfobState)
  {
      TRACE_INFO("Start BLE fast scan.");
      _keyfob_start_slow_scan();
      TRACE_INFO("Restart the BLE scan timer.");
      _keyfob_start_timer(s_KeyfobTimerHandle, KEYFOB_FAST_SCAN_TIMEOUT_MS);
      s_u32KeyfobState = KEYFOB_STATE_SCANNING_FAST;
  }
  else
  {
    /* Do nothing */
  }
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: SLOW_SCAN_TIMEOUT.
********************************************************************************** */
static void _keyfob_on_slow_scan_timeout(void)
{
    TRACE_INFO("BLE scan timeout detected. Re-initialize the system.");
    _keyfob_apply_default_setup();
    s_u32KeyfobState = KEYFOB_STATE_STAND_BY;
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: connection_request.
********************************************************************************** */
static void _keyfob_on_ble_connection_request(void)
{
    TRACE_INFO("Stop the BLE scan timer.");
    _keyfob_stop_timer(s_KeyfobTimerHandle);
    BleApp_Connect();
    _keyfob_start_timer(s_KeyfobTimerHandle, KEYFOB_CONNECT_TIMEOUT_MS);
    s_u32KeyfobState = KEYFOB_STATE_BLE_CONNECTION_AUTHENTICATION;
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: connection_success.
********************************************************************************** */
static void _keyfob_on_ble_connection_success(void)
{
    s_u32KeyfobState = KEYFOB_STATE_CONNECTED;
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: connection_failure.
********************************************************************************** */
static void _keyfob_on_ble_connection_failure(void)
{
    TRACE_INFO("Start BLE fast scan.");
    _keyfob_start_fast_scan();
    TRACE_INFO("Restart the BLE scan timer.");
    _keyfob_start_timer(s_KeyfobTimerHandle, KEYFOB_FAST_SCAN_TIMEOUT_MS);
    s_u32KeyfobState = KEYFOB_STATE_SCANNING_FAST;
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: FAST_SCAN_TIMEOUT.
********************************************************************************** */
static void _keyfob_on_fast_scan_timeout(void)
{
    TRACE_INFO("Stop the BLE scan timer.");
    _keyfob_stop_timer(s_KeyfobTimerHandle);
    TRACE_INFO("Re-initialize the system.");
    _keyfob_apply_default_setup();
    s_u32KeyfobState = KEYFOB_STATE_DEEP_SLEEP;
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: BLE_DISCONNECTED.
********************************************************************************** */
static void _keyfob_on_ble_disconnected(void)
{
    //if(s_bNoAccelerationAfter10Min == FALSE)
    if(bBleDisconnectionDuringStill == FALSE)
    {
        TRACE_INFO("Start BLE fast scan.");
        _keyfob_start_fast_scan();
        TRACE_INFO("Restart the BLE scan timer.");
        _keyfob_start_timer(s_KeyfobTimerHandle, KEYFOB_FAST_SCAN_TIMEOUT_MS);
        s_u32KeyfobState = KEYFOB_STATE_SCANNING_FAST;
    }
    else
    {
        s_bNoAccelerationAfter10Min = FALSE;
        TRACE_INFO("Re-initialize the system.");
        _keyfob_apply_default_setup();
        s_u32KeyfobState = KEYFOB_STATE_DEEP_SLEEP;
    }
    UWB_MGR_notify(UWB_EVENT_BLE_DISCONNECTED);
}

/*! ***********************************************************************************
 * \brief  This is the keyfob manager event handler: KEYFOB_EVENT_NO_ACC_DETECTED_AFTER_10_MIN.
************************************************************************************* */
static void _keyfob_on_no_acc_detected_after_10_min(void)
{
    gVehicleState_t eVehicleState;
    gUWBState_t eUwbState;

    eVehicleState = GetVehicleState();
    eUwbState = GetUWBState();
    TRACE_DEBUG("Vehicle is %s.", (gStatusLocked_c == eVehicleState)?"locked":"unlocked");
    TRACE_DEBUG("UWB ranging is %s.", (gUWBNoRanging_c == eUwbState)?"inactive":"active");
    if((gStatusLocked_c == eVehicleState) && (gUWBNoRanging_c == eUwbState))
    {
        TRACE_INFO("BLE disconnect.");
        (void)Gap_Disconnect(mCurrentPeerId);
        s_bNoAccelerationAfter10Min = TRUE;
    }
    else
    {
        s_bNoAccelerationAfter10Min = FALSE;
        TRACE_INFO("Nothing to do.");
    }
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: KEYFOB_EVENT_ENTER_FREEZE.
********************************************************************************** */
static void _keyfob_on_enter_freeze(void)
{
    TRACE_INFO("Stop the BLE scan timer.");
    _keyfob_stop_timer(s_KeyfobTimerHandle);
    s_u32KeyfobState = KEYFOB_STATE_FREEZE;
}

/*! *********************************************************************************
 * \brief  This is the keyfob manager event handler: KEYFOB_EVENT_EXIT_FREEZE.
********************************************************************************** */
static void _keyfob_on_exit_freeze(void)
{
    TRACE_INFO("Re-initialize the system.");
    _keyfob_apply_default_setup();
    s_u32KeyfobState = KEYFOB_STATE_DEEP_SLEEP;
}


/*! *********************************************************************************
* @}
********************************************************************************** */
