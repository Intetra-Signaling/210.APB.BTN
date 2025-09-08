/*
 * DetectTraffic.c
 *
 *  Created on: 18 Nis 2025
 *      Author: metesepetcioglu
 */

#include "main.h"
#include "DetectTraffic.h"
#include "Alarms.h"


uint32_t PedestrianFeedback_StuckLowTimeout_min = 1; //14400;  // hiç basılmazsa
uint32_t PedestrianFeedback_StuckHighTimeout_min = 1;  // sürekli basılıysa
extern bool hasRequestPlayedOnce;



InputDebounce_t green_input = {
    .pin = Green_FB_Input_Pin,
    .level_flags = {false, false, false},
    .confirmed_flag = false,
    .timestamp = 0,
    .label = "Green",
    .waiting_for_dark = false,
    .dark_start_time = 0,
    .request_start_time = 0,
    .countdown_current = 0,
    .countdown_start = 0,
    .isCountdown_active = false

};

InputDebounce_t red_input = {
    .pin = Red_FB_Input_Pin,
    .level_flags = {false, false, false},
    .confirmed_flag = false,
    .timestamp = 0,
    .label = "Red",
    .waiting_for_dark = false,
    .dark_start_time = 0,
    .request_start_time = 0,
    .countdown_current = 0,
    .countdown_start = 0,
    .isCountdown_active = false
};


InputDebounce_t Demand_input = {
    .pin = Demand_Input_Pin,
    .confirmed_flag = false,
    .timestamp = 0,
    .label = "input_01",
    .isDemandActive = false
};

InputDebounce_t No_demand_input = {
    .pin = No_Demand_Input_Pin,
    .confirmed_flag = false,
    .timestamp = 0,
    .label = "input_02"
};

InputDebounce_t User_button_input = {
    .pin = Button_Input_Pin,
    .confirmed_flag = false,
    .timestamp = 0,
    .label = "user_input"
};


