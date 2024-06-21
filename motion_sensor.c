/*
 * motion_sensor.c
 *
 *  Created on: 9 janv. 2024
 *      Author: yhermi
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "motion_sensor.h"
#include "fsl_gpio.h"
#include "keyfob_manager.h"
#include "stdio.h"
#include "trace.h"
#include "app_nvm.h"
#include "timers.h"
#include "gap_interface.h"
#include "app_digital_key_device.h"
#include "fsl_os_abstraction.h"
#include "gap_types.h"

/*******************************************************************************
 * Private macros
 ******************************************************************************/
#define TRANSFER_SIZE          10U     /*! Transfer dataSize */
#define TRANSFER_SIZE_WRITE     2U     /*! Transfer dataSize */
#define TRANSFER_BAUDRATE 500000U /*! Transfer baudrate - 500k */
#define EXAMPLE_LPSPI_MASTER_CLOCK_SOURCE (kCLOCK_IpSrcFro192M)

volatile bool isTransferCompleted  = false;
volatile bool isMasterOnTransmit   = false;
volatile bool isMasterOnReceive    = false;

/*******************************************************************************
 * Private type definitions
 ******************************************************************************/
typedef enum
{
    ONESHOT_TIMER = 0,
    PERIODIC_TIMER
}ms_timer_type;

/*******************************************************************************
 * Private functions prototypes
 ******************************************************************************/
static void Write_register(uint8_t register_addr, uint8_t value_to_write);
static uint8_t Read_register(uint8_t register_addr);

static void _ms_timer_handler(void const *argument);
static TimerHandle_t _ms_create_timer(const osa_time_def_t *timer_def, ms_timer_type type, void *argument);
static void _ms_start_timer(TimerHandle_t timer, uint32_t millisec);
static void _ms_stop_timer(TimerHandle_t timer);

/*******************************************************************************
 * Public memory declarations
 ******************************************************************************/
uint8_t masterRxData[TRANSFER_SIZE] = {0U};
uint8_t masterTxData[TRANSFER_SIZE] = {0U};
bool_t in_motion = false ; /* True if we receive a walk event from sensor */

uint32_t step_without_scan = 0;
uint32_t total_step_count = 0;
uint32_t step_while_scan = 0;
uint32_t still_detected_count = 0;
uint8_t anti_rebound_int = 0;
bool_t firstint1 = 0;
bool_t firstint2 = 0;
uint32_t step = 0;

extern volatile bool isTransferCompleted;
extern volatile gapRole_t mGapRole;
bool_t bBleDisconnectionDuringStill = false;
uint32_t u32DelayLoop ;
uint32_t u32DelayLoop_Max = 400000;

static void LPSPI_MasterSignalEvent_t(uint32_t event)
{
    if (true == isMasterOnReceive)
    {
        isMasterOnReceive = false;
    }
    if (true == isMasterOnTransmit)
    {
        isMasterOnTransmit = false;
    }
    isTransferCompleted = true;
}

static void LPSPI_Init()
{
    /* Set clock source for LPSPI slave and get the clock source */
    CLOCK_SetIpSrc(EXAMPLE_LPSPI_MASTER_CLOCK_NAME, EXAMPLE_LPSPI_MASTER_CLOCK_SOURCE);
    CLOCK_SetIpSrcDiv(EXAMPLE_LPSPI_MASTER_CLOCK_NAME, kSCG_SysClkDivBy16);

    /*LPSPI master init*/
    DRIVER_MASTER_SPI.Initialize(LPSPI_MasterSignalEvent_t);
    DRIVER_MASTER_SPI.PowerControl(ARM_POWER_FULL);
    DRIVER_MASTER_SPI.Control(ARM_SPI_MODE_MASTER, TRANSFER_BAUDRATE);
}

static TimerHandle_t s_MsTimerHandle;
OSA_TIMER_DEF(ms, _ms_timer_handler);

/*******************************************************************************
 * Public functions definitions
 ******************************************************************************/
#ifdef BMW_KEYFOB_EVK_BOARD
    /* No functions required */
