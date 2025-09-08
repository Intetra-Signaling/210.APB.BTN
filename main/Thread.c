/*
 * Thread.c
 *
 *  Created on: 15 Nis 2025
 *      Author: metesepetcioglu
 */
#include "Thread.h"
#include "esp_timer.h"
#include "main.h"
#include "DetectTraffic.h"
#include <stdint.h>
#include <stdio.h>
#include "SpeakerDriver.h"
#include "MichADCRead.h"
#include "mongoose.h"
#include "SystemTime.h"
#include "mongoose_glue.h"
#include "portmacro.h"
#include "wifi.h"
#include "Plan.h"
#include "stdbool.h"
#include "Alarms.h"
#include "FlashConfig.h"
#include "esp_task_wdt.h"
#include "Plan.h"

#define COUNTDOWN_STEP 1000  // 1 saniye = 1000 ms
#define COUNTDOWN_END 0
uint32_t COUNTDOWN_START;
#define GPIO_INPUT_IO_0     CONFIG_GPIO_INPUT_0
#define GPIO_INPUT_IO_1     CONFIG_GPIO_INPUT_1
#define MIN_GAP_MS 200


#define MAX_ADC_VALUE 4048.0f
#define MAX_VOLUME_FACTOR 0.5f
#define MIN_ADC_VALUE 0.0f
#define MIN_VOLUME_FACTOR 0.0f


#define NOISE_LEVEL_HISTORY_SIZE 15 // Son 15 saniyelik veriyi sakla
#define ADC_UPDATE_INTERVAL_MS 100 // 1 saniyelik periyotla adc verisini goster.
#define TIMER_CALLBACK_INTERVAL_MS 50 // Your current timer interval
#define UPDATES_PER_SECOND (ADC_UPDATE_INTERVAL_MS / TIMER_CALLBACK_INTERVAL_MS) // 1000ms / 50ms = 20

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
extern volatile float volume_factor;  // Başlangıç seviyesi
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
volatile uint32_t wav_play_delay_ms = 30;  // Başlangıçta 150ms
extern volatile bool s_stopADC;
extern bool  StartWriteConfigs;
extern bool   StopIO_Threat;


extern bool ConfigurationPageRequest;
extern bool SettingsPageSystemInfoRequest;
extern bool SettingsPageWifiSettingsRequest;
extern bool AudioConfigurationPageRequest;
extern bool CalendarPageRequest;
extern struct xCurrentConfiguration CurrentConfiguration;

SemaphoreHandle_t xFlashMutex = NULL; 
 
// Kritik bölge için gerekli değişken
portMUX_TYPE flash_mux1 = portMUX_INITIALIZER_UNLOCKED;
// ADC için mutex
extern SemaphoreHandle_t adc_mutex;

// This function would typically be called once during system initialization
void init_nvs_mutex(void) {
    if (xFlashMutex == NULL) {
        xFlashMutex = xSemaphoreCreateMutex();
        if (xFlashMutex == NULL) {
            ESP_LOGE(TAG_GLUE, "Failed to create global NVS mutex!");
            // Handle fatal error, e.g., esp_restart();
        }
    }
 }
 
void Timer_Threads_Init(void)
{
  
    // 500 ms timer
    xTimer_500ms = xTimerCreate(
        "Timer_500ms",
        pdMS_TO_TICKS(500),
        pdTRUE,
        NULL,
        TimerCallback_500ms
    );

    // 1000 ms (1 saniye) timer
    xTimer_1000ms = xTimerCreate(
        "Timer_1000ms",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        TimerCallback_1000ms
    );
}



