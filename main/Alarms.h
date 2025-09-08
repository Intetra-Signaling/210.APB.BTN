/*
 * Alarms.h
 *
 *  Created on: 23 Tem 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_ALARMS_H_
#define MAIN_ALARMS_H_

#include <stdint.h>
#include "SystemTime.h"

#define FLASH_ALARM_KEY "alarm_logs"
#define MAX_ALARMS 50
#define MAX_ALARM_MSG_LEN 64
#define MAX_LOG_MESSAGE_LEN 64
#define MAX_LOG_DATE_STR_LEN 20
#define MAX_LOGS_JSON_EXPORT 50
typedef struct {
    char message[MAX_ALARM_MSG_LEN];
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} AlarmEntry_t;


typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel_t;


typedef struct {
    char date[MAX_LOG_DATE_STR_LEN];   // "2025-06-10 12:34:56"
    LogLevel_t level;
    char message[MAX_LOG_MESSAGE_LEN];
} AlarmJsonLog_t;


void Alarm_Log(const char *msg, rtc_time_t *time);
void Alarm_PrintAll(void);
void Alarm_ClearAll(void);
const AlarmEntry_t* Alarm_GetLog(uint8_t index);
uint8_t Alarm_GetCount(void);
esp_err_t Alarm_SaveToFlash(void);
esp_err_t Alarm_LoadFromFlash(void);
const AlarmJsonLog_t* Alarm_GetJsonStructLogs(uint8_t *out_count);
void Alarm_Remove(const char *msg);
#endif /* MAIN_ALARMS_H_ */