#else
void Blink_led_ms()
{
    Led4On();
    for(u32DelayLoop = 0; u32DelayLoop < u32DelayLoop_Max; u32DelayLoop++)
    {
        __asm volatile ("nop");
    }
    Led4Off();
    /*for(u32DelayLoop = 0; u32DelayLoop < u32DelayLoop_Max; u32DelayLoop++)
    {
        __asm volatile ("nop");
    }*/
}

void MS_Init(void)
{
    systemParameters_t *pSysParams = NULL;
    App_NvmReadSystemParams(&pSysParams);

    s_MsTimerHandle = _ms_create_timer(OSA_TIMER(ms), ONESHOT_TIMER, NULL);

    /* LPSPI0 initialisation */
    LPSPI0_InitPins();
    /* LPSPI initialisation */
    LPSPI_Init();

#if (defined(gAppMsInterruptPinCnt_c) && (gAppMsInterruptPinCnt_c > 0))
    (void)HAL_GpioInstallCallback((hal_gpio_handle_t)g_GpioHandle[0], (hal_gpio_callback_t)MS_INT_1_HANDLER, NULL);
#endif /* (defined(gAppMsInterruptPinCnt_c) && (gAppMsInterruptPinCnt_c > 0)) */
#if (defined(gAppMsInterruptPinCnt_c) && (gAppMsInterruptPinCnt_c > 1))
    (void)HAL_GpioInstallCallback((hal_gpio_handle_t)g_GpioHandle[1], (hal_gpio_callback_t)MS_INT_2_HANDLER, NULL);
#endif /* (defined(gAppMsInterruptPinCnt_c) && (gAppMsInterruptPinCnt_c > 1)) */

    Write_register(0x7E, 0xB6);
    //Normal mode
    Write_register(0x19,0x02);
    //Range 2G,ODR 100 HZ, osr highest accuracy
    Update_Ms_register_0x1A(((pSysParams->system_params).fields.ms_accuracy_range), ((pSysParams->system_params).fields.ms_osr), ((pSysParams->system_params).fields.ms_odr));
    //Write_register(0x1A,0x38);
    //Accelerometer configuration 100Hz output data rate
    Write_register(0x1B,0x04);
    //Interrupt bit control gen1_int_en = 1
    Write_register(0x1F,0x04);

    //step_int_en = 1
    Write_register(0x20,0x01);
    //gen1_int is mapped to INT2 no motion still mapping
    Write_register(0x22,0x04);
    //step_without_scan interrupt mapped to INT1
    Write_register(0x23,0x01);
    //Write_register(0x21,0x04);
    //INT 1 and INT2 High active
    Write_register(0x24,0x22);
    //3 axes evaluation 24mg hysteresis updated every time by data source
    Write_register(0x3F,0xF9);

    //Autolopower config : gen1_int trigger going into low power
    Write_register(0x2B,0x02);
    //Use wake up interrupt for condition auto wake up
    Write_register(0x2D,0x02);
    //3axes for wake up interrupt with x sample,one time automated update before go LP mode
    Write_register(0x2F,0xE5);
    //Threshold for for wake up
    Write_register(0x30,0x02);

    //Gen1_comb_sel = AND gen1_criterion_sel = 0
    Write_register(0x40,0x01);
    //Threshold configuration detection gen1
    Update_Ms_register_0x41(((pSysParams->system_params).fields.ms_threshold_no_motion_detected));
    //Write_register(0x41,0x08);
    //Duration for gen1 interrupt
    Update_Ms_register_0x42(((pSysParams->system_params).fields.ms_motion_still_duration));
    //Write_register(0x42,0x00);
    //Duration for gen1 interrupt
    Write_register(0x43,0x64);
    Write_register(0x44,0x47);
    Write_register(0x46,0xF4);
    Write_register(0x47,0x0F);
    Write_register(0x49,0x04);


    if(((pSysParams->system_params).fields.ms_wrist_mode) == 1)
    {
        Ms_wrist_mode_on();
    }
    else if(((pSysParams->system_params).fields.ms_wrist_mode) == 0)
    {
        Ms_wrist_mode_off();
    }
    else
    {
        ;
    }
}

int8_t Get_Ms_Temp_Value()
{
	uint8_t TempValue = Read_register(0x11);
	return ((((int8_t)TempValue)*0.5)+23.0);
}

void Ms_EnableGlobal_IRQ()
{
	OSA_EnableIRQGlobal();
}

