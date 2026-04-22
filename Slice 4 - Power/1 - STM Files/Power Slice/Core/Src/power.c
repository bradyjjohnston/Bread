/*
 * power.c
 *
 *  Created on: Apr 5, 2026
 *      Author: Joseph Anthony
 */

#include "power.h"
#include "stm32f3xx_hal.h"

extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;

// Pin Defines
#define RTC_CS_PORT GPIOA
#define RTC_CS_PIN  GPIO_PIN_3

bool rtc_outage_flag()
{
	HAL_StatusTypeDef HAL_STATUS;
	uint8_t rx[1];
	uint8_t tx[2] = {RTC_READ, 0x04};
	uint8_t isOutage = 0;

	do {
		cs_low();
		HAL_Delay(100);
		HAL_SPI_TransmitReceive(&hspi1,tx,rx,3,1000);
		HAL_Delay(100);
		cs_high();
		HAL_Delay(100);
	} while (HAL_STATUS != HAL_OK);

    isOutage = (rx[0] & 0b00010000);	// Isolate PWR_FAIL pin
    isOutage = isOutage >> 4;

    return isOutage;
}

static uint8_t rtc_outage_duration(uint8_t* reg)
{

    uint8_t rx[8];
    uint8_t tx[2] = {RTC_READ, 0x18};

    cs_low();
    HAL_SPI_TransmitReceive(&hspi1,tx,rx,10,HAL_MAX_DELAY);
    cs_high();

    for (uint8_t i = 0; i < 4; i++)
    {
    	reg[i]=rx[i]-rx[i+4];
    }

    return reg;
}

uint32_t power_draw(uint8_t* reg)
{
	/* 	Reports the power draw in uW
	*
	*	Sensors are 264 mV/A, ADCs are 12-bit, 3V3 supply
	*	AD_RES_BUFFER pulls current values from 3V3, 5V, and 19V5
	*
	*	POWER = VOLTAGE * VAL * 3.3/(0.264*4096)
	*
	*	Pre-computing this value, we get approximately:
	*		- 10.0708 mW per ADC val on the 3V3 line
	*		- 15.2588 mW per ADC val on the 5V line
	*	We can then overestimate our power draw per-component
	*
	 */

	uint32_t pwr_output = 0;

	HAL_ADC_Start_DMA(&hadc1, reg, 3);
	pwr_output = pwr_output + (reg[0] - 2000) * 11;
	pwr_output = pwr_output + (reg[1] - 2000) * 11;
	pwr_output = pwr_output + (reg[2] - 2000) * 16;

	return pwr_output;
}

 void cs_low()
{
    HAL_GPIO_WritePin(RTC_CS_PORT, RTC_CS_PIN, GPIO_PIN_RESET);
}

 void cs_high()
{
    HAL_GPIO_WritePin(RTC_CS_PORT, RTC_CS_PIN, GPIO_PIN_SET);
}
