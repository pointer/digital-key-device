/*
 (c) NXP B.V. 2009-2019. All rights reserved.

 Disclaimer
 1. The NXP Software/Source Code is provided to Licensee "AS IS" without any
 warranties of any kind. NXP makes no warranties to Licensee and shall not
 indemnify Licensee or hold it harmless for any reason related to the NXP
 Software/Source Code or otherwise be liable to the NXP customer. The NXP
 customer acknowledges and agrees that the NXP Software/Source Code is
 provided AS-IS and accepts all risks of utilizing the NXP Software under
 the conditions set forth according to this disclaimer.

 2. NXP EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING,
 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY
 RIGHTS. NXP SHALL HAVE NO LIABILITY TO THE NXP CUSTOMER, OR ITS
 SUBSIDIARIES, AFFILIATES, OR ANY OTHER THIRD PARTY FOR ANY DAMAGES,
 INCLUDING WITHOUT LIMITATION, DAMAGES RESULTING OR ALLEGDED TO HAVE
 RESULTED FROM ANY DEFECT, ERROR OR OMMISSION IN THE NXP SOFTWARE/SOURCE
 CODE, THIRD PARTY APPLICATION SOFTWARE AND/OR DOCUMENTATION, OR AS A
 RESULT OF ANY INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHT OF ANY
 THIRD PARTY. IN NO EVENT SHALL NXP BE LIABLE FOR ANY INCIDENTAL,
 INDIRECT, SPECIAL, EXEMPLARY, PUNITIVE, OR CONSEQUENTIAL DAMAGES
 (INCLUDING LOST PROFITS) SUFFERED BY NXP CUSTOMER OR ITS SUBSIDIARIES,
 AFFILIATES, OR ANY OTHER THIRD PARTY ARISING OUT OF OR RELATED TO THE NXP
 SOFTWARE/SOURCE CODE EVEN IF NXP HAS BEEN ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGES.

 3. NXP reserves the right to make changes to the NXP Software/Sourcecode any
 time, also without informing customer.

 4. Licensee agrees to indemnify and hold harmless NXP and its affiliated
 companies from and against any claims, suits, losses, damages,
 liabilities, costs and expenses (including reasonable attorney's fees)
 resulting from Licensee's and/or Licensee customer's/licensee's use of the
 NXP Software/Source Code.
 */

/*
 *    @file: phscaNcj29d6_Cfg.c
 *   @brief: Configuration of driver for NCJ29D6 UWB module
 */

/* =============================================================================
 * External Includes
 * ========================================================================== */
#include "phscaTypes.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "fsl_lpspi.h"
#include "fsl_common_arm.h"

/* =============================================================================
 * Internal Includes
 * ========================================================================== */
#define PHSCANCJ29D6_CFG_EXTERN_GUARD
#include "phscaNcj29d6_Cfg.h"
#include "phscaNcj29d6.h"
#undef PHSCANCJ29D6_CFG_EXTERN_GUARD

/* =============================================================================
 * Private Symbol Defines
 * ========================================================================== */
#define PHSCANCJ29D6_SPI_INSTANCE        LPSPI0
#define PHSCANCJ29D6_SPI_BAUDRATE_HZ     (uint32_t)(10000000UL)

/* =============================================================================
 * Private Function-like Macros
 * ========================================================================== */

/* =============================================================================
 * Private Type Definitions
 * ========================================================================== */
typedef struct
{
	PORT_Type* port;
	GPIO_Type* gpio;
	uint32_t pin;
} Pin_t;

/* =============================================================================
 * Private Function Prototypes
 * ========================================================================== */

/* =============================================================================
 * Private Module-wide Visible Variables
 * ========================================================================== */

/* Ranger4 J5 and J6 sockets on KW45EVK Rev0
    KW45 <<------------------->> Ranger4
    PTB2  - LPSPI1_SK   <-> SCLK
    PTB1  - LPSPI1_SIN  <-> MOSI
    PTB3  - LPSPI1_SOUT <-> MISO
    PTB4  - LPSPI1_PCS3 <-> CS_N
    PTA18 - GPIO        <-> RST_N
    PTC5  - GPIO        <-> RDY_N
    PTC4  - GPIO        <-> INT_N
*/

/* Baba 22/01/2024
    KW45 <<------------------->> Ranger5
    PTA19  - LPSPI1_SK   <-> SCLK
    PTA18  - LPSPI1_SIN  <-> MOSI
    PTA17  - LPSPI1_SOUT <-> MISO
    PTA16  - LPSPI1_PCS3 <-> CS_N
    PTD2 - GPIO        <-> RST_N
    PTA21  - GPIO        <-> RDY_N
    PTB4  - GPIO        <-> INT_N
*/