void Ms_DisableGlobal_IRQ()
{
	OSA_DisableIRQGlobal();
}
/*******************************************************************************
 * Private functions definitions
 ******************************************************************************/
#if (defined(gAppMsInterruptPinCnt_c) && (gAppMsInterruptPinCnt_c > 0))
/*!*************************************************************************************************
 \fn     void MS_INT_1_HANDLER(void *GpioHandle)
 \brief  This function is used to handle walk detected event from motion sensor

 \param  [in]   GpioHandle - pointer to the gpio pin handle;
 ***************************************************************************************************/
void MS_INT_1_HANDLER(void *GpioHandle)
{
	 systemParameters_t *pSysParams = NULL;
		App_NvmReadSystemParams(&pSysParams);

		_ms_stop_timer(s_MsTimerHandle);
	    //Disable to just check one step_without_scan
	    //DisableIRQ(GPIOC_INT0_IRQn);
	    DisableIRQ(GPIOB_INT0_IRQn);

	#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
	    /* Clear external interrupt flag. */
	    GPIO_GpioClearInterruptFlags(GPIOC, 1U << 7U);
	#else
	    /* Clear external interrupt flag. */
	    GPIO_PortClearInterruptFlags(BOARD_SW_GPIO, 1U << 7U);
	#endif
	    //The interruption mean walking
	    in_motion = true;
	    anti_rebound_int =1;
	    u32DelayLoop_Max = 25000;

	    //Check if 0x18 is 1(WALK) or 2(RUN)
	    uint8_t registre_step_STAT = Read_register(0x18);
	    Led5Off();

        if (firstint1 == 0)//Anti rebond
        {
            firstint1 = 1;
        }
        else
        {
            firstint1 = 0; //Incerement step_without_scan count
    	    if(registre_step_STAT !=0)
    	    {

    	    	if(maPeerInformation[mCurrentPeerId].appState != mAppRunning_c)
    	    	{
    	    		KEYFOB_MGR_notify(KEYFOB_EVENT_WALK_DETECTED);
    	    	}
    	    	step++;
    	    	step_while_scan++;
    	    	total_step_count++;
    	    	u32DelayLoop_Max = 100000;
    	    	Led5On();
                Blink_led_ms();
                Led5Off();
    	    	PRINTF("step count while scanning %d \r\n",step_while_scan);
    	        PRINTF("step count whithout scanning %d \r\n",step_without_scan);
    	        PRINTF("Total step count %d \r\n\n",total_step_count);

    	    }
    	    else
    	    {
    	    		Led5On();
    	    		step_without_scan++;
    	    		total_step_count++;
    	    		step++;
    	    		PRINTF("step count while scanning %d \r\n",step_while_scan);
    	            PRINTF("step count whithout scanning %d \r\n",step_without_scan);
    	            PRINTF("Total step count %d \r\n\n",total_step_count);
    	            u32DelayLoop_Max = 100000;
    	            for(u32DelayLoop = 0; u32DelayLoop < u32DelayLoop_Max; u32DelayLoop++)
    	            {
    	            	__asm volatile ("nop");
    	            }
    	            Led5Off();
    	     }
	    }

	    if(step == ((pSysParams->system_params).fields.ble_scan_on_step))
	    {
	        //KEYFOB_MGR_notify(KEYFOB_EVENT_WALK_DETECTED);
	    }

	    //Enable to check no motion interrupt
	    EnableIRQ(GPIOB_INT0_IRQn);
	    EnableIRQ(GPIOC_INT0_IRQn);
	    SDK_ISR_EXIT_BARRIER;
 }
#endif /*gAppMsInterruptPinCnt_c > 0*/

#if (defined(gAppMsInterruptPinCnt_c) && (gAppMsInterruptPinCnt_c > 1))
/*!*************************************************************************************************
 \fn     void MS_INT_2_HANDLER(void *GpioHandle)
 \brief  This function is used to handle motion still detected event from motion sensor

 \param  [in]   GpioHandle - pointer to the gpio pin handle;
 ***************************************************************************************************/