/*******************************************************************************
* Function Name  			: GPIO_Init
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/

void GPIO_Init(void)
{
	 // GPIO konfigürasyonu
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,         // Interrupt kullanılmıyor
        .mode = GPIO_MODE_INPUT,                // Sadece giriş modu
        .pin_bit_mask = (1ULL << Red_FB_Input_Pin) | (1ULL << Green_FB_Input_Pin) | (1ULL << Demand_Input_Pin)  | (1ULL << No_Demand_Input_Pin)| (1ULL << Button_Input_Pin),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  // Pull-down yok
        .pull_up_en = GPIO_PULLUP_DISABLE       // Pull-up yok (gerekiyorsa ENABLE yap)
    };
    gpio_config(&io_conf);
}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void DetectFeedBack(InputDebounce_t* input) 
{
    // Ham GPIO değerini oku
    int raw_level = gpio_get_level(input->pin);
    
    // Debug: Ham değeri yazdır
    printf("[%s] RAW GPIO VALUE: %d\n", input->label, raw_level);
    
    // Örnekleme dizisini kaydır
    bool prev_samples[3] = {
        input->level_flags[0],
        input->level_flags[1], 
        input->level_flags[2]
    };
    
    input->level_flags[0] = input->level_flags[1];
    input->level_flags[1] = input->level_flags[2];
    input->level_flags[2] = (raw_level == 1);

    // Debug: Önceki ve yeni örnekleri yazdır
    printf("[%s] ONCEKI ORNEKLER: %d-%d-%d\n", 
           input->label, prev_samples[0], prev_samples[1], prev_samples[2]);
    printf("[%s] YENI ORNEKLER: %d-%d-%d\n", 
           input->label, input->level_flags[0], input->level_flags[1], input->level_flags[2]);

    // Çoğunluk seviyesini hesapla
    int high_count = input->level_flags[0] + input->level_flags[1] + input->level_flags[2];
    bool majority_level = (high_count >= 2);
    
    // Debug: Son kararı yazdır
    printf("[%s] COGUNLUK KARARI: %d (High count: %d)\n", 
           input->label, majority_level, high_count);
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void TrackInputRequest(InputDebounce_t *input) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // LOW -> HIGH transition (Light ON)
    if (input->confirmed_flag && !input->last_state) {
        uint32_t current_dark_duration = now - input->dark_start_time;
        input->last_dark_duration = current_dark_duration;

        input->light_start_time = now;
        input->light_mode = LIGHT_ON;

        // Start measurement only after proper dark period (2000ms)
        if (input->waiting_for_dark && current_dark_duration >= 2000) {
            input->counting = true;
            input->request_start_time = now;
            input->waiting_for_dark = false;
            input->total_flash_time = 0;
            input->total_steady_time = 0;

            input->measurement_count++;

            if (input->measurement_count >= 2) {
                input->isCountdown_active = true;
                printf("[%s] Countdown ACTIVE (since measurement #%u)\n", input->label, input->measurement_count);
            } else {
                input->isCountdown_active = false;
                printf("[%s] First measurement, countdown NOT active\n", input->label);
            }

            printf("[%s] Measurement STARTED\n", input->label);
        }

        input->last_state = true;
    }

    // HIGH -> LOW transition (Light OFF)
    else if (!input->confirmed_flag && input->last_state) {
        input->last_light_duration = now - input->light_start_time;
        input->last_light_off_time = now;
        input->dark_start_time = now;

        input->light_mode = LIGHT_OFF;

        // Flash detection
        bool is_flash = (input->last_light_duration >= 400 && input->last_light_duration <= 800) &&
                        (input->last_dark_duration >= 400 && input->last_dark_duration <= 800);

        if (is_flash) {
            if (!input->flash_mode) {
                input->flash_mode = true;
                input->light_mode = LIGHT_FLASH;
                printf("[%s] Flash sequence STARTED\n", input->label);
            }

            input->total_flash_time += input->last_light_duration + input->last_dark_duration;
            printf("[%s] Flash added: %"PRIu32"ms (ON) + %"PRIu32"ms (OFF) = %"PRIu32"ms\n",
                   input->label, input->last_light_duration, input->last_dark_duration,
                   input->last_light_duration + input->last_dark_duration);
        } else {
            if (input->flash_mode) {
                printf("[%s] Flash sequence ENDED\n", input->label);
                input->flash_mode = false;
            }

            if (input->counting && input->last_light_duration > 800) {
                input->total_steady_time += input->last_light_duration;
                printf("[%s] Steady light added: %"PRIu32"ms\n",
                       input->label, input->last_light_duration);
            }
        }

        input->last_state = false;
    }

    // Measurement end condition
    if (input->counting && !input->confirmed_flag && (now - input->last_light_off_time > 2000)) {
        uint32_t total_duration = input->total_steady_time + input->total_flash_time;

        printf("[%s] Measurement ENDED | Steady: %"PRIu32"ms | Flash: %"PRIu32"ms | Total: %"PRIu32"ms\n",
               input->label, input->total_steady_time, input->total_flash_time, total_duration);

        input->countdown_start = total_duration / 1000;
        input->countdown_current = input->countdown_start;
        
        input->isCountdown_active = false; // Ölçüm bittiğinde devre dışı bırak
        input->counting = false;
        input->waiting_for_dark = true;
        
	    //s_deviceStatus eşitleme
	    if (strcmp(input->label, "GREEN") == 0) {
	        s_deviceStatus.greenTime = (input->countdown_current);
	    } else if (strcmp(input->label, "RED") == 0) {
	        s_deviceStatus.redTime = (input->countdown_current);
	    }
    
    }
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void InputDebounce_Init(InputDebounce_t *input, const char* label)
{
    // Bütün yapıyı temizle
    memset(input, 0, sizeof(InputDebounce_t));
    // Başlangıç değerlerini ata
    input->label = label;
    input->confirmed_flag = false;
    
    // ⭐ ANAHTAR DEĞİŞİKLİK: Sistemin "karanlıkta bekleme" modunda başlamasını sağlıyoruz.
    input->waiting_for_dark = true; 
    printf("[%s] Init -> dark_start_time = %lu ms\n", input->label, (unsigned long)input->dark_start_time);

    // Diğer değişkenleri de sıfırla
    input->request_start_time = 0;

    input->countdown_current = 0;
}

void ResetAllTrafficVariables(void)
{
	InputDebounce_Init(&green_input, "GREEN");
    InputDebounce_Init(&red_input, "RED");
 }
 
 /*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void DetectGreenFeedback(void) 
{
    // 1. Seviye okuma ve debounce
    int raw_level = gpio_get_level(Green_FB_Input_Pin);
    
    // Örnekleme dizisini güncelle
    green_input.level_flags[0] = green_input.level_flags[1];
    green_input.level_flags[1] = green_input.level_flags[2];
    green_input.level_flags[2] = (raw_level == 1);

    // 2. Çoğunluk kararı
    int high_count = green_input.level_flags[0] + 
                    green_input.level_flags[1] + 
                    green_input.level_flags[2];
    bool new_state = (high_count >= 2);
    
    // 3. Durum değişikliği kontrolü
    if(new_state != green_input.confirmed_flag) {
        green_input.confirmed_flag = new_state;
        printf("[GREEN] Yeni durum: %s\n", new_state ? "HIGH" : "LOW");
    }
}
 /*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/

void DetectRedFeedback(void) 
{
    // 1. Seviye okuma ve debounce
    int raw_level = gpio_get_level(Red_FB_Input_Pin);
    
    // Örnekleme dizisini güncelle
    red_input.level_flags[0] = red_input.level_flags[1];
    red_input.level_flags[1] = red_input.level_flags[2];
    red_input.level_flags[2] = (raw_level == 1);

    // 2. Çoğunluk kararı
    int high_count = red_input.level_flags[0] + 
                    red_input.level_flags[1] + 
                    red_input.level_flags[2];
    bool new_state = (high_count >= 2);
    
    // 3. Durum değişikliği kontrolü
    if(new_state != red_input.confirmed_flag) {
        red_input.confirmed_flag = new_state;
        printf("[RED] Yeni durum: %s\n", new_state ? "HIGH" : "LOW");
    }
}

///*******************************************************************************
//* Function Name  			: None
//* Description    			: None
//* Input         			: None
//* Output        			: None
//* Return        			: None
//*******************************************************************************/
//void DetectRedFeedback(void) 
//{
//    // Ham GPIO değerini oku
//    int raw_level = gpio_get_level(Red_FB_Input_Pin);
//    
//    // Debug: Ham değeri yazdır
//    printf("[RED] RAW GPIO VALUE: %d\n", raw_level);
//    
//    // Örnekleme dizisini kaydır
//    bool prev_samples[3] = {
//        red_input.level_flags[0],
//        red_input.level_flags[1], 
//        red_input.level_flags[2]
//    };
//    
//    red_input.level_flags[0] = red_input.level_flags[1];
//    red_input.level_flags[1] = red_input.level_flags[2];
//    red_input.level_flags[2] = (raw_level == 1);
//
//    // Debug çıktıları
//    printf("[RED] ONCEKI ORNEKLER: %d-%d-%d\n", 
//           prev_samples[0], prev_samples[1], prev_samples[2]);
//    printf("[RED] YENI ORNEKLER: %d-%d-%d\n", 
//           red_input.level_flags[0], red_input.level_flags[1], red_input.level_flags[2]);
//
//    // Çoğunluk seviyesini hesapla
//    int high_count = red_input.level_flags[0] + red_input.level_flags[1] + red_input.level_flags[2];
//    bool majority_level = (high_count >= 2);
//    
//    printf("[RED] COGUNLUK KARARI: %d (High count: %d)\n", majority_level, high_count);
//}

 /*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/ 
void DetectPedestrianDemandFeedback(void) 
{
    // 1. Seviye okuma ve debounce
    int raw_level = gpio_get_level(Demand_Input_Pin);

    // Örnekleme dizisini güncelle
    Demand_input.level_flags[0] = Demand_input.level_flags[1];
    Demand_input.level_flags[1] = Demand_input.level_flags[2];
    Demand_input.level_flags[2] = (raw_level == 1);

    // 2. Çoğunluk kararı
    int high_count = Demand_input.level_flags[0] +
                     Demand_input.level_flags[1] +
                     Demand_input.level_flags[2];
    bool new_state = (high_count >= 2);

    uint64_t now_ms = esp_timer_get_time() / 1000;

    static uint64_t state_start_ms = 0;
    static bool state_initialized = false;
    

    if (!state_initialized) {
        Demand_input.confirmed_flag = new_state;
        state_start_ms = now_ms;
        state_initialized = true;
        printf("[Buton] Baslangic durumu: %s\n", new_state ? "HIGH" : "LOW");
        return;
    }
    
       // Talep yokken (LOW), yeni bir HIGH durumu tespit edildiğinde
    // hasRequestPlayedOnce'ı false yap.
    if (!Demand_input.confirmed_flag && new_state) {
        hasRequestPlayedOnce = false;
    }
    
    if (new_state != Demand_input.confirmed_flag) {
        Demand_input.confirmed_flag = new_state;
        state_start_ms = now_ms;
        printf("[Buton] Yeni durum: %s (timer sifirlandi)\n", new_state ? "HIGH" : "LOW");
    } else {
        uint64_t elapsed_ms = now_ms - state_start_ms;

        uint64_t low_threshold_ms  = (uint64_t)PedestrianFeedback_StuckLowTimeout_min * 60 * 1000;
        uint64_t high_threshold_ms = (uint64_t)PedestrianFeedback_StuckHighTimeout_min * 60 * 1000;

        bool should_log_alarm = false;

        if (!new_state && elapsed_ms >= low_threshold_ms) {
            // LOW durumu çok uzun sürdü
            should_log_alarm = true;
        } else if (new_state && elapsed_ms >= high_threshold_ms) {
            // HIGH durumu çok uzun sürdü
            should_log_alarm = true;
        }

        if (should_log_alarm) {
            Alarm_Log("Pedestrian feedback stuck for too long", &DeviceTime);
            state_start_ms = now_ms;  // tekrar alarm atmasını önle
        }
    }
    
    s_deviceStatus.buttonStatus = Demand_input.confirmed_flag;
}



 /*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
// Dosya isimlerini bir diziye aktaran yardımcı fonksiyon
const char* get_audio_file_path(uint32_t count) {
    static const char* audio_files[31];
    static bool initialized = false;

    // Diziye sadece bir kere kopyala
    if (!initialized) {
        audio_files[30] = s_audioConfig.sound30;
        audio_files[29] = s_audioConfig.sound29;
        audio_files[28] = s_audioConfig.sound28;
        audio_files[27] = s_audioConfig.sound27;
        audio_files[26] = s_audioConfig.sound26;
        audio_files[25] = s_audioConfig.sound25;
        audio_files[24] = s_audioConfig.sound24;
        audio_files[23] = s_audioConfig.sound23;
        audio_files[22] = s_audioConfig.sound22;
        audio_files[21] = s_audioConfig.sound21;
        audio_files[20] = s_audioConfig.sound20;
        audio_files[19] = s_audioConfig.sound19;
        audio_files[18] = s_audioConfig.sound18;
        audio_files[17] = s_audioConfig.sound17;
        audio_files[16] = s_audioConfig.sound16;
        audio_files[15] = s_audioConfig.sound15;
        audio_files[14] = s_audioConfig.sound14;
        audio_files[13] = s_audioConfig.sound13;
        audio_files[12] = s_audioConfig.sound12;
        audio_files[11] = s_audioConfig.sound11;
        audio_files[10] = s_audioConfig.sound10;
        audio_files[9] = s_audioConfig.sound9;
        audio_files[8] = s_audioConfig.sound8;
        audio_files[7] = s_audioConfig.sound7;
        audio_files[6] = s_audioConfig.sound6;
        audio_files[5] = s_audioConfig.sound5;
        audio_files[4] = s_audioConfig.sound4;
        audio_files[3] = s_audioConfig.sound3;
        audio_files[2] = s_audioConfig.sound2;
        audio_files[1] = s_audioConfig.sound1;
        initialized = true;
    }
    
    // Geri sayım aralığı kontrolü
    if (count >= 1 && count <= 30) {
        return audio_files[count];
    }
    return NULL; // Geçersiz sayım değeri için
}

