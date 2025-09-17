/*
 * Thread.c
 *
 *  Created on: 15 Nis 2025
 *      Author: metesepetcioglu
 */
#include "Thread.h"
#include "Alarms.h"
#include "DetectTraffic.h"
#include "FlashConfig.h"
#include "MichADCRead.h"
#include "Plan.h"
#include "SpeakerDriver.h"
#include "SystemTime.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "main.h"
#include "mongoose.h"
#include "mongoose_glue.h"
#include "portmacro.h"
#include "stdbool.h"
#include "wifi.h"
#include <stdint.h>
#include <stdio.h>

#define COUNTDOWN_STEP 1000 // 1 saniye = 1000 ms
#define COUNTDOWN_END 0
uint32_t COUNTDOWN_START;
#define GPIO_INPUT_IO_0 CONFIG_GPIO_INPUT_0
#define GPIO_INPUT_IO_1 CONFIG_GPIO_INPUT_1
#define MIN_GAP_MS 200

#define MAX_ADC_VALUE 4048.0f
#define MAX_VOLUME_FACTOR 0.5f
#define MIN_ADC_VALUE 0.0f
#define MIN_VOLUME_FACTOR 0.0f

#define NOISE_LEVEL_HISTORY_SIZE 15 // Son 15 saniyelik veriyi sakla
#define ADC_UPDATE_INTERVAL_MS 100 // 1 saniyelik periyotla adc verisini goster.
#define TIMER_CALLBACK_INTERVAL_MS 50 // Your current timer interval
#define UPDATES_PER_SECOND                                                     \
  (ADC_UPDATE_INTERVAL_MS / TIMER_CALLBACK_INTERVAL_MS) // 1000ms / 50ms = 20

int s_adc_update_counter = 0;

TimerHandle_t xTimer_50ms;
TimerHandle_t xTimer_500ms;
TimerHandle_t xTimer_1000ms;
TimerHandle_t testModeTimer = NULL;
TaskHandle_t mongoose_task_handle = NULL;
TaskHandle_t process_task_handle = NULL;
TaskHandle_t play_wav_task_handle = NULL;
TaskHandle_t xIO_TaskHandle = NULL;
TaskHandle_t flashWrite_task_handle = NULL;

extern volatile uint32_t g_movingRMS;
extern volatile float volume_factor; // Başlangıç seviyesi
int64_t time_difference;
uint64_t GreenCountdown = 0;
bool RedFB_Flag = false;
bool GreenFB_Flag = false;
bool FirstDetect = false;
bool PlayCountFlag = false;
bool playDemandflag = false;
bool playOrientationflag = false;
bool goCrossOverFlag = false;
bool isPlayGreenLightVoice = false;
uint8_t RequestSoundPlaybackPeriod = 0;
int noise_level_history[NOISE_LEVEL_HISTORY_SIZE] = {0};
bool isGreenCountdownAction = false;

uint8_t ui8_greenCountFrom = 10;
uint8_t ui8_greenCountTo = 0;
bool ui8_isIdleActive = false;
bool ui8_idleContAfterReq = false;
bool ui8_isReqActive = false;
bool ui8_isGreenActive = false;
char ch_idleSound[] = "lutfen_bekleyiniz.wav";
char ch_requestSound[] = "talebiniz_alindi.wav";
char *TAG_GLUE = "GLUE_NVS";

uint8_t isButtonReqActive = 0;
uint8_t isPlayButtonReq = 0;
bool IdlePlayFlag = false;
bool RequestPlayFlag = false;
bool hasRequestPlayedOnce = false;
bool GreenCounterPlayFlag = false;
volatile uint32_t wav_play_delay_ms = 30; // Başlangıçta 150ms
extern volatile bool s_stopADC;
extern bool StartWriteConfigs;
extern bool StopIO_Threat;
char current_playing_file[32] = {0};
bool new_file_available = false;

extern bool ConfigurationPageRequest;
extern bool SettingsPageSystemInfoRequest;
extern bool SettingsPageWifiSettingsRequest;
extern bool AudioConfigurationPageRequest;
extern bool CalendarPageRequest;
extern bool SettingsPageLoginInfoChange;
extern bool NetworkSettingsPageRequest;
extern bool isOtaDone;
extern struct xCurrentConfiguration CurrentConfiguration;


 // Son çalınma zamanlarını tutan değişkenler
TickType_t last_any_play_time = 0;
TickType_t last_sound_end_time = 0; // Son sesin bittiği zaman

#define IDLE_PERIOD_MS 700 // Idle periyodu sabit 700ms
#define MIN_SILENCE_MS 700 // Her ses arası minimum 700ms boşluk
// Global değişkenler
char play_wav_file[60] = {0};
char play_wav_file_2[60] = {0};  // İkinci ses için
volatile bool new_file_ready = false;
volatile bool new_file_2_ready = false;  // İkinci ses için flag
volatile bool audio_playing = false;



/**
 * @brief Initializes and creates periodic FreeRTOS timers.
 *
 * This function sets up two software timers:
 * - A 500ms timer with the callback TimerCallback_500ms.
 * - A 1000ms (1 second) timer with the callback TimerCallback_1000ms.
 * Both timers are created as auto-reload timers.
 */
void Timer_Threads_Init(void) {

  // 500 ms timer
  xTimer_500ms = xTimerCreate("Timer_500ms", pdMS_TO_TICKS(500), pdTRUE, NULL,
                              TimerCallback_500ms);

  // 1000 ms (1 saniye) timer
  xTimer_1000ms = xTimerCreate("Timer_1000ms", pdMS_TO_TICKS(1000), pdTRUE,
                               NULL, TimerCallback_1000ms);
}