/*******************************************************************************
* Function Name  			: ProcessThread 
* Description    			: ProcessThread 
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void ProcessThread(void *arg) { 
	
     while (1) {
	
    //mcp7940n_get_time(&DeviceTime);
    
    vTaskDelay(pdMS_TO_TICKS(1000)); 
	

    //play_wav_dac("/sdcard/default_4.wav");
    //play_wav("/sdcard/default_4.wav");
	 
	
	
//	// Get the current alarm count
//    uint8_t count = Alarm_GetCount();
//    printf("Total alarms in system: %d\n", count);
//    
//    // Determine how many alarms to show (up to 50)
//    uint8_t show_count = (count > 50) ? 50 : count;
//    
//    if (show_count == 0) {
//        printf("No alarms to display\n");
//        vTaskDelete(NULL);
//        return;
//    }
//    
//    // Print all alarms (or up to 50)
//    printf("\n==== Displaying %d Alarms ====\n", show_count);
//    for (uint8_t i = 0; i < show_count; i++) {
//        const AlarmEntry_t* alarm = Alarm_GetLog(i);
//        if (alarm != NULL) {
//            printf("%02d: %02d/%02d/20%02d %02d:%02d:%02d - %s\n",
//                   i+1,
//                   alarm->day, alarm->month, alarm->year,
//                   alarm->hours, alarm->minutes, alarm->seconds,
//                   alarm->message);
//           }
//    }
  }  
  
}

/*******************************************************************************
* Function Name  			: TimerCallback_500ms 
* Description    			: TimerCallback_500ms 
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/ 
void TimerCallback_500ms(TimerHandle_t xTimer) {
//    static bool flashState = false;  // LED durumu takipçisi
//    flashState = !flashState;  // Durumu tersine çevir (toggle)
//    gpio_set_level(Button_Pin, flashState ? 1 : 0);  // LED'i aç/kapat
//        printf("[FLASH] LED: %s\n", flashState ? "ON" : "OFF");
}

/*******************************************************************************
* Function Name  			: TimerCallback_1000ms 
* Description    			: TimerCallback_1000ms 
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
char current_playing_file[32] = {0};
bool new_file_available = false;

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
        if(current_count == 0) {
            current_count = green_input.countdown_current;
            silent_phase = (current_count > ui8_greenCountFrom);
            printf("[TIMER] baslangic: %" PRIu8 "->%" PRIu8 " (Su an: %" PRIu32 ")\n",
                  ui8_greenCountFrom, ui8_greenCountTo, current_count);
        }
        
        // Sayım devam ediyorsa
        if(current_count > ui8_greenCountTo) {
            current_count--;
            
            // Sessiz fazdan konuşma fazına geçiş kontrolü
            if(silent_phase && current_count <= ui8_greenCountFrom) {
                silent_phase = false;
                printf("[TIMER] Konusma fazi basladi (%" PRIu32 ")\n", current_count);
            }
            
              // Yalnızca konuşma fazındaysa dosya adını al
            if(!silent_phase) {
                const char* new_file_path = get_audio_file_path(current_count);

                if (new_file_path != NULL) {
                    if(strcmp(new_file_path, current_playing_file) != 0) {
                        strncpy(current_playing_file, new_file_path, sizeof(current_playing_file) - 1);
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
            printf("[TIMER] Sayim tamamlandi (%" PRIu8 "e ulasildi)\n", ui8_greenCountTo);
        }
    }
    
 //Aktif plan'a göre ses seviyesini uygula:
 //uint8_t minVol = Config.plans[active_plan].min_volume;
 //uint8_t maxVol = Config.plans[active_plan].max_volume;
 //saniyede 1 kez süreyi rtc modulunden alarak gunceller
 
 
    GetCurrentPlan();
    
//    printf("Device Time:\n");
//    printf("  Year        : %02d\n", DeviceTime.year);
//    printf("  Month       : %02d\n", DeviceTime.month);
//    printf("  Day         : %02d\n", DeviceTime.day);
//    printf("  Day of Week : %d\n", DeviceTime.day_of_week); // 0: Sunday, 1: Monday, ..., 6: Saturday
//    printf("  Hours       : %02d\n", DeviceTime.hours);
//    printf("  Minutes     : %02d\n", DeviceTime.minutes);
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
                    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                        days_in_month = 31; break;
                    case 4: case 6: case 9: case 11:
                        days_in_month = 30; break;
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

/*******************************************************************************
* Function Name  			: mongoose thread
* Description    			: mongoose thread
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void mongoose_task(void *pvParameters) {
	
    while (1) {
		 mongoose_poll();
		 vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}

/*******************************************************************************
* Function Name  			: mongoose thread
* Description    			: mongoose thread
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/


void IO_Task(void *pvParameters) {
			
    while (1) {
		
		if(StopIO_Threat == false && StartWriteConfigs == false)
		{
		/**************ADC**************/ 
        // ADC oku ve diziye ekle
		uint32_t sample = ADC_Read_Average(ADC_SAMPLE_COUNT);
	    memmove(&adc_samples[0], &adc_samples[1], (ADC_AVERAGE_COUNT - 1) * sizeof(uint32_t));
		// Yeni değeri dizinin sonuna ekle
		adc_samples[ADC_AVERAGE_COUNT - 1] = sample;
		// Her seferinde ortalamayı yeniden hesapla
		uint64_t sum = 0; // Toplam değeri için daha büyük bir tip kullanmak taşmayı önler.
		for (int i = 0; i < ADC_AVERAGE_COUNT; i++) {
		    sum += adc_samples[i];
		}
		g_adcAverage = sum / ADC_AVERAGE_COUNT;

      
