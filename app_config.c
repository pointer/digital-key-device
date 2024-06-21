/*! *********************************************************************************
* \addtogroup App Config
* @{
********************************************************************************** */
/*! *********************************************************************************
* \file app_config.c
*
* Copyright 2020-2023 NXP
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
#include "gap_interface.h"
#include "ble_constants.h"
#include "ble_conn_manager.h"
#include "digital_key_device.h"
#include "gatt_db_handles.h"

/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/
#define smpEdiv                 0x1F99
#define mcEncryptionKeySize_c   16
#define gUseCustomIRK_d                 1

/************************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
************************************************************************************/

/* Default Advertising Parameters. Values can be changed at runtime
    to align with profile requirements */
gapAdvertisingParameters_t gAdvParams = {
    /* minInterval */         gGapAdvertisingIntervalRangeMinimum_c,
    /* maxInterval */         gGapAdvertisingIntervalRangeMinimum_c,
    /* advertisingType */     gAdvConnectableUndirected_c,
    /* addressType */         gBleAddrTypePublic_c,
    /* directedAddressType */ gBleAddrTypePublic_c,
    /* directedAddress */     {0, 0, 0, 0, 0, 0},
    /* channelMap */          (gapAdvertisingChannelMapFlags_t) (gGapAdvertisingChannelMapDefault_c),
    /* filterPolicy */        gProcessAll_c
};

/* Scanning and Advertising Data */
static uint8_t adData0[1] =  { (gapAdTypeFlags_t)(gLeGeneralDiscoverableMode_c | gBrEdrNotSupported_c) };
static gapAdStructure_t advScanStruct[3] = {
  {
    .length = NumberOfElements(adData0) + 1,
    .adType = gAdFlags_c,
    .aData = (uint8_t *)adData0
  },
  {
    .length = NumberOfElements(uuid_service_BLE_params) + 1,
    .adType = gAdComplete128bitServiceList_c,
    .aData = (uint8_t *)uuid_service_BLE_params
  },
  {
    .adType = gAdShortenedLocalName_c,
    .length = 7,
    .aData = (uint8_t*)"BMW_DK"
  }
};

gapAdvertisingData_t gAppAdvertisingData =
{
    NumberOfElements(advScanStruct),
    (void *)advScanStruct
};

gapScanResponseData_t gAppScanRspData =
{
    0,
    NULL
};

gapScanningParameters_t gScanParams =
{
    /* type */              gScanTypePassive_c,
    /* interval */          gcScanIntervalCCC_c,
    /* window */            gcScanWindowCCC_c,
    /* ownAddressType */    gBleAddrTypePublic_c,
    /* filterPolicy */      (uint8_t)gScanAll_c,
    /* scanning PHY */      (uint8_t)( gLePhy1MFlag_c | gLePhyCodedFlag_c )
};

/* Default Connection Request Parameters */
gapConnectionRequestParameters_t gConnReqParams =
{
    .scanInterval = gcScanIntervalCCC_c,
    .scanWindow = gcScanWindowCCC_c,
    .filterPolicy = (uint8_t)gUseDeviceAddress_c,
    .ownAddressType = gBleAddrTypePublic_c,
    .connIntervalMin = gcConnectionIntervalCCC_c,
    .connIntervalMax = gcConnectionIntervalCCC_c,
    .connLatency = 0,
    .supervisionTimeout = 0x03E8,
    .connEventLengthMin = 0,
    .connEventLengthMax = 0xFFFF,
    .initiatingPHYs = (uint8_t)( gLePhy1MFlag_c | gLePhyCodedFlag_c )
};

/* SMP Data */
gapPairingParameters_t gPairingParameters = {
    .withBonding = (bool_t)gAppUseBonding_d,
    .securityModeAndLevel = gSecurityMode_1_Level_4_c,
    .maxEncryptionKeySize = mcEncryptionKeySize_c,
    .localIoCapabilities = gIoNone_c,
    .oobAvailable = TRUE,
    .centralKeys = (gapSmpKeyFlags_t) (gIrk_c),
    .peripheralKeys = (gapSmpKeyFlags_t) (gIrk_c),
    .leSecureConnectionSupported = TRUE,
    .useKeypressNotifications = FALSE,
};

/* LTK */
/*static uint8_t smpLtk[gcSmpMaxLtkSize_c] =
    {0xD6, 0x93, 0xE8, 0xA4, 0x23, 0x55, 0x48, 0x99,
     0x1D, 0x77, 0x61, 0xE6, 0x63, 0x2B, 0x10, 0x8E};*/


/* IRK */
static uint8_t smpIrk[gcSmpIrkSize_c] =
    {0x0A, 0x2D, 0xF4, 0x65, 0xE3, 0xBD, 0x7B, 0x49,
     0x1E, 0xB4, 0xC0, 0x95, 0x95, 0x13, 0x46, 0x73};

/* BMW LTK */
static uint8_t smpLtk[gcSmpMaxLtkSize_c] =
    {0xA2, 0x25, 0xA4, 0xBD, 0x10, 0x70, 0x9A, 0x1D,
     0x6A, 0x0D, 0xF7, 0x12, 0x40, 0x21, 0x7A, 0x2F};

/* RAND*/
static uint8_t smpRand[gcSmpMaxRandSize_c] =
    {0x26, 0x1E, 0xF6, 0x09, 0x97, 0x2E, 0xAD, 0x7E};

/* BMW IRK */
/*static uint8_t smpIrk[gcSmpIrkSize_c] =
    {0x38, 0x37, 0x52, 0x02, 0x35, 0xFA, 0x17, 0xC6,
     0xA8, 0xC4, 0x57, 0x4C, 0x96, 0xC6, 0x33, 0x75};*/

/* CSRK */
static uint8_t smpCsrk[gcSmpCsrkSize_c] =
    {0x90, 0xD5, 0x06, 0x95, 0x92, 0xED, 0x91, 0xD7,
     0xA8, 0x9E, 0x2C, 0xDC, 0x4A, 0x93, 0x5B, 0xF9};

gapSmpKeys_t gSmpKeys = {
    .cLtkSize = mcEncryptionKeySize_c,
    .aLtk = (void *)smpLtk,
    .aIrk = (void *)smpIrk,
    .aCsrk = (void *)smpCsrk,
    .aRand = (void *)smpRand,
    .cRandSize = gcSmpMaxRandSize_c,
    .ediv = smpEdiv,
};

/* Device Security Requirements */
static gapSecurityRequirements_t        deviceSecurity = gGapDefaultSecurityRequirements_d;

gapDeviceSecurityRequirements_t deviceSecurityRequirements = {
    .pSecurityRequirements          = (void*)&deviceSecurity,
    .cNumServices                   = 0,
    .aServiceSecurityRequirements   = NULL
};