static const Pin_t pin_r4_cs  = { .port = PORTA, .gpio = GPIOA, .pin = 16 };
static const Pin_t pin_r4_rst = { .port = PORTD, .gpio = GPIOD, .pin = 2 };
static const Pin_t pin_r4_rdy = { .port = PORTA, .gpio = GPIOA, .pin = 21 };
static const Pin_t pin_r4_int = { .port = PORTB, .gpio = GPIOB, .pin = 4 };

static const gpio_pin_config_t gpio_config_input = { .pinDirection = kGPIO_DigitalInput, .outputLogic = 1 };
static const gpio_pin_config_t gpio_config_output = { .pinDirection = kGPIO_DigitalOutput, .outputLogic = 1 };

static const port_pin_config_t port_pin_config = {
	.mux = kPORT_MuxAsGpio,
	.pullSelect = kPORT_PullUp,
	.slewRate = kPORT_FastSlewRate,
	.passiveFilterEnable = kPORT_PassiveFilterDisable,
	.driveStrength = kPORT_LowDriveStrength,
};

/* =============================================================================
 * Function Definitions
 * ========================================================================== */
void phscaNcj29d6_InitDevice(void)

{

#define EXAMPLE_LPSPI_MASTER_CLOCK_NAME       (kCLOCK_Lpspi0)
#define EXAMPLE_LPSPI_MASTER_CLOCK_SOURCE     (kCLOCK_IpSrcFro192M)
#define LPSPI_MASTER_CLK_FREQ                 (CLOCK_GetIpFreq(EXAMPLE_LPSPI_MASTER_CLOCK_NAME))
#if 0

    /* TODO: Enable clocks for GPIOs of 6-wire SPI protocol */
    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);
    //CLOCK_EnableClock(kCLOCK_PortD);

    const port_pin_config_t lpspi1_sin_port_pin_cfg = {/* Internal pull-up resistor is enabled */
                                                    kPORT_PullUp,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Normal drive strength is configured */
                                                    kPORT_NormalDriveStrength,
                                                    /* Pin is configured as LPSPI0_SIN */
                                                    kPORT_MuxAlt2,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORTA18 is configured as LPSPI1_SIN */
    PORT_SetPinConfig(PORTA, 17U, &lpspi1_sin_port_pin_cfg);

    const port_pin_config_t lpspi1_sout_port_pin_cfg = {/* Internal pull-up resistor is enabled */
                                                    kPORT_PullUp,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Normal drive strength is configured */
                                                    kPORT_NormalDriveStrength,
                                                    /* Pin is configured as LPSPI0_SOUT */
                                                    kPORT_MuxAlt2,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORTA17 is configured as LPSPI1_SOUT */
    PORT_SetPinConfig(PORTA, 18U, &lpspi1_sout_port_pin_cfg);
#endif
    const port_pin_config_t lpspi1_sck_port_pin_cfg = {/* Internal pull-up resistor is enabled */
                                                    kPORT_PullUp,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Normal drive strength is configured */
                                                    kPORT_NormalDriveStrength,
                                                    /* Pin is configured as LPSPI0_SCK */
                                                    kPORT_MuxAlt2,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORTA19 is configured as LPSPI1_SCK */
    PORT_SetPinConfig(PORTA, 19U, &lpspi1_sck_port_pin_cfg);

    /* Initialize SPI */
    CLOCK_SetIpSrc(EXAMPLE_LPSPI_MASTER_CLOCK_NAME, EXAMPLE_LPSPI_MASTER_CLOCK_SOURCE);
    CLOCK_SetIpSrcDiv(EXAMPLE_LPSPI_MASTER_CLOCK_NAME, kSCG_SysClkDivBy16);

    lpspi_master_config_t spi_config = {
            .baudRate = PHSCANCJ29D6_SPI_BAUDRATE_HZ,
            .bitsPerFrame = 8,
            .cpol = kLPSPI_ClockPolarityActiveLow,
            .cpha = kLPSPI_ClockPhaseFirstEdge,
            .direction = kLPSPI_MsbFirst,
            .pcsToSckDelayInNanoSec        = 0,
            .lastSckToPcsDelayInNanoSec    = 0,
            .betweenTransferDelayInNanoSec = 0,
            .whichPcs           = kLPSPI_Pcs3,
            .pcsActiveHighOrLow = kLPSPI_PcsActiveLow,
            .pinCfg        = kLPSPI_SdiInSdoOut,
            .dataOutConfig = kLpspiDataOutRetained
    };
    LPSPI_MasterInit(PHSCANCJ29D6_SPI_INSTANCE, &spi_config, LPSPI_MASTER_CLK_FREQ);

    /* Init CS_N as GPIO output */
    PORT_SetPinConfig(pin_r4_cs.port, pin_r4_cs.pin, &port_pin_config);
    GPIO_PinInit(pin_r4_cs.gpio, pin_r4_cs.pin, &gpio_config_output);

    /* Init RST_N pin as GPIO output */
    PORT_SetPinConfig(pin_r4_rst.port, pin_r4_rst.pin, &port_pin_config);
    GPIO_PinInit(pin_r4_rst.gpio, pin_r4_rst.pin, &gpio_config_output);

    /* Init RDY_N pin as GPIO input */
    PORT_SetPinConfig(pin_r4_rdy.port, pin_r4_rdy.pin, &port_pin_config);
    GPIO_PinInit(pin_r4_rdy.gpio, pin_r4_rdy.pin, &gpio_config_input);

    /* Init INT_N pin as GPIO input, enable GPIO IRQ for it as well.
     * phscaNcj29d6_SetIntPinInterruptEnable shall control whether this interrupt is enabled in runtime. */
    PORT_SetPinConfig(pin_r4_int.port, pin_r4_int.pin, &port_pin_config);
    GPIO_PinInit(pin_r4_int.gpio, pin_r4_int.pin, &gpio_config_input);
    status_t enableIrqStatus = EnableIRQ(GPIOA_INT0_IRQn);
    uint32_t oldIsr = InstallIRQHandler(GPIOA_INT0_IRQn, (uint32_t)&phscaNcj29d6_IntPinCallbackIsr);
}

void phscaNcj29d6_DelayMilliseconds(const uint32_t u32_DelayMilliseconds)
{
	SDK_DelayAtLeastUs(u32_DelayMilliseconds * 1000u, SystemCoreClock);
}

void phscaNcj29d6_SetRst(const bool b_AssertRst)
{
    GPIO_PinWrite(pin_r4_rst.gpio, pin_r4_rst.pin,
                  b_AssertRst == PHSCATYPES_b_TRUE ? (uint8_t)0u : (uint8_t)1u);
}

void phscaNcj29d6_SetCs(const bool b_AssertCs)
{
    GPIO_PinWrite(pin_r4_cs.gpio, pin_r4_cs.pin,
                  b_AssertCs == PHSCATYPES_b_TRUE ? (uint8_t)0u : (uint8_t)1u);
}

bool phscaNcj29d6_GetRdy(void)
{
    return (bool)GPIO_PinRead(pin_r4_rdy.gpio, pin_r4_rdy.pin);
}

bool phscaNcj29d6_GetInt(void)
{
    return (bool)GPIO_PinRead(pin_r4_int.gpio, pin_r4_int.pin);
}

void phscaNcj29d6_SetIntPinInterruptEnable(const bool b_EnableIntPinInterrupt)
{
	GPIO_SetPinInterruptConfig(pin_r4_int.gpio, pin_r4_int.pin, b_EnableIntPinInterrupt == true ? kGPIO_InterruptFallingEdge : kGPIO_InterruptStatusFlagDisabled);
}

uint16_t phscaNcj29d6_CalculateCrc16(uint8_t u8arr_Data[], uint16_t u16_DataLength)
{
    return phscaNcj29d6_CalculateCrc16Sw(u8arr_Data, u16_DataLength);
}

void phscaNcj29d6_ClearIntIrqStatus(void)
{
	GPIO_PinClearInterruptFlag(pin_r4_int.gpio, pin_r4_int.pin);
}

void phscaNcj29d6_SpiTransceive(const uint32_t u32_DataLength, const uint8_t u8arr_DataToTransmit[],
								uint8_t u8arr_DataReceived[])
{
    lpspi_transfer_t transfer = {
            .txData = (uint8_t *)u8arr_DataToTransmit,
            .rxData = (uint8_t *)u8arr_DataReceived,
            .dataSize = (size_t)u32_DataLength,
            .configFlags = kLPSPI_MasterPcs3 | kLPSPI_MasterPcsContinuous | kLPSPI_MasterByteSwap
    };
    LPSPI_MasterTransferBlocking(PHSCANCJ29D6_SPI_INSTANCE, &transfer);
}
