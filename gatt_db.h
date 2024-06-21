PRIMARY_SERVICE(service_gatt, gBleSig_GenericAttributeProfile_d)
        CHARACTERISTIC(char_service_changed, gBleSig_GattServiceChanged_d, (gGattCharPropIndicate_c) )
            VALUE(value_service_changed, gBleSig_GattServiceChanged_d, (gPermissionNone_c), 4, 0x00, 0x00, 0x00, 0x00)
            CCCD(cccd_service_changed)

PRIMARY_SERVICE(service_gap, gBleSig_GenericAccessProfile_d)
    CHARACTERISTIC(char_device_name, gBleSig_GapDeviceName_d, (gGattCharPropRead_c) )
        VALUE(value_device_name, gBleSig_GapDeviceName_d, (gPermissionFlagReadable_c), 12, "NXP_BLE_TEMP")
    CHARACTERISTIC(char_security_levels, gBleSig_GattSecurityLevels_d, (gGattCharPropRead_c) )
        VALUE(value_security_levels, gBleSig_GattSecurityLevels_d, (gPermissionFlagReadable_c), 2, 0x01, 0x01)

/************************************************************************************
*************************************************************************************
* BLE parameters service and characteristics
*************************************************************************************
************************************************************************************/
PRIMARY_SERVICE_UUID128(service_BLE_params, uuid_service_BLE_params)
    CHARACTERISTIC_UUID128(char_scanning_interval, uuid_char_scanning_interval, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_scanning_interval, uuid_char_scanning_interval, (gPermissionFlagReadable_c | gPermissionFlagWritable_c),gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_scanning_interval, uuid_desc_scanning_interval, (gPermissionFlagReadable_c), sizeof("scanning_interval")-1, "scanning_interval")
    CHARACTERISTIC_UUID128(char_scan_windows, uuid_char_scan_windows, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_scan_windows, uuid_char_scan_windows, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_scan_windows, uuid_desc_scan_windows, (gPermissionFlagReadable_c), sizeof("scan_windows")-1, "scan_windows")
    CHARACTERISTIC_UUID128(char_connection_interval, uuid_char_connection_interval, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_connection_interval, uuid_char_connection_interval, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_connection_interval, uuid_desc_connection_interval, (gPermissionFlagReadable_c), sizeof("connection_interval")-1, "connection_interval")
    CHARACTERISTIC_UUID128(char_ble_power_output, uuid_char_ble_power_output, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_ble_power_output, uuid_char_ble_power_output, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ble_power_output, uuid_desc_ble_power_output, (gPermissionFlagReadable_c), sizeof("ble_power_output")-1, "ble_power_output")
    CHARACTERISTIC_UUID128(char_ble_scan_on_step, uuid_char_ble_scan_on_step, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_ble_scan_on_step, uuid_char_ble_scan_on_step, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ble_scan_on_step, uuid_desc_ble_scan_on_step, (gPermissionFlagReadable_c), sizeof("ble_scan_on_step")-1, "ble_scan_on_step")
    CHARACTERISTIC_UUID128(char_ble_disc_during_still, uuid_char_ble_disc_during_still, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_ble_disc_during_still, uuid_char_ble_disc_during_still, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ble_disc_during_still, uuid_desc_ble_disc_during_still, (gPermissionFlagReadable_c), sizeof("ble_disc_during_still")-1, "ble_disc_during_still")
    CHARACTERISTIC_UUID128(char_delta_rssi_low, uuid_char_delta_rssi_low, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_delta_rssi_low, uuid_char_delta_rssi_low, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_delta_rssi_low, uuid_desc_delta_rssi_low, (gPermissionFlagReadable_c), sizeof("delta_rssi_low")-1, "delta_rssi_low")
    CHARACTERISTIC_UUID128(char_delta_rssi_medium, uuid_char_delta_rssi_medium, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_delta_rssi_medium, uuid_char_delta_rssi_medium, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_delta_rssi_medium, uuid_desc_delta_rssi_medium, (gPermissionFlagReadable_c), sizeof("delta_rssi_medium")-1, "delta_rssi_medium")
    CHARACTERISTIC_UUID128(char_delta_rssi_high, uuid_char_delta_rssi_high, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_delta_rssi_high, uuid_char_delta_rssi_high, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_delta_rssi_high, uuid_desc_delta_rssi_high, (gPermissionFlagReadable_c), sizeof("delta_rssi_high")-1, "delta_rssi_high")
    CHARACTERISTIC_UUID128(char_rssi_intent_high, uuid_char_rssi_intent_high, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_rssi_intent_high, uuid_char_rssi_intent_high, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_rssi_intent_high, uuid_desc_rssi_intent_high, (gPermissionFlagReadable_c), sizeof("rssi_intent_high")-1, "rssi_intent_high")
    CHARACTERISTIC_UUID128(char_timeout_between_same_intents, uuid_char_timeout_between_same_intents, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_timeout_between_same_intents, uuid_char_timeout_between_same_intents, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_timeout_between_same_intents, uuid_desc_timeout_between_same_intents, (gPermissionFlagReadable_c), sizeof("timeout_between_same_intents")-1, "timeout_between_same_intents")
    CHARACTERISTIC_UUID128(char_rssi_on_duration, uuid_char_rssi_on_duration, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_rssi_on_duration, uuid_char_rssi_on_duration, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_rssi_on_duration, uuid_desc_rssi_on_duration, (gPermissionFlagReadable_c), sizeof("rssi_on_duration")-1, "rssi_on_duration")

