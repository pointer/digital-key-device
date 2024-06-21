/*
 * app_nvm.h
 *
 *  Created on: 19 déc. 2023
 *      Author: yhermi
 */

#ifndef APP_NVM_H_
#define APP_NVM_H_

/*****************************************************************************
 ******************************************************************************
 * Includes
 ******************************************************************************
 ******************************************************************************/

#include "EmbeddedTypes.h"
#include "ble_general.h"

/*****************************************************************************
 ******************************************************************************
 * Macros
 ******************************************************************************
 ******************************************************************************/

#define KEY_MAX_SIZE          16
#define BD_ADDR_MAX_SIZE       6

/*****************************************************************************
 ******************************************************************************
 * Public memory declarations
 ******************************************************************************
 ******************************************************************************/

/* IRK */
static uint8_t IRK_default_value[KEY_MAX_SIZE] = /*{0x0A, 0x2D, 0xF4, 0x65, 0xE3, 0xBD, 0x7B, 0x49,
	     0x1E, 0xB4, 0xC0, 0x95, 0x95, 0x13, 0x46, 0x73};*/
    {0x38, 0x37, 0x52, 0x02, 0x35, 0xFA, 0x17, 0xC6,
     0xA8, 0xC4, 0x57, 0x4C, 0x96, 0xC6, 0x33, 0x75};

/* LTK */
static uint8_t LTK_default_value[KEY_MAX_SIZE] = /*{0xD6, 0x93, 0xE8, 0xA4, 0x23, 0x55, 0x48, 0x99,
	     0x1D, 0x77, 0x61, 0xE6, 0x63, 0x2B, 0x10, 0x8E};*/
    {0xA2, 0x25, 0xA4, 0xBD, 0x10, 0x70, 0x9A, 0x1D,
     0x6A, 0x0D, 0xF7, 0x12, 0x40, 0x21, 0x7A, 0x2F};

/* BD_ADDR */
static uint8_t BD_Addr_default_value[BD_ADDR_MAX_SIZE] =
    {0xBC, 0x09, 0x63, 0xAC, 0xB4, 0xB2};

/*****************************************************************************
 ******************************************************************************
 * Public type declarations
 ******************************************************************************
 ******************************************************************************/

/*! \brief  system parameter tag (rank).
 *
 * Order must be consistent between systemParams_t and  systemParamID_t
 */
typedef enum
{
    SlowScanIntervalID = 0,
    SlowScanWindowID,
    ConnectionIntervalID,
    BlePowerOutputID,
    BleScanOnStepID,
    BleDiscDuringStillID,
    DeltaRssiLowID,
    DeltaRssiMediumID,
    DeltaRssiHighID,
    RssiIntentHighID,
    TimeoutBetweenSameIntentsID,
    RssiOnDurationID,
    MsThresholdNoMotionDetectionID,
    MsOsrID,
    MsOdrID,
    MsAccuracyRangeID,
    MsMotionStillDurationID,
    MsWristModeID,
	MsStepInScanID,
	MsStepOutScanID,
	MsTotalStepID,
	MsStillDetectedID,
	TemperatureID,
	BatteryLevelID,
    NumberOfAnchorsID,
    FastScanIntervalID,
    FastScanWindowID,
    ParamMaxID,
}systemParamID_t;

/*! \brief  ble keys tag (rank).
 *
 * Order must be consistent between bleKeys_t and  bleKeysID_t
 */
typedef enum
{
    IrkKeyID = 0,
    LtkKeyID,
    BdAddrKeyID,
    KeyMaxID,
}bleKeysID_t;

/*! \brief  system parameter item structure content.
 *
 * Data structure containing system parameter item
 */
typedef struct systemItem_tag
{
    systemParamID_t eID;
    const char * name;
    int32_t default_value;
    int32_t min_value;
    int32_t max_value;
}systemItem_t;

/*! \brief  ble key item structure content.
 *
 * Data structure containing ble key item
 */
typedef struct keysItem_tag
{
    bleKeysID_t eID;
    const char * name;
    uint8_t *default_key_bytes;
    uint8_t max_size;
}keysItem_t;

/*! \brief  system parameter structure content.
 *
 * Data structure containing system parameters
 */
typedef PACKED_STRUCT systemParams_tag
{
    uint32_t slow_scan_interval;
    uint32_t slow_scan_window;
    uint32_t connection_interval;
    uint32_t ble_power_output;
    uint32_t ble_scan_on_step;
    uint32_t ble_disc_during_still;
    uint32_t delta_rssi_low;
    uint32_t delta_rssi_medium;
    uint32_t delta_rssi_high;
    int32_t rssi_intent_high;
    uint32_t timeout_between_same_intents;
    uint32_t rssi_on_duration;
    uint32_t ms_threshold_no_motion_detected;
    uint32_t ms_osr;
    uint32_t ms_odr;
    uint32_t ms_accuracy_range;
    uint32_t ms_motion_still_duration;
    uint32_t ms_wrist_mode;
    uint32_t ms_step_in_scan;
    uint32_t ms_step_out_scan;
    uint32_t ms_total_step;
    uint32_t ms_still_detected;
    int32_t temperature;
    uint32_t battery_voltage;
    uint32_t number_of_anchors;
    uint32_t fast_scan_interval;
    uint32_t fast_scan_window;
}systemParams_t;