//	    volume_factor = ((float)(g_adcAverage - MIN_ADC_VALUE) / (MAX_ADC_VALUE - MIN_ADC_VALUE)) 
//	                    * (MAX_VOLUME_FACTOR - MIN_VOLUME_FACTOR) + MIN_VOLUME_FACTOR;
	                    
//      	ESP_LOGI(TAGADC, "ADC ort: %"PRIu32", volume_factor: %.3f", g_adcAverage, volume_factor); 

		 // *** NEW LOGIC FOR HISTORY UPDATE (Once per second) ***
		 s_adc_update_counter++;
		 if (s_adc_update_counter >= UPDATES_PER_SECOND) { // True every 1 second (every 20th call)
         s_adc_update_counter = 0; // Reset counter for the next second
         // Scale g_adcAverage to 0-150 range
        double scaled_value_double = (double)g_adcAverage * (150.0 / 4095.0);
        int current_scaled_value = (int)round(scaled_value_double);

        // Ensure value stays within 0-150 bounds
        if (current_scaled_value < 0) current_scaled_value = 0;
        if (current_scaled_value > 150) current_scaled_value = 150; 
     
        memmove(&noise_level_history[0], &noise_level_history[1], (NOISE_LEVEL_HISTORY_SIZE - 1) * sizeof(int));
        noise_level_history[NOISE_LEVEL_HISTORY_SIZE - 1] = current_scaled_value;
        
       // ESP_LOGI("TimerCallback", "Gecmis dizi guncellendi. Son eklenen deger: %d", current_scaled_value);
          }
	
        /**************GPIO**************/
         //DetectFeedBack(&User_button_input);
         //DetectFeedBack(&No_demand_input);
         DetectPedestrianDemandFeedback();
 		 DetectGreenFeedback();
 		 DetectRedFeedback();
		 TrackInputRequest(&green_input);  // <-- Talep süresi takibi, IO 35
		 TrackInputRequest(&red_input);  // <-- Talep süresi takib, IO 32
		 
            if(isGreenCountdownAction == true)
            {
				printf("isGreenCountdownAction = true\n");
			}		
		    if(isGreenCountdownAction == false)
            {
		    printf("isGreenCountdownAction = false\n");
			}	
			
			
			if (green_input.isCountdown_active == true) 
			{
		    isGreenCountdownAction = true;
			}
			
			
		 if(red_input.isCountdown_active == false && green_input.isCountdown_active == false) // iki lamba da kapaliysa sureleri sil...
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
		  if(red_input.total_flash_time > 20000  || green_input.total_flash_time > 20000  ) // iki lambadan birisi uzun sure flash yaptiysa sureleri sil...
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
		if (Demand_input.confirmed_flag && !Demand_input.isDemandActive && !green_input.confirmed_flag) {
		    Demand_input.isDemandActive = true; //yesil yanmiyorsa ve butona basildiysa talebi al.
		     printf("Buton talebi algilandi.\n");
		}
		// Yeşil ışık aktifse ve daha önce talep edilmişse
		if (green_input.confirmed_flag) {
		    Demand_input.isDemandActive = false; // talebi yesil yanan ve sil.
		    printf("Buton talebi temizlendi.\n");
		}
		}
		
		if(StopIO_Threat == true && StartWriteConfigs == false)
		{
			DeInitADC();
            printf("ADC DEINIT\n");
			StartWriteConfigs = true;
		}
		 vTaskDelay(50);
    }
}

