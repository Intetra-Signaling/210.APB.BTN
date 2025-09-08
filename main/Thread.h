/*
 * Thread.h
 *
 *  Created on: 15 Nis 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_THREAD_H_
#define MAIN_THREAD_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



void ProcessThread(void *arg);

void TimerCallback_50ms(TimerHandle_t xTimer);
void TimerCallback_500ms(TimerHandle_t xTimer);
void TimerCallback_1000ms(TimerHandle_t xTimer);
void Timer_Threads_Init(void);
void mongoose_task(void *pvParameters);
void play_wav_task(void *pvParameters);
void stopTestModeThread(TimerHandle_t xTimer);
void stopTestMode(void);
void FlashWrite_task(void *pvParameters);
void init_nvs_mutex(void);
void IO_Task(void *pvParameters);
void suspend_all_tasks_for_ota(void);

extern TimerHandle_t xTimer_50ms;
extern TimerHandle_t xTimer_500ms;
extern TimerHandle_t xTimer_1000ms;

extern TaskHandle_t mongoose_task_handle;
extern TaskHandle_t process_task_handle;
extern TaskHandle_t play_wav_task_handle;
extern TaskHandle_t xIO_TaskHandle;
extern TaskHandle_t flashWrite_task_handle;

extern uint8_t ui8_greenCountFrom;
extern uint8_t ui8_greenCountTo;
extern bool ui8_isIdleActive;
extern char ch_idleSound[50];
extern bool ui8_idleContAfterReq;
extern bool ui8_isReqActive; 
extern bool ui8_isGreenActive;
extern uint8_t RequestSoundPlaybackPeriod;
extern  SemaphoreHandle_t s_nvs_global_mutex; 
extern char *TAG_GLUE; 
extern  SemaphoreHandle_t xFlashMutex;
#endif /* MAIN_THREAD_H_ */