/*! \brief  ble key structure content.
 *
 * Data structure containing ble keys
 */
typedef PACKED_STRUCT bleKeys_tag
{
    uint8_t IRK[KEY_MAX_SIZE];
    uint8_t LTK[KEY_MAX_SIZE];
    uint8_t BD_ADDR[BD_ADDR_MAX_SIZE];
}bleKeys_t;

/*****************************************************************************
 ******************************************************************************
 * Public memory declarations
 ******************************************************************************
 ******************************************************************************/
static const systemItem_t SystemParamsRegistry[] =
{
    /* ID                             name                           default     min    max */
    {SlowScanIntervalID,             "scanning_interval",              1468,    100,   3000},
    {SlowScanWindowID,               "scan_windows",                     52,     40,    100},
    {ConnectionIntervalID,           "connection_interval",              30,      7,   3000},
    {BlePowerOutputID,               "ble_power_output",                  0,      0,     10},
    {BleScanOnStepID,                "ble_scan_on_step",                  3,      0,      5},
    {BleDiscDuringStillID,           "ble_disc_during_still",             5,      0,     60},
    {DeltaRssiLowID,                 "delta_rssi_low",                    6,      0,     40},
    {DeltaRssiMediumID,              "delta_rssi_medium",                12,      0,     40},
    {DeltaRssiHighID,                "delta_rssi_high",                  18,      0,     40},
    {RssiIntentHighID,               "rssi_intent_high",                -50,    -90,    -30},
    {TimeoutBetweenSameIntentsID,    "timeout_between_same_intents",      5,      0,     60},
    {RssiOnDurationID,               "rssi_on_duration",                300,      0,    300},
    {MsThresholdNoMotionDetectionID, "ms_threshold_no_motion_detected",   8,      0,    255},
    {MsOsrID,                        "ms_osr",                            3,      0,      3},
    {MsOdrID,                        "ms_odr",                            8,      0,     15},
    {MsAccuracyRangeID,              "ms_accuracy_range",                 0,      0,      3},
    {MsMotionStillDurationID,        "ms_motion_still_duration",          0,      0,    255},
    {MsWristModeID,                  "ms_wrist_mode",                     0,      0,      1},
    {MsStepInScanID,                 "ms_step_in_scan",                   0,      0, 200000},
    {MsStepOutScanID,                "ms_step_out_scan",                  0,      0, 200000},
    {MsTotalStepID,                  "ms_total_step",                     0,      0, 200000},
    {MsStillDetectedID,              "ms_still_detected",                 0,      0, 200000},
    {TemperatureID,                  "temperature",                       0,    -40,     80},// new
    {BatteryLevelID,                 "battery_level",                     0,      0,    100},// new
    {NumberOfAnchorsID,              "number_of_anchors",                 6,      1,     10},
    {FastScanIntervalID,             "fast_scan_interval",               60,     50,   3000},
    {FastScanWindowID,               "fast_scan_window",                 57,     40,    100},
};

static const keysItem_t BleKeysRegistry[] =
{
    /* ID                 name                  default                                max size */
    {IrkKeyID,           "IRK_Key",            (void*)IRK_default_value,              KEY_MAX_SIZE},
    {LtkKeyID,           "LTK_Key",            (void*)LTK_default_value,              KEY_MAX_SIZE},
	{BdAddrKeyID,        "BD_Addr",            (void*)BD_Addr_default_value,          BD_ADDR_MAX_SIZE},
};

/*****************************************************************************
 ******************************************************************************
 * Public macros
 ******************************************************************************
 ******************************************************************************/

typedef struct systemParameters_tag
{
    union                             /*! Custom settings for BMW Keyfob PoC */
    {
        systemParams_t fields;
        uint32_t buffer[ParamMaxID];
    }system_params;
}systemParameters_t;

typedef struct
{
    uint8_t key[KEY_MAX_SIZE];
}bleIrkLtkKeys_t;

typedef struct IrkLtkKeys_tag
{
    union
    {
    	bleKeys_t vkeys;
    	bleIrkLtkKeys_t keys[KeyMaxID];
    }ble_keys;
}IrkLtkKeys_t;

/*****************************************************************************
 ******************************************************************************
 * Public functions
 ******************************************************************************
 ******************************************************************************/
bleResult_t App_NvmLoadBleKeys(void);
bleResult_t App_NvmReadBleKeys(IrkLtkKeys_t **pBleKeys);
bleResult_t App_NvmWriteBleKey(bleKeysID_t id, uint8_t *key_bytes, uint16_t key_size);
bleResult_t App_NvmLoadSystemParams(void);
bleResult_t App_NvmReadSystemParams(systemParameters_t **pSysParams);
bleResult_t App_NvmWriteSystemParam(systemParamID_t id, int32_t value);

#endif /* APP_NVM_H_ */
