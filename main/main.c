 
#include "main.h"
#include "MichADCRead.h"
#include "Thread.h"
#include "arch/sys_arch.h"
#include "mongoose_glue.h"
#include "sdkconfig.h"
#include "DetectTraffic.h"
#include "wifi.h"
#include "FlashConfig.h"
#include "SystemTime.h"
#include "mongoose.h"
#include "SpeakerDriver.h"
#include "SystemTime.h"
#include "Plan.h"
#include "esp_task_wdt.h"

uint8_t eth_port_cnt = 0;
esp_eth_handle_t *eth_handles = NULL;


#define WIFI_SSID_MG "TETRAHGS IT"
#define WIFI_PASS_MG "GTU!TETRA2021"
#define WIFI_SSID_MG_FABRIKA "intetra-Personel"
#define WIFI_PASS_MG_FABRIKA "INT!per13249*"

// ADC için mutex
SemaphoreHandle_t adc_mutex = NULL;

void app_main(void) {
    FlashInit();
    loadConfigurationsFromFlash();
	init_sd_card();
    GPIO_Init();
    ADC_Read_Init();

    ResetAllTrafficVariables();
	i2c_master_init();  //RTC module
	mcp7940n_get_time(&DeviceTime); 
	 
    //ETHapp_main();
    //loadWifiSettings(); 
    
    
    wifi_init(WIFI_SSID_MG, WIFI_PASS_MG);
    mongoose_init();
    Timer_Threads_Init();
    //xTimerStart(xTimer_500ms, 0);
    xTimerStart(xTimer_1000ms, 0);  //RTC, Countdown Timer
    xTaskCreate(IO_Task, "IO_Task", 1024*4, NULL, 10, &xIO_TaskHandle);
    xTaskCreate(mongoose_task, "mongoose_task", 1024*12, NULL, 10, &mongoose_task_handle);
    //xTaskCreate(ProcessThread, "ProcessThread", 1024*8, NULL, 1, &process_task_handle);
    xTaskCreate(play_wav_task, "play_wav_task", 1024*8, NULL, 3, &play_wav_task_handle);
    xTaskCreate(FlashWrite_task, "FlashWrite_task", 1024*4, NULL, 24, &flashWrite_task_handle);
}


