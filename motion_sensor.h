/*
 * motion_sensor.h
 *
 *  Created on: 9 janv. 2024
 *      Author: yhermi
 */

#ifndef MOTION_SENSOR_H_
#define MOTION_SENSOR_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "fsl_lpspi_cmsis.h"
#include "Driver_Common.h"
#include "Driver_SPI.h"
#include "fsl_adapter_gpio.h"
#include "app.h"

/*******************************************************************************
 * Public macros
 ******************************************************************************/
#define DRIVER_MASTER_SPI         Driver_SPI0


/*******************************************************************************
 * Public functions
 ******************************************************************************/
#ifdef BMW_KEYFOB_EVK_BOARD
/* No functions required */
#else
void MS_Init(void);
int8_t Get_Ms_Temp_Value();
void Update_Ms_register_0x1A(int32_t ms_accuracy_range, int32_t ms_osr, int32_t ms_odr);
void Update_Ms_register_0x41(int32_t ms_threshold_activity_change);
void Update_Ms_register_0x42(int32_t ms_motion_still_duration);
void Ms_wrist_mode_on(void);
void Ms_wrist_mode_off(void);

void Ms_DisableGlobal_IRQ(void);
void Ms_EnableGlobal_IRQ(void);

#if (defined(gAppMsInterruptPinCnt_c) && (gAppMsInterruptPinCnt_c > 0))
void MS_INT_1_HANDLER(void *GpioHandle);
#endif /*gAppMsInterruptPinCnt_c > 0*/
#if (defined(gAppMsInterruptPinCnt_c) && (gAppMsInterruptPinCnt_c > 1))
void MS_INT_2_HANDLER(void *GpioHandle);
#endif /*gAppMsInterruptPinCnt_c > 1*/
#endif

#endif /* MOTION_SENSOR_H_ */
