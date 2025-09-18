/*
 * SystemTime.h
 *
 *  Created on: 29 Nis 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_SYSTEMTIME_H_
#define MAIN_SYSTEMTIME_H_

#include <time.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <sys/time.h>
#include <driver/rtc_io.h>
#include "driver/i2c.h"

#define MCP7940N_ADDR  0x6F  // 7-bit I2C address
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0

extern uint32_t currentTime_epoch;

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day_of_week;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} rtc_time_t;

extern rtc_time_t DeviceTime;
esp_err_t mcp7940n_init();
esp_err_t mcp7940n_set_time(const rtc_time_t *time);
esp_err_t mcp7940n_get_time(rtc_time_t *time);
void i2c_master_init(void);


#endif /* MAIN_SYSTEMTIME_H_ */