/************************************************************************************
*************************************************************************************
* Motion sensor parameters service and characteristics
*************************************************************************************
************************************************************************************/
PRIMARY_SERVICE_UUID128(service_MS_params, uuid_service_MS_params)
    CHARACTERISTIC_UUID128(char_ms_threshold_no_motion_detected, uuid_char_ms_threshold_no_motion_detected, (gGattCharPropWrite_c | gGattCharPropRead_c) )
        VALUE_UUID128_VARLEN(value_ms_threshold_no_motion_detected, uuid_char_ms_threshold_no_motion_detected, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_threshold_no_motion_detected, uuid_desc_ms_threshold_no_motion_detected, (gPermissionFlagReadable_c), sizeof("ms_threshold_no_motion_detected")-1, "ms_threshold_no_motion_detected")
    CHARACTERISTIC_UUID128(char_ms_osr, uuid_char_ms_osr, (gGattCharPropWrite_c | gGattCharPropRead_c) )
        VALUE_UUID128_VARLEN(value_ms_osr, uuid_char_ms_osr, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_osr, uuid_desc_ms_osr, (gPermissionFlagReadable_c), sizeof("ms_osr")-1, "ms_osr")
    CHARACTERISTIC_UUID128(char_ms_odr, uuid_char_ms_odr, (gGattCharPropWrite_c | gGattCharPropRead_c) )
        VALUE_UUID128_VARLEN(value_ms_odr, uuid_char_ms_odr, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_odr, uuid_desc_ms_odr, (gPermissionFlagReadable_c), sizeof("ms_odr")-1, "ms_odr")
    CHARACTERISTIC_UUID128(char_ms_accuracy_range, uuid_char_ms_accuracy_range, (gGattCharPropWrite_c | gGattCharPropRead_c) )
        VALUE_UUID128_VARLEN(value_ms_accuracy_range, uuid_char_ms_accuracy_range, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_accuracy_range, uuid_desc_ms_accuracy_range, (gPermissionFlagReadable_c), sizeof("ms_accuracy_range")-1, "ms_accuracy_range")
    CHARACTERISTIC_UUID128(char_ms_motion_still_duration, uuid_char_ms_motion_still_duration, (gGattCharPropWrite_c | gGattCharPropRead_c) )
        VALUE_UUID128_VARLEN(value_ms_motion_still_duration, uuid_char_ms_motion_still_duration, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_motion_still_duration, uuid_desc_ms_motion_still_duration, (gPermissionFlagReadable_c), sizeof("ms_motion_still_duration")-1, "ms_motion_still_duration")
    CHARACTERISTIC_UUID128(char_ms_wrist_mode, uuid_char_ms_wrist_mode, (gGattCharPropWrite_c | gGattCharPropRead_c) )
        VALUE_UUID128_VARLEN(value_ms_wrist_mode, uuid_char_ms_wrist_mode, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_wrist_mode, uuid_desc_ms_wrist_mode, (gPermissionFlagReadable_c), sizeof("ms_wrist_mode")-1, "ms_wrist_mode")
    CHARACTERISTIC_UUID128(char_ms_step_in_scan, uuid_char_ms_step_in_scan, (gGattCharPropRead_c))
        VALUE_UUID128_VARLEN(value_ms_step_in_scan, uuid_char_ms_step_in_scan, (gPermissionFlagReadable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_step_in_scan, uuid_desc_ms_step_in_scan, (gPermissionFlagReadable_c), sizeof("ms_step_in_scan")-1, "ms_step_in_scan")
    CHARACTERISTIC_UUID128(char_ms_step_out_scan, uuid_char_ms_step_out_scan, (gGattCharPropRead_c))
        VALUE_UUID128_VARLEN(value_ms_step_out_scan, uuid_char_ms_step_out_scan, (gPermissionFlagReadable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_step_out_scan, uuid_desc_ms_step_out_scan, (gPermissionFlagReadable_c), sizeof("ms_step_out_scan")-1, "ms_step_out_scan")
    CHARACTERISTIC_UUID128(char_ms_total_step, uuid_char_ms_total_step, (gGattCharPropRead_c))
        VALUE_UUID128_VARLEN(value_ms_total_step, uuid_char_ms_total_step, (gPermissionFlagReadable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_total_step, uuid_desc_ms_total_step, (gPermissionFlagReadable_c), sizeof("ms_total_step")-1, "ms_total_step")
    CHARACTERISTIC_UUID128(char_ms_still_detected, uuid_char_ms_still_detected, (gGattCharPropRead_c))
        VALUE_UUID128_VARLEN(value_ms_still_detected, uuid_char_ms_still_detected, (gPermissionFlagReadable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_ms_still_detected, uuid_desc_ms_still_detected, (gPermissionFlagReadable_c), sizeof("ms_still_detected")-1, "ms_still_detected")
    CHARACTERISTIC_UUID128(char_temperature, uuid_char_temperature, (gGattCharPropRead_c))
        VALUE_UUID128_VARLEN(value_temperature, uuid_char_temperature, (gPermissionFlagReadable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_temperature, uuid_desc_temperature, (gPermissionFlagReadable_c), sizeof("temperature")-1, "temperature")
    CHARACTERISTIC_UUID128(char_battery_level, uuid_char_battery_level, (gGattCharPropRead_c))
        VALUE_UUID128_VARLEN(value_battery_level, uuid_char_battery_level, (gPermissionFlagReadable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_battery_level, uuid_desc_battery_level, (gPermissionFlagReadable_c), sizeof("battery_level")-1, "battery_level")
/************************************************************************************
*************************************************************************************
* Commands service and characteristics
*************************************************************************************
************************************************************************************/
PRIMARY_SERVICE_UUID128(service_commands, uuid_service_commands)
    CHARACTERISTIC_UUID128(char_commands_version, uuid_char_commands_version, (gGattCharPropRead_c) )
        VALUE_UUID128_VARLEN(value_commands_version, uuid_char_commands_version, (gPermissionFlagReadable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_commands_version, uuid_desc_commands_version, (gPermissionFlagReadable_c), sizeof("version")-1, "version")
    CHARACTERISTIC_UUID128(char_commands_reset, uuid_char_commands_reset, (gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_commands_reset, uuid_char_commands_reset, (gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_commands_reset, uuid_desc_commands_reset, (gPermissionFlagReadable_c), sizeof("reset")-1, "reset")

/************************************************************************************
*************************************************************************************
* keys service and characteristics
*************************************************************************************
************************************************************************************/
PRIMARY_SERVICE_UUID128(service_keys, uuid_service_keys)
    CHARACTERISTIC_UUID128(char_keys_IRK, uuid_char_keys_IRK, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_keys_IRK, uuid_char_keys_IRK, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_keys_IRK, uuid_desc_keys_IRK, (gPermissionFlagReadable_c), sizeof("IRK")-1, "IRK")
    CHARACTERISTIC_UUID128(char_keys_LTK, uuid_char_keys_LTK, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_keys_LTK, uuid_char_keys_LTK, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_keys_LTK, uuid_desc_keys_LTK, (gPermissionFlagReadable_c), sizeof("LTK")-1, "LTK")
    CHARACTERISTIC_UUID128(char_BD_ADDR, uuid_char_BD_ADDR, (gGattCharPropRead_c | gGattCharPropWrite_c) )
        VALUE_UUID128_VARLEN(value_BD_ADDR, uuid_char_BD_ADDR, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_BD_ADDR, uuid_desc_BD_ADDR, (gPermissionFlagReadable_c), sizeof("BD_ADDR")-1, "BD_ADDR")

/************************************************************************************
*************************************************************************************
* UWB parameters service and characteristics
*************************************************************************************
************************************************************************************/
PRIMARY_SERVICE_UUID128(service_UWB_params, uuid_service_UWB_params)
    CHARACTERISTIC_UUID128(char_number_of_anchors, uuid_char_number_of_anchors, (gGattCharPropWrite_c | gGattCharPropRead_c) )
        VALUE_UUID128_VARLEN(value_number_of_anchors, uuid_char_number_of_anchors, (gPermissionFlagReadable_c | gPermissionFlagWritable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)
        DESCRIPTOR_UUID128(desc_number_of_anchors, uuid_desc_number_of_anchors, (gPermissionFlagReadable_c), sizeof("number_of_anchors")-1, "number_of_anchors")