void MS_INT_2_HANDLER(void *GpioHandle)
{
	if (firstint2 == 0)//Anti rebond
	{
		firstint2 = 1;
	}
	else
	{
		firstint2 = 0;
	if(((maPeerInformation[mCurrentPeerId].appState) == mAppRunning_c) && (mGapRole == gGapCentral_c))
    {
        systemParameters_t *pSysParams = NULL;
        App_NvmReadSystemParams(&pSysParams);

        /* start 5 minutes timer */
        _ms_stop_timer(s_MsTimerHandle);
		//_ms_start_timer(s_MsTimerHandle, ((pSysParams->system_params).fields.ble_disc_during_still)*60*1000);
    }
	step=0;
    still_detected_count++;
    PRINTF("Still detected %d \r\n",still_detected_count);
    //Disable to just check one step_without_scan
    //DisableIRQ(GPIOC_INT0_IRQn);
    DisableIRQ(GPIOB_INT0_IRQn);
    KEYFOB_MGR_notify(KEYFOB_EVENT_MOTION_STILL_DETECTED);

#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
    /* Clear external interrupt flag. */
    GPIO_GpioClearInterruptFlags(GPIOB, 1U << 3U);
#else
    /* Clear external interrupt flag. */
    GPIO_PortClearInterruptFlags(PORTB, 1U << 3U);
#endif
    //The interruption mean no motion
    in_motion = false;
    //Reset step_without_scan count
    //step_without_scan = 0;
    //TRACE_DEBUG("step_without_scan count reset");
    u32DelayLoop_Max = 1000000;

    //Led for walk detected ON -> OFF
    Led4Off();
    //Led for still detected OFF -> ON
    Led5On();

	   /*Blink_led_ms();
		Blink_led_ms();*/
		Blink_led_ms();

    //EnableIRQ(GPIOB_INT0_IRQn);
    EnableIRQ(GPIOC_INT0_IRQn);

    SDK_ISR_EXIT_BARRIER;
}
}
#endif /*gAppMsInterruptPinCnt_c > 1*/

void Update_Ms_register_0x1A(int32_t ms_accuracy_range, int32_t ms_osr, int32_t ms_odr)
{
    /* Convert parameters to 8-bit hexadecimal */
    uint8_t hexValue = (ms_accuracy_range << 6) | (ms_osr << 4) | ms_odr;

    Write_register(0x1A, hexValue);
    //PRINTF("Set MS register 0x1A value to : %02x", hexValue);
}

void Update_Ms_register_0x41(int32_t ms_threshold_activity_change)
{
    /* Convert parameter to 8-bit hexadecimal */
    uint8_t hexValue = ms_threshold_activity_change & 0xFF;

    Write_register(0x41, hexValue);
    //PRINTF("Set MS register 0x41 value to : %02x", hexValue);
}

void Update_Ms_register_0x42(int32_t ms_motion_still_duration)
{
    /* Convert parameter to 8-bit hexadecimal */
    uint8_t hexValue = ms_motion_still_duration & 0xFF;

    Write_register(0x42, hexValue);
    //PRINTF("Set MS register 0x42 value to : %02x", hexValue);
}

void Ms_wrist_mode_on(void)
{
    /* Wrist-On configuration */
    Write_register(0x59,0x01);
    Write_register(0x5A,0x2D);
    Write_register(0x5B,0x7B);
    Write_register(0x5C,0xD4);
    Write_register(0x5D,0x44);
    Write_register(0x5E,0x01);
    Write_register(0x5F,0x3B);
    Write_register(0x60,0x7A);
    Write_register(0x61,0xDB);
    Write_register(0x62,0x7B);
    Write_register(0x63,0x3F);
    Write_register(0x64,0x6C);
    Write_register(0x65,0xCD);
    Write_register(0x66,0x27);
    Write_register(0x67,0x19);
    Write_register(0x68,0x96);
    Write_register(0x69,0xA0);
    Write_register(0x6A,0xC3);
    Write_register(0x6B,0x0E);
    Write_register(0x6C,0x0C);
    Write_register(0x6D,0x3C);
    Write_register(0x6E,0xF0);
    Write_register(0x6F,0x00);
    Write_register(0x70,0xF7);
    PRINTF("MS wrist mode on !");
}