/**
 * @brief FreeRTOS thread for managing sound playback logic and state transitions in the pedestrian button project.
 *
 * This thread evaluates the current configuration and input states to determine whether idle, request, green counter, or green action sounds should be played.
 * It handles the scheduling and timing of sound playback, including single and sequential sounds, volume adjustment, and sound duration caching.
 * Special logic is included for test mode, request period management, silence intervals, green light voice and counter, and green action sound.
 *
 * @param[in] arg Pointer to thread parameters (unused).
 */
void Process_Thread(void *arg) {
  static TickType_t last_idle_play_time = 0;
  static TickType_t last_request_end_time = 0;
  static float cached_idle_duration = 0.0;
  static bool idle_duration_cached = false;
  
  while (1) {

   /*
   -------------------------------------------------
   | 000 | 001 | 010 | 011 | 100 | 101 | 110 | 111 |
   |-----------------------------------------------|
   |  +  |  +  |  +  |  +  |  +  |  +  |  +  |  +  |
   -------------------------------------------------
   */

    // --------------------
    // CASE 1: 100
    // --------------------
    if (CurrentConfiguration.isIdleActive == true &&
        CurrentConfiguration.idleContAfterReq == false &&
        CurrentConfiguration.isReqActive == false) {
      RequestPlayFlag = false;
      IdlePlayFlag = true;
      GreenCounterPlayFlag = false;
      // printf("100\n");
      if (Demand_input.isDemandActive ==
          true) // talep alindiktan sonra idle sesini kes. ve yesil saymayi
                // birakana kadar
      {
        IdlePlayFlag = false;
      }
      if (CurrentConfiguration.isGreenActive == true &&
          green_input.confirmed_flag == true) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
        StopPlayWav = true;
        GreenCounterPlayFlag = true;
      }

    }
    // --------------------
    // CASE 2: 110
    // --------------------
    else if (CurrentConfiguration.isIdleActive == true &&
             CurrentConfiguration.idleContAfterReq == true &&
             CurrentConfiguration.isReqActive == false) {
      IdlePlayFlag = true;
      RequestPlayFlag = false;
      GreenCounterPlayFlag = false;
      // printf("110\n");
      if (Demand_input.isDemandActive) // talep alinmissa idle sesi devam etmeli
      {
        IdlePlayFlag = true;
      }
      if (CurrentConfiguration.isGreenActive == true &&
          green_input.confirmed_flag == true) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
        StopPlayWav = true;
        GreenCounterPlayFlag = true;
      }

    }
    // --------------------
    // CASE 3: 010
    // --------------------
    else if (CurrentConfiguration.isIdleActive == false &&
             CurrentConfiguration.idleContAfterReq == true &&
             CurrentConfiguration.isReqActive == false) {
      IdlePlayFlag = false;
      RequestPlayFlag = false;
      GreenCounterPlayFlag = false;
      // printf("010\n");
      if (Demand_input
              .isDemandActive) // talep alinmissa talep gidene kadar sesini cal
      {
        IdlePlayFlag = true;
      }
      if (CurrentConfiguration.isGreenActive == true &&
          green_input.confirmed_flag == true) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
        StopPlayWav = true;
        GreenCounterPlayFlag = true;
      }

    }
    // --------------------
    // CASE 4: 000
    // --------------------
    else if (CurrentConfiguration.isIdleActive == false &&
             CurrentConfiguration.idleContAfterReq == false &&
             CurrentConfiguration.isReqActive == false) {
      IdlePlayFlag = false;
      GreenCounterPlayFlag = false;
      // printf("000\n");
      if (Demand_input.isDemandActive) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
      }
      if (CurrentConfiguration.isGreenActive == true &&
          green_input.confirmed_flag == true) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
        StopPlayWav = true;
        GreenCounterPlayFlag = true;
      }
    }
    // --------------------
    // CASE 5: 111
    // --------------------
    else if (CurrentConfiguration.isIdleActive == true &&
             CurrentConfiguration.idleContAfterReq == true &&
             CurrentConfiguration.isReqActive == true) {
      IdlePlayFlag = true;
      RequestPlayFlag = false;
      GreenCounterPlayFlag = false;
      // printf("111\n");
      if (Demand_input.isDemandActive) {
        IdlePlayFlag = true;
        RequestPlayFlag = true;
        StopPlayWav = true;
      }
      if (CurrentConfiguration.isGreenActive == true &&
          green_input.confirmed_flag == true) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
        StopPlayWav = true;
        GreenCounterPlayFlag = true;
      }
    }
    // --------------------
    // CASE 6: 001
    // --------------------
    else if (CurrentConfiguration.isIdleActive == false &&
             CurrentConfiguration.idleContAfterReq == false &&
             CurrentConfiguration.isReqActive == true) {
      IdlePlayFlag = false;
      GreenCounterPlayFlag = false;
      RequestPlayFlag = false;
      // printf("001\n");
      if (Demand_input.isDemandActive) {
        IdlePlayFlag = false;
        RequestPlayFlag = true;
        StopPlayWav = true;
      }
      if (CurrentConfiguration.isGreenActive == true &&
          green_input.confirmed_flag == true) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
        StopPlayWav = true;
        GreenCounterPlayFlag = true;
      }

    }
    // --------------------
    // CASE 7: 011
    // --------------------
    else if (CurrentConfiguration.isIdleActive == false &&
             CurrentConfiguration.idleContAfterReq == true &&
             CurrentConfiguration.isReqActive == true) {
      IdlePlayFlag = false;
      GreenCounterPlayFlag = false;
      RequestPlayFlag = false;
      // printf("011\n");
      if (Demand_input.isDemandActive) {
        IdlePlayFlag = true;
        RequestPlayFlag = true;
        StopPlayWav = true;
      }
      if (CurrentConfiguration.isGreenActive == true &&
          green_input.confirmed_flag == true) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
        StopPlayWav = true;
        GreenCounterPlayFlag = true;
      }
    }
    // --------------------
    // CASE 8: 101
    // --------------------
    else if (CurrentConfiguration.isIdleActive == true &&
             CurrentConfiguration.idleContAfterReq == false &&
             CurrentConfiguration.isReqActive == true) {
      IdlePlayFlag = true;
      GreenCounterPlayFlag = false;
      RequestPlayFlag = false;
      // printf("101\n");
      if (Demand_input.isDemandActive) {
        IdlePlayFlag = false;
        RequestPlayFlag = true;
        StopPlayWav = true;
      }
      if (CurrentConfiguration.isGreenActive == true &&
          green_input.confirmed_flag == true) {
        IdlePlayFlag = false;
        RequestPlayFlag = false;
        StopPlayWav = true;
        GreenCounterPlayFlag = true;
      }
    }
    
      // --------------------
    // PLAY SOUND
    // --------------------
     if (TestMode == true) {
            snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", s_playSound.fileName);
            new_file_ready = true;
            printf("[PROCESS] Test dosyasi ayarlandi: %s\n", play_wav_file);
            TestMode = false;
        }

        if (TestMode == false) {
            TickType_t current_time = xTaskGetTickCount();
            int req_delay_ms = 0;

            if (RequestPlayFlag) {
                switch (CurrentConfiguration.reqPlayPeriod) {
                    case 1: req_delay_ms = 1000; break;
                    case 2: req_delay_ms = 2000; break;
                    case 3: req_delay_ms = 3000; break;
                    case 4: req_delay_ms = 4000; break;
                    case 5: req_delay_ms = 5000; break;
                    case 6: req_delay_ms = 10000; break;
                    case 7: req_delay_ms = 15000; break;
                    case 8: req_delay_ms = 30000; break;
                    case 9: req_delay_ms = 60000; break;
                    default: req_delay_ms = 0; break;
                }
            }

            TickType_t request_period_ticks = pdMS_TO_TICKS(req_delay_ms);
            TickType_t min_silence_ticks = pdMS_TO_TICKS(MIN_SILENCE_MS);
            bool is_sound_playing = (current_time < last_sound_end_time) || audio_playing; // *** ÖNEMLİ: audio_playing eklendi
            bool silence_period_passed = (current_time >= (last_sound_end_time + min_silence_ticks));

            // Idle ses süresini cache'le
            if (!idle_duration_cached && IdlePlayFlag) {
                char idle_path[256];
                snprintf(idle_path, sizeof(idle_path), "/sdcard/%s", CurrentConfiguration.idleSound);
                cached_idle_duration = CheckWavDuration(idle_path);
                idle_duration_cached = true;
            }

            if (IdlePlayFlag && RequestPlayFlag && req_delay_ms > 0) {
                TickType_t period_time = last_request_end_time + request_period_ticks;
                TickType_t silence_time = last_sound_end_time + min_silence_ticks;
                TickType_t actual_next_request_time = (period_time > silence_time) ? period_time : silence_time;
                
                if (last_request_end_time == 0) {
                    actual_next_request_time = current_time;
                }
                bool request_time_reached = (current_time >= actual_next_request_time);

                if (!is_sound_playing && silence_period_passed) {
                    if (request_time_reached) {
                        // *** REQUEST SES 1 + 2 AYARLAMA ***
                        snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", CurrentConfiguration.reqSound1);
                        new_file_ready = true;
                        printf("[PROCESS] Request ses 1 ayarlandi: %s\n", play_wav_file);
                        
                        // Timing hesaplamaları
                        float duration1 = CheckWavDuration(play_wav_file);
                        TickType_t total_sound_duration = pdMS_TO_TICKS((int)(duration1 * 1000));

                        if (strcmp(CurrentConfiguration.reqSound2, "-") != 0) {
                            snprintf(play_wav_file_2, sizeof(play_wav_file_2), "/sdcard/%s", CurrentConfiguration.reqSound2);
                            new_file_2_ready = true;
                            printf("[PROCESS] Request ses 2 ayarlandi: %s\n", play_wav_file_2);
                            
                            float duration2 = CheckWavDuration(play_wav_file_2);
                            total_sound_duration += pdMS_TO_TICKS((int)(duration2 * 1000));
                            total_sound_duration += pdMS_TO_TICKS(50); // İki ses arası delay
                        } else {
                            new_file_2_ready = false;
                            play_wav_file_2[0] = '\0';
                        }

                        last_sound_end_time = current_time + total_sound_duration;
                        last_request_end_time = last_sound_end_time;
                    } else {
                        // *** IDLE SES AYARLAMA ***
                        TickType_t time_until_next_request = actual_next_request_time - current_time;
                        TickType_t idle_duration_ticks = pdMS_TO_TICKS((int)(cached_idle_duration * 1000));
                        
                        if (time_until_next_request > idle_duration_ticks) {
                            snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", CurrentConfiguration.idleSound);
                            new_file_ready = true;
                            new_file_2_ready = false;
                            printf("[PROCESS] Idle dosyasi ayarlandi: %s\n", play_wav_file);
                            
                            last_sound_end_time = current_time + idle_duration_ticks;
                        }
                    }
                }
            } 
            else if (IdlePlayFlag && !RequestPlayFlag) { // --- Sadece Idle aktifse ---
                if (silence_period_passed && !is_sound_playing) {
                    if (!idle_duration_cached) {
                        char idle_path[256];
                        snprintf(idle_path, sizeof(idle_path), "/sdcard/%s", CurrentConfiguration.idleSound);
                        cached_idle_duration = CheckWavDuration(idle_path);
                        idle_duration_cached = true;
                    }
                    
                    // *** IDLE ONLY SES AYARLAMA ***
                    snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", CurrentConfiguration.idleSound);
                    new_file_ready = true;
                    new_file_2_ready = false;
                    printf("[PROCESS] Idle only dosyasi ayarlandi: %s\n", play_wav_file);
                    
                    last_idle_play_time = current_time;
                    last_sound_end_time = current_time + pdMS_TO_TICKS((int)(cached_idle_duration * 1000));
                }
            } 
            else if (!IdlePlayFlag && RequestPlayFlag && req_delay_ms > 0) { // --- Sadece Request aktifse ---
                TickType_t earliest_request_time_by_period = 0;
                TickType_t earliest_request_time_by_silence = last_sound_end_time + min_silence_ticks;
                TickType_t actual_next_request_time = 0;

                if (last_request_end_time == 0) {
                    earliest_request_time_by_period = current_time;
                    actual_next_request_time = current_time;
                } else {
                    earliest_request_time_by_period = last_request_end_time + request_period_ticks;
                    
                    if (earliest_request_time_by_period > earliest_request_time_by_silence) {
                        actual_next_request_time = earliest_request_time_by_period;
                    } else {
                        actual_next_request_time = earliest_request_time_by_silence;
                    }
                }

                if (current_time >= actual_next_request_time && !is_sound_playing && silence_period_passed) {
                    // *** REQUEST ONLY SES 1 + 2 AYARLAMA ***
                    snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", CurrentConfiguration.reqSound1);
                    new_file_ready = true;
                    printf("[PROCESS] Request only ses 1 ayarlandi: %s\n", play_wav_file);
                    
                    float duration1 = CheckWavDuration(play_wav_file);
                    TickType_t total_sound_duration = pdMS_TO_TICKS((int)(duration1 * 1000));

                    if (strcmp(CurrentConfiguration.reqSound2, "-") != 0) {
                        snprintf(play_wav_file_2, sizeof(play_wav_file_2), "/sdcard/%s", CurrentConfiguration.reqSound2);
                        new_file_2_ready = true;
                        printf("[PROCESS] Request only ses 2 ayarlandi: %s\n", play_wav_file_2);
                        
                        float duration2 = CheckWavDuration(play_wav_file_2);
                        total_sound_duration += pdMS_TO_TICKS((int)(duration2 * 1000));
                        total_sound_duration += pdMS_TO_TICKS(50);
                    } else {
                        new_file_2_ready = false;
                        play_wav_file_2[0] = '\0';
                    }
                    
                    last_sound_end_time = current_time + total_sound_duration;
                    last_request_end_time = last_sound_end_time;
                }
            } 
            else if (RequestPlayFlag && CurrentConfiguration.reqPlayPeriod == 0 && silence_period_passed && !is_sound_playing) { // --- Tek seferlik request ---
                // *** TEK SEFERLİK REQUEST SES 1 + 2 AYARLAMA ***
                snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", CurrentConfiguration.reqSound1);
                new_file_ready = true;
                printf("[PROCESS] Tek seferlik ses 1 ayarlandi: %s\n", play_wav_file);
                
                float duration1 = CheckWavDuration(play_wav_file);
                TickType_t total_sound_duration = pdMS_TO_TICKS((int)(duration1 * 1000));

                if (strcmp(CurrentConfiguration.reqSound2, "-") != 0) {
                    snprintf(play_wav_file_2, sizeof(play_wav_file_2), "/sdcard/%s", CurrentConfiguration.reqSound2);
                    new_file_2_ready = true;
                    printf("[PROCESS] Tek seferlik ses 2 ayarlandi: %s\n", play_wav_file_2);
                    
                    float duration2 = CheckWavDuration(play_wav_file_2);
                    total_sound_duration += pdMS_TO_TICKS((int)(duration2 * 1000));
                    total_sound_duration += pdMS_TO_TICKS(50);
                } else {
                    new_file_2_ready = false;
                    play_wav_file_2[0] = '\0';
                }

                last_sound_end_time = current_time + total_sound_duration;
                RequestPlayFlag = false;
            }

            // *** GREEN SOUND AYARLAMA ***
            if (green_input.confirmed_flag == true && !isPlayGreenLightVoice && 
                strcmp(CurrentConfiguration.greenSound, "-") != 0) {
                
                float max_factor = ((float)CurrentConfiguration.greenMaxVolume / 100.0f) * MAX_VOLUME_FACTOR;
                float min_factor = ((float)CurrentConfiguration.greenMinVolume / 100.0f) * MAX_VOLUME_FACTOR;
                volume_factor = ((float)(g_adcAverage - MIN_ADC_VALUE) / (MAX_ADC_VALUE - MIN_ADC_VALUE)) * 
                               (max_factor - min_factor) + min_factor;
                
                printf("GreenSound ayarlaniyor: %s\n", CurrentConfiguration.greenSound);
                snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", CurrentConfiguration.greenSound);
                new_file_ready = true;
                new_file_2_ready = false;
                printf("[PROCESS] Green dosyasi ayarlandi: %s\n", play_wav_file);
                
                isPlayGreenLightVoice = true;
            }
            
            if (green_input.confirmed_flag == false && isPlayGreenLightVoice == true) {
                isPlayGreenLightVoice = false;
            }

            // *** GREEN COUNTER AYARLAMA ***
            if (GreenCounterPlayFlag == true && new_file_available == true) {
                new_file_available = false;
                
                float max_factor = ((float)CurrentConfiguration.greenMaxVolume / 100.0f) * MAX_VOLUME_FACTOR;
                float min_factor = ((float)CurrentConfiguration.greenMinVolume / 100.0f) * MAX_VOLUME_FACTOR;
                volume_factor = ((float)(g_adcAverage - MIN_ADC_VALUE) / (MAX_ADC_VALUE - MIN_ADC_VALUE)) * 
                               (max_factor - min_factor) + min_factor;

                if (green_input.countdown_current <= CurrentConfiguration.greenCountFrom && 
                    green_input.countdown_current > CurrentConfiguration.greenCountTo - 1) {
                    
                    printf("Sayim ayarlaniyor: %s\n", current_playing_file);
                    snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", current_playing_file);
                    new_file_ready = true;
                    new_file_2_ready = false;
                    printf("[PROCESS] Counter dosyasi ayarlandi: %s\n", play_wav_file);

                    while (!new_file_available && green_input.isCountdown_active) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }
            }
            
            // *** GREEN ACTION AYARLAMA ***
            if ((strcmp(CurrentConfiguration.greenAction, "-") != 0) && 
                isGreenCountdownAction == true && green_input.confirmed_flag == false) {
                
                printf("Yesil bitis sesi ayarlaniyor: %s\n", CurrentConfiguration.greenAction);
                snprintf(play_wav_file, sizeof(play_wav_file), "/sdcard/%s", CurrentConfiguration.greenAction);
                new_file_ready = true;
                new_file_2_ready = false;
                printf("[PROCESS] Green action dosyasi ayarlandi: %s\n", play_wav_file);
                
                isGreenCountdownAction = false;
            }
        }
        
    
    vTaskDelay(pdMS_TO_TICKS(10)); // CPU'yu serbest bırak
  }
}