/*******************************************************************************
* Function Name  			: play wav thread
* Description    			: play wav thread
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
// Son çalınma zamanlarını tutan değişkenler
static TickType_t last_idle_play_time = 0;
static TickType_t last_request_play_time = 0;
static TickType_t last_play_time = 0;
TickType_t last_any_play_time = 0;
 
void play_wav_task(void *pvParameters) {
	
     // Başlangıç değerlerini ayarla
    last_idle_play_time = xTaskGetTickCount();
    last_request_play_time = xTaskGetTickCount();


    while(1) {
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
		if(CurrentConfiguration.isIdleActive == true && CurrentConfiguration.idleContAfterReq == false &&  CurrentConfiguration.isReqActive == false)
		{ 
			RequestPlayFlag = false;
			IdlePlayFlag = true;
			GreenCounterPlayFlag = false;
			//printf("100\n");
			if(Demand_input.isDemandActive == true) // talep alindiktan sonra idle sesini kes. ve yesil saymayi birakana kadar
			{
			IdlePlayFlag = false;
			}
			if(CurrentConfiguration.isGreenActive == true && green_input.confirmed_flag == true)
		    {
		     IdlePlayFlag = false;
		     RequestPlayFlag = false;
		     StopPlayWav = true;
		     GreenCounterPlayFlag = true;
 		    }
		    
  		    
		}
		// --------------------
		// CASE 2: 110
		// --------------------
		else if(CurrentConfiguration.isIdleActive == true && CurrentConfiguration.idleContAfterReq == true &&  CurrentConfiguration.isReqActive == false)
		{
			IdlePlayFlag = true;
			RequestPlayFlag = false;
			GreenCounterPlayFlag = false;
			//printf("110\n");
			if(Demand_input.isDemandActive) // talep alinmissa idle sesi devam etmeli
			{
			IdlePlayFlag = true;
			}
			if(CurrentConfiguration.isGreenActive == true && green_input.confirmed_flag == true)
		    {
		     IdlePlayFlag = false;
		     RequestPlayFlag = false;
		     StopPlayWav = true;
		     GreenCounterPlayFlag = true;
 		    }
		    
			
		}
		// --------------------
		// CASE 3: 010
		// --------------------
		else if(CurrentConfiguration.isIdleActive == false && CurrentConfiguration.idleContAfterReq == true &&  CurrentConfiguration.isReqActive == false)
		{
			IdlePlayFlag = false;
			RequestPlayFlag = false;
			GreenCounterPlayFlag = false;
			//printf("010\n");
			if(Demand_input.isDemandActive) // talep alinmissa talep gidene kadar sesini cal
			{
			IdlePlayFlag = true;
			}
			if(CurrentConfiguration.isGreenActive == true && green_input.confirmed_flag == true)
		    {
		     IdlePlayFlag = false;
		     RequestPlayFlag = false;
		     StopPlayWav = true;
		     GreenCounterPlayFlag = true;
 		    }
		    
			
		}
		// --------------------
		// CASE 4: 000
		// --------------------
		else if(CurrentConfiguration.isIdleActive == false && CurrentConfiguration.idleContAfterReq == false &&  CurrentConfiguration.isReqActive == false)
		{
			IdlePlayFlag = false;
			GreenCounterPlayFlag = false;
			//printf("000\n");
			if(Demand_input.isDemandActive)
			{
			IdlePlayFlag = false;
			RequestPlayFlag = false;
			}
			if(CurrentConfiguration.isGreenActive == true && green_input.confirmed_flag == true)
		    {
		     IdlePlayFlag = false;
		     RequestPlayFlag = false;
		     StopPlayWav = true;
		     GreenCounterPlayFlag = true;
 		    }
		}
		// --------------------
		// CASE 5: 111
		// --------------------
		else if(CurrentConfiguration.isIdleActive == true && CurrentConfiguration.idleContAfterReq == true &&  CurrentConfiguration.isReqActive == true)
		{
			IdlePlayFlag = true;
			RequestPlayFlag = false;
			GreenCounterPlayFlag = false;
			//printf("111\n");
		    if(Demand_input.isDemandActive)
			{
			IdlePlayFlag = true;
			RequestPlayFlag = true;
			StopPlayWav = true;
  			}
			if(CurrentConfiguration.isGreenActive == true && green_input.confirmed_flag == true)
		    {
		     IdlePlayFlag = false;
		     RequestPlayFlag = false;
		     StopPlayWav = true;
		     GreenCounterPlayFlag = true;
		    }
		}
		// --------------------
		// CASE 6: 001
		// --------------------
		else if(CurrentConfiguration.isIdleActive == false && CurrentConfiguration.idleContAfterReq == false &&  CurrentConfiguration.isReqActive == true)
		{
			IdlePlayFlag = false;
			GreenCounterPlayFlag = false;
			RequestPlayFlag = false;
			//printf("001\n");
			if(Demand_input.isDemandActive)
			{
			IdlePlayFlag = false;
			RequestPlayFlag = true;
			StopPlayWav = true;
  			}
			if(CurrentConfiguration.isGreenActive == true && green_input.confirmed_flag == true)
		    {
		     IdlePlayFlag = false;
		     RequestPlayFlag = false;
		     StopPlayWav = true;
		     GreenCounterPlayFlag = true;
		    }
		   
		}
		// --------------------
		// CASE 7: 011
		// --------------------
		else if(CurrentConfiguration.isIdleActive == false && CurrentConfiguration.idleContAfterReq == true &&  CurrentConfiguration.isReqActive == true)
		{
			IdlePlayFlag = false;
			GreenCounterPlayFlag = false;
			RequestPlayFlag = false;
			//printf("011\n");
		    if(Demand_input.isDemandActive)
			{
			IdlePlayFlag = true;
			RequestPlayFlag = true;
			StopPlayWav = true;
  			} 
			if(CurrentConfiguration.isGreenActive == true && green_input.confirmed_flag == true)
		    {
		     IdlePlayFlag = false;
		     RequestPlayFlag = false;
		     StopPlayWav = true;
		     GreenCounterPlayFlag = true;
		    }
		}
		// --------------------
		// CASE 8: 101
		// --------------------
		else if(CurrentConfiguration.isIdleActive == true && CurrentConfiguration.idleContAfterReq == false &&  CurrentConfiguration.isReqActive == true)
		{
			IdlePlayFlag = true;
			GreenCounterPlayFlag = false;
			RequestPlayFlag = false;
			//printf("101\n");
			if(Demand_input.isDemandActive)
			{
			IdlePlayFlag = false;
			RequestPlayFlag = true;
			StopPlayWav = true;
 			} 
			if(CurrentConfiguration.isGreenActive == true && green_input.confirmed_flag == true)
		    {
		     IdlePlayFlag = false;
		     RequestPlayFlag = false;
		     StopPlayWav = true;
		     GreenCounterPlayFlag = true;
		    }
		}

		// --------------------
		// PLAY SOUND
		// --------------------
		if(TestMode == true)
		{
		// Create the full file path correctly
        char filePath[60];  // Enough space for "/sdcard/" + filename + ".wav" + null terminator
        snprintf(filePath, sizeof(filePath), "/sdcard/%s", s_playSound.fileName);
        // Play the WAV file
        play_wav(filePath);
        TestMode = false;
		}
		
		if(TestMode == false)
		{
				TickType_t current_time = xTaskGetTickCount();	 


				// Idle sesi kontrolü
		        if (IdlePlayFlag == true ) {
					
					float max_factor = ((float)CurrentConfiguration.idleMaxVolume / 100.0f) * MAX_VOLUME_FACTOR;
					float min_factor = ((float)CurrentConfiguration.idleMinVolume / 100.0f) * MAX_VOLUME_FACTOR;
					
					volume_factor = ((float)(g_adcAverage - MIN_ADC_VALUE) / (MAX_ADC_VALUE - MIN_ADC_VALUE))  
					                    * (max_factor - min_factor) + min_factor;
	                    
	                    
		            TickType_t idle_period_ticks = pdMS_TO_TICKS(700);
		            if ((current_time - last_idle_play_time) >= idle_period_ticks &&
                        (current_time - last_any_play_time) >= pdMS_TO_TICKS(MIN_GAP_MS)) {
		                char idle_path[256];
		                snprintf(idle_path, sizeof(idle_path), "/sdcard/%s", CurrentConfiguration.idleSound);
		                printf("Idle sound playing: %s\n", idle_path);
		                play_wav_idle(idle_path);
		                vTaskDelay(10);
		                last_idle_play_time = xTaskGetTickCount();
                        last_any_play_time  = last_idle_play_time;
 
 		            }
		        }
				 // Request sesi kontrolü
		        if (RequestPlayFlag == true) {
					int req_delay_ms = 0;
				    float max_factor = ((float)CurrentConfiguration.reqMaxVolume / 100.0f) * MAX_VOLUME_FACTOR;
					float min_factor = ((float)CurrentConfiguration.reqMinVolume / 100.0f) * MAX_VOLUME_FACTOR;
					
					volume_factor = ((float)(g_adcAverage - MIN_ADC_VALUE) / (MAX_ADC_VALUE - MIN_ADC_VALUE))  
					                    * (max_factor - min_factor) + min_factor;
				    // Request periyodunu önceden hesapla
				    switch (CurrentConfiguration.reqPlayPeriod) {
				        case 1:  req_delay_ms = 500; break;
				        case 2:  req_delay_ms = 1000; break;
				        case 3:  req_delay_ms = 1500; break;
				        case 4:  req_delay_ms = 2000; break;
			            case 5:  req_delay_ms = 2500; break;
			            case 6:  req_delay_ms = 5000; break;
			            case 7:  req_delay_ms = 7500; break;
			            case 8:  req_delay_ms = 15000; break;
				        case 9:  req_delay_ms = 30000; break;
				        default: req_delay_ms = 0; break;
				    }
		            // Sadece bir kez çalma mantığı
		            if (CurrentConfiguration.reqPlayPeriod == 0 && hasRequestPlayedOnce == false) {
		                char filePath[60];
		                snprintf(filePath, sizeof(filePath), "/sdcard/%s", CurrentConfiguration.reqSound1);
		                printf("Request sound playing once\n");
		                play_wav(filePath);
		                if(strcmp(CurrentConfiguration.reqSound2, "-") != 0)
	                    {
						char filePath[60];
	                    snprintf(filePath, sizeof(filePath), "/sdcard/%s", CurrentConfiguration.reqSound2);
	                    printf("Request sound playing periyodik\n");
	                    play_wav(filePath);
						}
		                
		                hasRequestPlayedOnce = true;
		                RequestPlayFlag = false;
		            }
		            // Periyodik çalma mantığı
		            else if (CurrentConfiguration.reqPlayPeriod != 0 && req_delay_ms > 0) {
		                TickType_t request_period_ticks = pdMS_TO_TICKS(req_delay_ms);
		                current_time = xTaskGetTickCount();
		                 if ((current_time - last_request_play_time) >= request_period_ticks &&
                            (current_time - last_any_play_time) >= pdMS_TO_TICKS(MIN_GAP_MS)) {
							
		                    char filePath[60];
		                    snprintf(filePath, sizeof(filePath), "/sdcard/%s", CurrentConfiguration.reqSound1);
		                    printf("Request sound playing periyodik\n");
		                    play_wav(filePath);
		                    vTaskDelay(5);
		                    if(strcmp(CurrentConfiguration.reqSound2, "-") != 0)
		                    {
							char filePath[60];
		                    snprintf(filePath, sizeof(filePath), "/sdcard/%s", CurrentConfiguration.reqSound2);
		                    printf("Request sound playing periyodik\n");
		                    play_wav(filePath);
							}
		                    last_request_play_time = xTaskGetTickCount();
                            last_any_play_time     = last_request_play_time;
		                }
		            }
		        }
				// Play greenSound ONLY if it hasn't been played yet
				if(green_input.confirmed_flag == true && !isPlayGreenLightVoice && strcmp(CurrentConfiguration.greenSound, "-") != 0)
			    {
					float max_factor = ((float)CurrentConfiguration.greenMaxVolume / 100.0f) * MAX_VOLUME_FACTOR;
					float min_factor = ((float)CurrentConfiguration.greenMinVolume / 100.0f) * MAX_VOLUME_FACTOR;
					
					volume_factor = ((float)(g_adcAverage - MIN_ADC_VALUE) / (MAX_ADC_VALUE - MIN_ADC_VALUE))  
					                    * (max_factor - min_factor) + min_factor;
					                    
			        printf("GreenSound oynatiliyor: %s\n", CurrentConfiguration.greenSound);
			        char filePath[60];
				    snprintf(filePath, sizeof(filePath), "/sdcard/%s", CurrentConfiguration.greenSound);
				    play_wav(filePath);
				    vTaskDelay(pdMS_TO_TICKS(20));
			        isPlayGreenLightVoice = true;
			    }
			    if(green_input.confirmed_flag == false && isPlayGreenLightVoice == true)
			    {
					isPlayGreenLightVoice = false;
				}
			    
			    
				if(GreenCounterPlayFlag == true && new_file_available == true)
				{
				    new_file_available = false;	
				    char full_file_path[64] = {0};  // Adjust size if needed
				    
			        float max_factor = ((float)CurrentConfiguration.greenMaxVolume / 100.0f) * MAX_VOLUME_FACTOR;
					float min_factor = ((float)CurrentConfiguration.greenMinVolume / 100.0f) * MAX_VOLUME_FACTOR;
					
					volume_factor = ((float)(g_adcAverage - MIN_ADC_VALUE) / (MAX_ADC_VALUE - MIN_ADC_VALUE))  
					                    * (max_factor - min_factor) + min_factor;
				    // Normal countdown playback
				    
				    if(green_input.countdown_current <= CurrentConfiguration.greenCountFrom && 
				       green_input.countdown_current > CurrentConfiguration.greenCountTo - 1) 
				    {
						// Construct full path
                        snprintf(full_file_path, sizeof(full_file_path), "/sdcard/%s", current_playing_file);
				        printf("Sayim oynatiliyor: %s\n", current_playing_file);
				        play_wav_counter(full_file_path);
				        
				        while(!new_file_available && green_input.isCountdown_active) {
				            vTaskDelay(pdMS_TO_TICKS(10));
				        }
				    }
				}
				if((strcmp(CurrentConfiguration.greenAction, "-") != 0) && isGreenCountdownAction == true && green_input.confirmed_flag == false ) //yesil sonrasi ses dosyasi varsa ve yesil acildiktan sonra kapanmissa.
				{
					char filePath[60];
                    snprintf(filePath, sizeof(filePath), "/sdcard/%s", CurrentConfiguration.greenAction);
                    printf("Yesil bitis sesi calindi\n");
                    play_wav(filePath);
                    isGreenCountdownAction = false;
				}
          vTaskDelay(pdMS_TO_TICKS(10)); // CPU'yu serbest bırak
		 }
}
}
/*******************************************************************************
* Function Name  			: stopTestMode
* Description    			: stopTestMode
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void stopTestModeThread(TimerHandle_t xTimer) {
    TestMode = false;
    printf("Test modu kapatildi\n");
    // Timer'ı sil ve NULL yap
    if (xTimer != NULL) {
        xTimerDelete(xTimer, 0);
        testModeTimer = NULL;
    }
}

/*******************************************************************************
* Function Name  			: stopTestMode
* Description    			: stopTestMode
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void stopTestMode(void) {
  // Timer oluştur (eğer henüz oluşturulmadıysa)
    if (testModeTimer == NULL) {
        testModeTimer = xTimerCreate(
            "TestModeTimer",          // Timer adı
            pdMS_TO_TICKS(3000),     // 3 saniye
            pdFALSE,                 // One-shot timer
            (void*)0,                // Timer ID
            stopTestModeThread             // Callback fonksiyonu
        );
    }
     // Timer'ı başlat (yeniden)
    xTimerStart(testModeTimer, 0);
}

/*******************************************************************************
* Function Name  			: save_all_configs_task
* Description    			: save_all_configs_task
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/

void FlashWrite_task(void *pvParameters) 
{
	while(1)
	{
		//ESP_LOGI("FlashWrite_task", "Task dongude calisiyor...");
		if(StopIO_Threat == true && StartWriteConfigs == true)
		{
			bool ResetMCU = false;
			ESP_LOGI("FlashWrite_task", "Flash yazma sartlari saglandi.");
			//portENTER_CRITICAL(&flash_mux1);
			ESP_LOGI("FlashWrite_task", "Flash mutex alindi.");
		    nvs_handle_t nvs_handle;
		    ESP_LOGI("FLASH", "nvs_open baslatiliyor...");
		    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
			
            if (err == ESP_OK)
			{
				ESP_LOGI("FLASH", "nvs_open basarili.");
			if(ConfigurationPageRequest == true)
			{
				ESP_LOGI("FLASH", "Konfigurasyon yaziliyor...");
	            // Tüm konfigürasyonları tek bir struct'ta birleştirerek yaz
	            nvs_set_blob(nvs_handle, "defconf", &s_defaultConfiguration, sizeof(struct defaultConfiguration));
	            nvs_set_blob(nvs_handle, "alt1conf", &s_alt1Configuration, sizeof(struct alt1Configuration));
	            nvs_set_blob(nvs_handle, "alt2conf", &s_alt2Configuration, sizeof(struct alt2Configuration));
	            nvs_set_blob(nvs_handle, "alt3conf", &s_alt3Configuration, sizeof(struct alt3Configuration));  
	            ESP_LOGI("FLASH", "Konfigurasyonlar yazildi"); 
	      
		        ConfigurationPageRequest = false;
			}
	        if(SettingsPageSystemInfoRequest == true)
	        {
				ESP_LOGI("FLASH", "Sistem bilgileri yaziliyor...");
				nvs_set_blob(nvs_handle, "device_name", &s_systemInfo.deviceName, sizeof(s_systemInfo.deviceName));
				nvs_set_blob(nvs_handle, "device_comment", &s_systemInfo.deviceComment, sizeof(s_systemInfo.deviceComment));
				 
				ESP_LOGI("FLASH", "Device name ve comment yazildi"); 
				SettingsPageSystemInfoRequest = false;
			}
			if(SettingsPageWifiSettingsRequest == true)
			{
				ESP_LOGI("FLASH", "WiFi ayarlari yaziliyor...");
				nvs_set_blob(nvs_handle, WIFI_SSID_KEY, &s_wifiSettings.ssid, strlen(s_wifiSettings.ssid)+1);
				ResetMCU = true;
			    printf("Yeni WiFi ayarlari kaydedildi. Yeniden baslatiliyor...\n");
			    vTaskDelay(pdMS_TO_TICKS(200)); 
				SettingsPageWifiSettingsRequest = false;
			}
			if(AudioConfigurationPageRequest == true)
			{
				ESP_LOGI("FLASH", "Ses konfigurasyonlari yaziliyor...");
				nvs_set_blob(nvs_handle, "audio_configs", &s_audioConfig, sizeof(struct audioConfig));
			    ESP_LOGI("FLASH", "Ses konfigurasyonu yazildi.");
				AudioConfigurationPageRequest = false;
			}
			if(CalendarPageRequest == true)
			{
				ESP_LOGI("FLASH", "Takvim yaziliyor...");
	            // Tüm konfigürasyonları tek bir struct'ta birleştirerek yaz
	            nvs_set_blob(nvs_handle, "sunday", &s_sunday, sizeof(struct sunday));
	            nvs_set_blob(nvs_handle, "monday", &s_monday, sizeof(struct monday));
	            nvs_set_blob(nvs_handle, "tuesday", &s_tuesday, sizeof(struct tuesday));
	            nvs_set_blob(nvs_handle, "wednesday", &s_wednesday, sizeof(struct wednesday));
	            nvs_set_blob(nvs_handle, "thursday", &s_thursday, sizeof(struct thursday));
	            nvs_set_blob(nvs_handle, "friday", &s_friday, sizeof(struct friday));
	            nvs_set_blob(nvs_handle, "saturday", &s_saturday, sizeof(struct saturday));
	            nvs_set_blob(nvs_handle, "holidays", &s_holidays, sizeof(struct holidays));
	            ESP_LOGI("FLASH", "Takvim yazildi");
				CalendarPageRequest = false;
			}
			    ESP_LOGI("FLASH", "nvs_commit cagiriliyor...");
				err = nvs_commit(nvs_handle);
				if (err != ESP_OK)
				{
					ESP_LOGE("FLASH", "nvs_commit basarisiz: %s", esp_err_to_name(err));
				}
				else
				{
					ESP_LOGI("FLASH", "nvs_commit basarili.");
				}

				ESP_LOGI("FLASH", "nvs_close cagiriliyor...");
				nvs_close(nvs_handle);
			    if(ResetMCU==true)
			    {
					esp_restart();
				}
			} else
			{
				ESP_LOGE("FLASH", "nvs_open basarisiz: %s", esp_err_to_name(err));
			}
	
		  //portEXIT_CRITICAL(&flash_mux1);
		  ESP_LOGI("FlashWrite_task", "Flash mutex birakildi.");
	      StartWriteConfigs = false;
	      StopIO_Threat = false;
		}
		vTaskDelay(500);
	}
}

/*******************************************************************************
* Function Name  			: save_all_configs_task
* Description    			: save_all_configs_task
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void suspend_all_tasks_for_ota(void) {
  printf("Suspending all tasks for OTA\n");
  if (xIO_TaskHandle) vTaskSuspend(xIO_TaskHandle);
  //if (mongoose_task_handle) vTaskSuspend(mongoose_task_handle);
  if (play_wav_task_handle) vTaskSuspend(play_wav_task_handle);
  if (flashWrite_task_handle) vTaskSuspend(flashWrite_task_handle);
  // Timer'ı da durdur:
  if (xTimer_1000ms) xTimerStop(xTimer_1000ms, 0);
}