void Ms_wrist_mode_off(void)
{
    /* Wrist-Off configuration */
    Write_register(0x59,0x01);
    Write_register(0x5A,0x32);
    Write_register(0x5B,0x78);
    Write_register(0x5C,0xE6);
    Write_register(0x5D,0x84);
    Write_register(0x5E,0x00);
    Write_register(0x5F,0x84);
    Write_register(0x60,0x6C);
    Write_register(0x61,0x9C);
    Write_register(0x62,0x75);
    Write_register(0x63,0x64);
    Write_register(0x64,0x7E);
    Write_register(0x65,0xAA);
    Write_register(0x66,0x0C);
    Write_register(0x67,0x0C);
    Write_register(0x68,0x4A);
    Write_register(0x69,0xA0);
    Write_register(0x6A,0x00);
    Write_register(0x6B,0x00);
    Write_register(0x6C,0x0C);
    Write_register(0x6D,0x3C);
    Write_register(0x6E,0xF0);
    Write_register(0x6F,0x01);
    Write_register(0x70,0x00);
    PRINTF("MS wrist mode off !");
}
#endif /* BMW_KEYFOB_EVK_BOARD */

/*******************************************************************************
 * Private functions
 ******************************************************************************/

/*! *********************************************************************************
 * \brief  Write register in MS.
 *
 * \param[in]    register_addr       Register address
 * \param[in]    value_to_write      value to write in register address
********************************************************************************** */
/* This function is used to write register in MS */
static void Write_register(uint8_t register_addr, uint8_t value_to_write)
{
    //First bit to 0 for writing in SPI
    uint8_t write_mask = 0b01111111;

    masterTxData[0] = register_addr & write_mask;
    masterTxData[1] = value_to_write;

    isTransferCompleted = false;

    //Send SPI trame and receive at the same time
    DRIVER_MASTER_SPI.Transfer(masterTxData,masterRxData, TRANSFER_SIZE_WRITE);

    while(!isTransferCompleted)
    {

    }
}

/*! *********************************************************************************
 * \brief  Read register in MS.
 *
 * \param[in]    register_addr       Register address
********************************************************************************** */
static uint8_t Read_register(uint8_t register_addr)
{
	uint8_t read_mask = 0b10000000;

	uint8_t tmp = register_addr;

	/* Put register addr in first trame and force first bit to 1 for reading */
	masterTxData[0] = register_addr | read_mask;

	isTransferCompleted = false;

	for(int i = 0; i < 3; i++)
	{
		DRIVER_MASTER_SPI.Transfer(masterTxData,masterRxData, TRANSFER_SIZE);

		/* Need to wait a little to receive data */
	    while(!isTransferCompleted)
	    {

	    }
	}
	return masterRxData[2];
}

/*! *********************************************************************************
 * \brief  Create a software timer for motion sensor.
 *
 * \param[in]    timer       Timer handle
 * \param[in]    type        Timer type
 * \param[in]    argument    Timer argument
********************************************************************************** */
static TimerHandle_t _ms_create_timer(const osa_time_def_t *timer, ms_timer_type type, void *argument)
{
    return xTimerCreate((const char *)"",
                        1, /* period should be filled when starting the Timer using osTimerStart */
                        (type == PERIODIC_TIMER),
                        (void *) argument,
                        (TimerCallbackFunction_t)timer->pfCallback);
}

/*! *********************************************************************************
 * \brief  Start a software timer for motion sensor.
 *
 * \param[in]    timer       Timer handle
 * \param[in]    millisec    Timer period
********************************************************************************** */
static void _ms_start_timer(TimerHandle_t timer, uint32_t millisec)
{
    TickType_t ticks = millisec / portTICK_PERIOD_MS;

    if (ticks == 0)
    {
        ticks = 1;
    }
    xTimerChangePeriod(timer, ticks, 0);
}

/*! *********************************************************************************
 * \brief  Stop a software timer for motion sensor.
 *
 * \param[in]    timer       Timer handle
********************************************************************************** */
static void _ms_stop_timer(TimerHandle_t timer)
{
    xTimerStop(timer, 0);
}

/*! *********************************************************************************
 * \brief  This is the motion sensor timer handler.
********************************************************************************** */
static void _ms_timer_handler(void const *argument)
{
    (void) argument;
    bBleDisconnectionDuringStill = true;
    TRACE_INFO("BLE disconnect.");
    (void)Gap_Disconnect(mCurrentPeerId);
}