/**
 * @brief Timer callback function triggered every 500ms.
 *
 * This function is called periodically by a FreeRTOS timer.
 * Currently, it does not implement any logic.
 *
 * @param[in] xTimer Handle to the FreeRTOS timer (unused).
 */
void TimerCallback_500ms(TimerHandle_t xTimer) {
   
}



/**
 * @brief Timer callback function triggered every 1000ms for periodic tasks.
 *
 * This function manages the countdown logic for the green signal, including silent and speaking phases,
 * and selects the appropriate audio file for playback as the countdown progresses.
 * It also updates the current active plan and handles state transitions and file availability.
 *
 * @param[in] xTimer Handle to the FreeRTOS timer (unused).
 */
void TimerCallback_1000ms(TimerHandle_t xTimer) {

  //   ESP_LOGI("RTC", "Time: %02d:%02d:%02d Date: %02d/%02d/20%02d",
  //   DeviceTime.hours, DeviceTime.minutes, DeviceTime.seconds,
  //   DeviceTime.day, DeviceTime.month, DeviceTime.year);

  // printf("Thread1000Ms\n");
  // uint8_t active_plan = get_active_plan_index(&Config, &DeviceTime);
  // ESP_LOGI("PLAN", "Aktif plan: %d", active_plan);

  //      //GERI SAYIM...
  if (green_input.isCountdown_active) {
    static uint32_t current_count = 0;
    static bool silent_phase = true;

    // İlk çalışmada başlangıç değerlerini ayarla
    if (current_count == 0) {
      current_count = green_input.countdown_current;
      silent_phase = (current_count > ui8_greenCountFrom);
//      printf("[TIMER] baslangic: %" PRIu8 "->%" PRIu8 " (Su an: %" PRIu32 ")\n",
//             ui8_greenCountFrom, ui8_greenCountTo, current_count);
    }

    // Sayım devam ediyorsa
    if (current_count > ui8_greenCountTo) {
      current_count--;

      // Sessiz fazdan konuşma fazına geçiş kontrolü
      if (silent_phase && current_count <= ui8_greenCountFrom) {
        silent_phase = false;
//        printf("[TIMER] Konusma fazi basladi (%" PRIu32 ")\n", current_count);
      }

      // Yalnızca konuşma fazındaysa dosya adını al
      if (!silent_phase) {
        const char *new_file_path = get_audio_file_path(current_count);

        if (new_file_path != NULL) {
          if (strcmp(new_file_path, current_playing_file) != 0) {
            strncpy(current_playing_file, new_file_path,
                    sizeof(current_playing_file) - 1);
            current_playing_file[sizeof(current_playing_file) - 1] = '\0';
            new_file_available = true;
          }
        }
      }
      green_input.countdown_current = current_count;
    } else {
      // Sayım tamamlandı
      green_input.isCountdown_active = false;
      current_count = 0;
//      printf("[TIMER] Sayim tamamlandi (%" PRIu8 "e ulasildi)\n",
//             ui8_greenCountTo);
    }
  }

  // Aktif plan'a göre ses seviyesini uygula:
  // uint8_t minVol = Config.plans[active_plan].min_volume;
  // uint8_t maxVol = Config.plans[active_plan].max_volume;
  // saniyede 1 kez süreyi rtc modulunden alarak gunceller

  GetCurrentPlan();

  //    printf("Device Time:\n");
  //    printf("  Year        : %02d\n", DeviceTime.year);
  //    printf("  Month       : %02d\n", DeviceTime.month);
  //    printf("  Day         : %02d\n", DeviceTime.day);
  //    printf("  Day of Week : %d\n", DeviceTime.day_of_week); // 0: Sunday, 1:
  //    Monday, ..., 6: Saturday printf("  Hours       : %02d\n",
  //    DeviceTime.hours); printf("  Minutes     : %02d\n", DeviceTime.minutes);
  //    printf("  Seconds     : %02d\n", DeviceTime.seconds);

  DeviceTime.seconds++;

  if (DeviceTime.seconds >= 60) {
    DeviceTime.seconds = 0;
    DeviceTime.minutes++;

    if (DeviceTime.minutes >= 60) {
      DeviceTime.minutes = 0;
      DeviceTime.hours++;

      if (DeviceTime.hours >= 24) {
        DeviceTime.hours = 0;
        DeviceTime.day++;
        DeviceTime.day_of_week = (DeviceTime.day_of_week % 7) + 1;

        // Ay gün sayısı kontrolü
        uint8_t days_in_month;
        switch (DeviceTime.month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
          days_in_month = 31;
          break;
        case 4:
        case 6:
        case 9:
        case 11:
          days_in_month = 30;
          break;
        case 2:
          // Basit artık yıl kontrolü (2000 yılı sonrası için)
          days_in_month = (DeviceTime.year % 4 == 0) ? 29 : 28;
          break;
        default:
          days_in_month = 30; // geçersiz ayda varsayılan
          break;
        }

        if (DeviceTime.day > days_in_month) {
          DeviceTime.day = 1;
          DeviceTime.month++;

          if (DeviceTime.month > 12) {
            DeviceTime.month = 1;
            DeviceTime.year++;
          }
        }
      }
    }
  }
}



