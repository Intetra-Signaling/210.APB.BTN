/*
 * DetectTraffic.h
 *
 *  Created on: 18 Nis 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_DETECTTRAFFIC_H_
#define MAIN_DETECTTRAFFIC_H_

#include "driver/gpio.h"

#define Red_FB_Input_Pin        32
#define Green_FB_Input_Pin      35
#define Button_Pin              36
#define Demand_Input_Pin        16
#define No_Demand_Input_Pin     17
#define Button_Input_Pin        36

#define FLASH_MIN_DURATION 300 // 400ms
#define FLASH_MAX_DURATION 700 // 600ms


typedef enum {
    LIGHT_OFF,
    LIGHT_ON,
    LIGHT_FLASH
} LightMode;

typedef struct {
    gpio_num_t pin;
    bool level_flags[3];   // 3 ardışık ölçüm için
    bool confirmed_flag;
    int64_t timestamp;
    const char* label;
    
    // Talep takip için temel alanlar:
    bool waiting_for_dark;        // LOW süresi beklenecek mi?
    uint32_t dark_start_time;     // LOW başlangıç zamanı
    uint32_t request_start_time;  // Talep başlangıç zamanı
    
    // Countdown bilgileri
    uint32_t countdown_current;   // Geri sayım süresi (saniye)
    uint32_t countdown_start;     // Başlangıç değeri (saniye)
    bool isCountdown_active;
    uint32_t learned_durations[2];  // Son 2 öğrenilen süre (ms)
    uint8_t learning_phase;         // 0: başlangıç, 1: ilk öğrenme, 2: ikinci öğrenme
    uint8_t countdown_phase;        // 0: bekleme, 1: ilk sayım, 2: ikinci sayım
    uint32_t current_countdown;     // Şu anki geri sayım değeri (ms) 

    // Işık durum takibi
    bool last_state;              // Önceki durum
    uint32_t last_dark_duration;  // Son karanlık süresi
    uint32_t last_light_duration; // Son ışık süresi
    uint32_t light_start_time;    // Işık başlangıç zamanı
    uint32_t last_light_off_time; // Son ışık kapanış zamanı

    // Süre hesaplamaları
    uint32_t total_flash_time;    // Toplam flaş süresi
    uint32_t total_steady_time;   // Toplam sabit ışık süresi

    // Modlar ve durumlar
    bool flash_mode;              // Flaş modu aktif mi?
    bool counting;                // Ölçüm yapılıyor mu?
    LightMode light_mode;         // Mevcut ışık modu
    uint8_t measurement_count; // Ölçüm sayacı
    
    bool isDemandActive;
    
} InputDebounce_t;



// Sadece bildirimi burada yap
extern InputDebounce_t green_input;
extern InputDebounce_t red_input;
extern InputDebounce_t Demand_input;
extern InputDebounce_t No_demand_input;
extern InputDebounce_t User_button_input;

extern uint32_t Counter;

void DetectFeedBack(InputDebounce_t* input);
void GPIO_Init(void);
void TrackInputRequest(InputDebounce_t *input);
void InputDebounce_Init(InputDebounce_t *input, const char* label);
void ResetAllTrafficVariables(void);
void DetectGreenFeedback(void);
void DetectButtonFeedback(void); 
void DetectRedFeedback(void);
void DetectPedestrianDemandFeedback(void);
const char* get_audio_file_path(uint32_t count);
#endif /* MAIN_DETECTTRAFFIC_H_ */
