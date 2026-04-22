/*
 * power.h
 *
 *  Created on: Apr 5, 2026
 *      Author: Joseph Anthony
 */

#ifndef INC_POWER_H_
#define INC_POWER_H_

#include <stdint.h>
#include <stdbool.h>

// Registers

#define REG_RTCHSEC		0x00
#define REG_RTCSEG		0x01
#define REG_RTCMIN		0x02
#define REG_RTCHOUR		0x03
#define REG_RTCWKDAY	0x04
#define REG_RTCDATE		0x05
#define REG_RTC_MTH		0x06
#define REG_RTCYEAR		0x07

// Instruction Set

#define RTC_EEREAD		0b00000011
#define RTC_EEWRITE		0b00000010
#define RTC_EEWRDI		0b00000100
#define RTC_EEWREN		0b00000110

#define RTC_SRREAD		0b00000101
#define RTC_SRWRITE		0b00000001
#define RTC_READ		0b00010011
//#define RTC_READ		0b11001000
#define RTC_WRITE		0b00010010

#define RTC_UNLOCK		0b00010100
#define RTC_IDWRITE		0b00110010
#define RTC_IDREAD		0b00110011
#define RTC_CLRRAM		0b01010100

// Constants
#define LOAF_ADDRESS 	0x28

uint32_t power_draw(uint8_t*);
bool rtc_outage_flag();

#endif /* INC_POWER_H_ */