/**
 * @brief FreeRTOS task for polling Mongoose events.
 *
 * This task repeatedly calls mongoose_poll() to process network events and timers.
 * It runs in an infinite loop with a 1ms delay between polls.
 *
 * @param[in] pvParameters Pointer to task parameters (unused).
 */
void mongoose_task(void *pvParameters) {
	
    while (1) {
		 mongoose_poll();
		 vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}



/**
 * @brief FreeRTOS task for handling IO operations, including ADC sampling and pedestrian button logic.
 *
 * This task performs periodic ADC readings, updates a noise level history array, and manages GPIO feedback/input routines for pedestrian signaling.
 * It scales and stores the ADC average, updates demand states based on button presses, and clears timing if lamp states meet certain conditions.
 * When a write operation is requested, it deinitializes the ADC and prepares for configuration writing.
 *
 * @param[in] pvParameters Pointer to task parameters (unused).
 */
void IO_Task(void *pvParameters) {

  while (1) {

    if (StopIO_Threat == false && StartWriteConfigs == false) {
      /**************ADC**************/
      // ADC oku ve diziye ekle
      uint32_t sample = ADC_Read_Average(ADC_SAMPLE_COUNT);
      memmove(&adc_samples[0], &adc_samples[1],
              (ADC_AVERAGE_COUNT - 1) * sizeof(uint32_t));
      // Yeni değeri dizinin sonuna ekle
      adc_samples[ADC_AVERAGE_COUNT - 1] = sample;
      // Her seferinde ortalamayı yeniden hesapla
      uint64_t sum =
          0; // Toplam değeri için daha büyük bir tip kullanmak taşmayı önler.
      for (int i = 0; i < ADC_AVERAGE_COUNT; i++) {
        sum += adc_samples[i];
      }
      g_adcAverage = sum / ADC_AVERAGE_COUNT;
      
      // *** NEW LOGIC FOR HISTORY UPDATE (Once per second) ***
      s_adc_update_counter++;
      if (s_adc_update_counter >=
          UPDATES_PER_SECOND) {   // True every 1 second (every 20th call)
        s_adc_update_counter = 0; // Reset counter for the next second
                                  // Scale g_adcAverage to 0-150 range
        double scaled_value_double = (double)g_adcAverage * (150.0 / 4095.0);
        int current_scaled_value = (int)round(scaled_value_double);

        // Ensure value stays within 0-150 bounds
        if (current_scaled_value < 0)
          current_scaled_value = 0;
        if (current_scaled_value > 150)
          current_scaled_value = 150;

        memmove(&noise_level_history[0], &noise_level_history[1],
                (NOISE_LEVEL_HISTORY_SIZE - 1) * sizeof(int));
        noise_level_history[NOISE_LEVEL_HISTORY_SIZE - 1] =
            current_scaled_value;

        // ESP_LOGI("TimerCallback", "Gecmis dizi guncellendi. Son eklenen
        // deger: %d", current_scaled_value);
      }

      /**************GPIO**************/
      // DetectFeedBack(&User_button_input);
      // DetectFeedBack(&No_demand_input);
      DetectPedestrianDemandFeedback();
      //DetectGreenFeedback();
      //DetectRedFeedback();
      //TrackInputRequest(&green_input); // <-- Talep süresi takibi, IO 35
      //TrackInputRequest(&red_input);   // <-- Talep süresi takib, IO 32

      
      if (green_input.isCountdown_active == true) {
        isGreenCountdownAction = true;
      }

      if (red_input.isCountdown_active == false && green_input.isCountdown_active == false) // iki lamba da kapaliysa sureleri sil...
      {
        //			red_input.countdown_current = 0;
        //		    green_input.countdown_current = 0;
        //
        //		    s_deviceStatus.redTime = 0;
        //		    s_deviceStatus.greenTime = 0;
        //
        //		    red_input.total_flash_time = 0;
        //		    green_input.total_flash_time = 0;
        //
        //		    red_input.total_steady_time = 0;
        //		    green_input.total_steady_time = 0;
        //  printf(">>> Both lamps OFF, cleared all times <<<\n");
      }
      if (red_input.total_flash_time > 20000 ||
          green_input.total_flash_time >
              20000) // iki lambadan birisi uzun sure flash yaptiysa sureleri sil...
      {
        //			red_input.countdown_current = 0;
        //		    green_input.countdown_current = 0;
        //
        //		    s_deviceStatus.redTime = 0;
        //		    s_deviceStatus.greenTime = 0;
        //
        //		    red_input.total_flash_time = 0;
        //		    green_input.total_flash_time = 0;
        //
        //		    red_input.total_steady_time = 0;
        //		    green_input.total_steady_time = 0;
        //
        //		    red_input.counting = false;
        //		    green_input.counting = false;
        //   printf(">>> Flash exceeded 20s, cleared all times <<<\n");
      }

      // Talep algılandıysa ve henüz yeşil ışık aktif değilse
      if (Demand_input.confirmed_flag && !Demand_input.isDemandActive &&
          !green_input.confirmed_flag) {
        Demand_input.isDemandActive =
            true; // yesil yanmiyorsa ve butona basildiysa talebi al.
        printf("Buton talebi algilandi.\n");
      }
      // Yeşil ışık aktifse ve daha önce talep edilmişse
      if (green_input.confirmed_flag) {
        Demand_input.isDemandActive = false; // talebi yesil yanan ve sil.
        printf("Buton talebi temizlendi.\n");
      }
    }

    if (StopIO_Threat == true && StartWriteConfigs == false) {
      DeInitADC();
      printf("ADC DEINIT\n");
      StartWriteConfigs = true;
    }
    vTaskDelay(50);
  }
}






/**
 * @brief FreeRTOS task for playing WAV audio files.
 *
 * This task checks if a new audio file (or two sequential files) is ready to play.
 * It copies the file paths, plays the first audio file, and if a second file is ready, waits 50ms and plays the second file.
 * Playback duration is measured and printed for each file.
 * The audio_playing flag is set while playback is active and reset after both files are played.
 *
 * @param[in] pvParameters Pointer to task parameters (unused).
 */
void PlayWav_Task(void *pvParameters) {
    char local_file_path[60];
    char local_file_path_2[60];
    
    while (1) {
        // Yeni dosya hazır mı kontrol et
        if (new_file_ready && !audio_playing) {
            // Dosya yollarını kopyala
            strcpy(local_file_path, play_wav_file);
            bool has_second_file = new_file_2_ready;
            if (has_second_file) {
                strcpy(local_file_path_2, play_wav_file_2);
            }
            
            new_file_ready = false;
            new_file_2_ready = false;
            audio_playing = true;
            
            // İLK SES DOSYASINI ÇAL
            printf("[AUDIO TASK] Caliniyor (1/%s): %s\n", 
                   has_second_file ? "2" : "1", local_file_path);
            uint32_t start_time = esp_timer_get_time() / 1000;
            
            play_wav(local_file_path);
            
            uint32_t end_time = esp_timer_get_time() / 1000;
            printf("[AUDIO TASK] Tamamlandi (1): %s (sure: %" PRIu32 " ms)\n", 
                   local_file_path, end_time - start_time);
            
            // İKİNCİ SES VARSA ONU DA ÇAL
            if (has_second_file && strlen(local_file_path_2) > 0) {
                printf("[AUDIO TASK] Ara gecis... (50ms)\n");
                vTaskDelay(pdMS_TO_TICKS(50));  // İki ses arası bekle
                
                printf("[AUDIO TASK] Caliniyor (2/2): %s\n", local_file_path_2);
                uint32_t start_time_2 = esp_timer_get_time() / 1000;
                
                play_wav(local_file_path_2);
                
                uint32_t end_time_2 = esp_timer_get_time() / 1000;
                printf("[AUDIO TASK] Tamamlandi (2): %s (sure: %" PRIu32 " ms)\n", 
                       local_file_path_2, end_time_2 - start_time_2);
                
                printf("[AUDIO TASK] REQUEST SEQUENCE TAMAMLANDI!\n");
            }
            
            audio_playing = false; // Tüm sesler bitti
            printf("[AUDIO TASK] Audio playing = false\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}



/**
 * @brief FreeRTOS task for saving configuration and system settings to NVS flash.
 *
 * This task checks various flags to determine which settings or configuration pages need to be written to flash.
 * It performs writing for configuration, system info, WiFi settings, audio configuration, user login info, calendar, and network settings.
 * If certain settings are updated (e.g., WiFi or network), the MCU is reset automatically to apply changes.
 * Also, if an OTA update is completed, the MCU is restarted.
 *
 * @param[in] pvParameters Pointer to task parameters (unused).
 */

void FlashWrite_task(void *pvParameters) {
  while (1) {
    // ESP_LOGI("FlashWrite_task", "Task dongude calisiyor...");
    if (StopIO_Threat == true && StartWriteConfigs == true) {
      bool ResetMCU = false;
      ESP_LOGI("FlashWrite_task", "Flash yazma sartlari saglandi.");
      // portENTER_CRITICAL(&flash_mux1);
      ESP_LOGI("FlashWrite_task", "Flash mutex alindi.");
      nvs_handle_t nvs_handle;
      ESP_LOGI("FLASH", "nvs_open baslatiliyor...");
      esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);

      if (err == ESP_OK) {
        ESP_LOGI("FLASH", "nvs_open basarili.");
        if (ConfigurationPageRequest == true) {
          ESP_LOGI("FLASH", "Konfigurasyon yaziliyor...");
          // Tüm konfigürasyonları tek bir struct'ta birleştirerek yaz
          nvs_set_blob(nvs_handle, "defconf", &s_defaultConfiguration,
                       sizeof(struct defaultConfiguration));
          nvs_set_blob(nvs_handle, "alt1conf", &s_alt1Configuration,
                       sizeof(struct alt1Configuration));
          nvs_set_blob(nvs_handle, "alt2conf", &s_alt2Configuration,
                       sizeof(struct alt2Configuration));
          nvs_set_blob(nvs_handle, "alt3conf", &s_alt3Configuration,
                       sizeof(struct alt3Configuration));
          ESP_LOGI("FLASH", "Konfigurasyonlar yazildi");

          ConfigurationPageRequest = false;
        }
        if (SettingsPageSystemInfoRequest == true) {
          ESP_LOGI("FLASH", "Sistem bilgileri yaziliyor...");
          nvs_set_blob(nvs_handle, "device_name", &s_systemInfo.deviceName,
                       sizeof(s_systemInfo.deviceName));
          nvs_set_blob(nvs_handle, "device_comment",
                       &s_systemInfo.deviceComment,
                       sizeof(s_systemInfo.deviceComment));

          ESP_LOGI("FLASH", "Device name ve comment yazildi");
          SettingsPageSystemInfoRequest = false;
        }
        if (SettingsPageWifiSettingsRequest == true) {
          ESP_LOGI("FLASH", "WiFi ayarlari yaziliyor...");
          nvs_set_blob(nvs_handle, WIFI_SSID_KEY, &s_wifiSettings.ssid,
                       strlen(s_wifiSettings.ssid) + 1);
          ResetMCU = true;
          printf("Yeni WiFi ayarlari kaydedildi. Yeniden baslatiliyor...\n");
          vTaskDelay(pdMS_TO_TICKS(200));
          SettingsPageWifiSettingsRequest = false;
        }
        if (AudioConfigurationPageRequest == true) {
          ESP_LOGI("FLASH", "Ses konfigurasyonlari yaziliyor...");
          nvs_set_blob(nvs_handle, "audio_configs", &s_audioConfig,
                       sizeof(struct audioConfig));
          ESP_LOGI("FLASH", "Ses konfigurasyonu yazildi.");
          AudioConfigurationPageRequest = false;
        }
        if (SettingsPageLoginInfoChange == true) {
          ESP_LOGI("FLASH", "Yeni kullanici ve sifre yaziliyor...");
          nvs_set_blob(nvs_handle, "user_name_pass", &s_security,
                       sizeof(struct security));
          ESP_LOGI("FLASH", "Yeni kullanici ve sifre yazildi.");
          SettingsPageLoginInfoChange = false;
        }
        if (CalendarPageRequest == true) {
          ESP_LOGI("FLASH", "Takvim yaziliyor...");
          // Tüm konfigürasyonları tek bir struct'ta birleştirerek yaz
          nvs_set_blob(nvs_handle, "sunday", &s_sunday, sizeof(struct sunday));
          nvs_set_blob(nvs_handle, "monday", &s_monday, sizeof(struct monday));
          nvs_set_blob(nvs_handle, "tuesday", &s_tuesday,
                       sizeof(struct tuesday));
          nvs_set_blob(nvs_handle, "wednesday", &s_wednesday,
                       sizeof(struct wednesday));
          nvs_set_blob(nvs_handle, "thursday", &s_thursday,
                       sizeof(struct thursday));
          nvs_set_blob(nvs_handle, "friday", &s_friday, sizeof(struct friday));
          nvs_set_blob(nvs_handle, "saturday", &s_saturday,
                       sizeof(struct saturday));
          nvs_set_blob(nvs_handle, "holidays", &s_holidays,
                       sizeof(struct holidays));
          ESP_LOGI("FLASH", "Takvim yazildi");
          CalendarPageRequest = false;
        }
        if (NetworkSettingsPageRequest == true) {
          ESP_LOGI("FLASH", "Network ayarlari yaziliyor...");
          esp_err_t err =
              nvs_set_blob(nvs_handle, "network_settings", &s_network_settings,
                           sizeof(struct network_settings));
          if (err == ESP_OK) {
            ESP_LOGI("FLASH", "Network ayarlari yazildi");
          } else {
            ESP_LOGE("FLASH", "Network ayarlari yazilamadi: %s",
                     esp_err_to_name(err));
          }

          NetworkSettingsPageRequest = false;
          ResetMCU = true;
        }
        ESP_LOGI("FLASH", "nvs_commit cagiriliyor...");
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
          ESP_LOGE("FLASH", "nvs_commit basarisiz: %s", esp_err_to_name(err));
        } else {
          ESP_LOGI("FLASH", "nvs_commit basarili.");
        }

        ESP_LOGI("FLASH", "nvs_close cagiriliyor...");
        nvs_close(nvs_handle);

        if (ResetMCU == true) {
          esp_restart();
        }
      } else {
        ESP_LOGE("FLASH", "nvs_open basarisiz: %s", esp_err_to_name(err));
      }

      // portEXIT_CRITICAL(&flash_mux1);
      ESP_LOGI("FlashWrite_task", "Flash mutex birakildi.");
      StartWriteConfigs = false;
      StopIO_Threat = false;
    }
    if (isOtaDone == true) {
      vTaskDelay(200);
      esp_restart();
    }
    vTaskDelay(100);
  }
}

 