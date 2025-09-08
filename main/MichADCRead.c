/*
 * MichADCRead.c
 *
 *  Created on: 15 Nis 2025
 *      Author: metesepetcioglu
 */

#include "MichADCRead.h"

volatile uint32_t g_movingRMS;
const char *TAGADC = "ADCEXAMPLE";
TaskHandle_t s_task_handle;
adc_channel_t channel[1] = {ADC1_CHANNEL_6}; //IO 34

adc_continuous_handle_t adc_handle = NULL;
uint32_t adc_samples[ADC_SAMPLE_COUNT] = {0};
uint8_t adc_sample_index = 0;
uint32_t g_adcAverage = 0;

/*******************************************************************************
* Function Name  			: ADC thread
* Description    			: ADC thread
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void ADC_TASK(void *pvParameters)
{
//    esp_err_t ret;
//    uint32_t ret_num = 0;
//    uint8_t result[EXAMPLE_READ_LEN] = {0};
//    memset(result, 0xcc, EXAMPLE_READ_LEN);
//
//    s_task_handle = xTaskGetCurrentTaskHandle();
//    adc_continuous_handle_t handle = NULL;
//    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);
//
//    adc_continuous_evt_cbs_t cbs = {
//        .on_conv_done = s_conv_done_cb,
//    };
//    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
//    ESP_ERROR_CHECK(adc_continuous_start(handle));
//
//    // Moving average hesaplama için buffer, sayaç ve toplam değişkenleri
//    uint32_t rms_buffer[MOVING_SIZE] = {0};
//    int currentIndex = 0;        // Döngüsel tampon indeksi
//    int totalSamples = 0;        // Toplam toplanan örnek sayısı
//    uint64_t rms_sum = 0;        // Son 10 değerin toplamı
//
//    while (1) {
//        // Yeni veri geldiğinde bildirim bekle
//        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//
//        while (1) {
//            ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
//            if (ret == ESP_OK) {
//                int sample_count = ret_num / sizeof(adc_digi_output_data_t);
//                uint64_t sum_squares = 0;
//
//                for (int i = 0; i < sample_count; i++) {
//                    adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i * sizeof(adc_digi_output_data_t)];
//                    uint32_t data = EXAMPLE_ADC_GET_DATA(p);
//                    
//                    // DC offset: 2048 (12-bit ADC için, eğer farklıysa ayarla)
//                    int32_t value = (int32_t)data - 2048;
//                    sum_squares += (int64_t)value * value;
//                }
//
//                // RMS hesaplama: √(ortalama kare)
//                uint32_t currentRMS = sqrt(sum_squares / sample_count);
//
//                // Moving average hesaplama (circular buffer)
//                if(totalSamples < MOVING_SIZE) {
//                    // Henüz buffer tam dolmadıysa, direkt ekle
//                    rms_buffer[currentIndex] = currentRMS;
//                    rms_sum += currentRMS;
//                    totalSamples++;
//                } else {
//                    // Buffer tam dolu: eski değeri çıkar, yeni değeri ekle
//                    rms_sum -= rms_buffer[currentIndex];
//                    rms_buffer[currentIndex] = currentRMS;
//                    rms_sum += currentRMS;
//                }
//                
//                // Döngüsel indeks güncellemesi
//                currentIndex = (currentIndex + 1) % MOVING_SIZE;
//                
//                // Ortalama RMS hesabı (buffer dolduysa MOVING_SIZE'a böl; dolu değilse mevcut örnek sayısına böl)
//                uint32_t movingRMS = (totalSamples < MOVING_SIZE) ? (rms_sum / totalSamples) : (rms_sum / MOVING_SIZE);
//                g_movingRMS = movingRMS;
//                // Terminale logla
//                adc_digi_output_data_t *p0 = (adc_digi_output_data_t*)&result[0];
//                uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p0);
//                ESP_LOGI(TAGADC, "ADC CH:%"PRIu32" - Moving RMS (son 100): %"PRIu32, chan_num, movingRMS);
//            } else if (ret == ESP_ERR_TIMEOUT) {
//                break;
//            }
//            // Diğer görevler için CPU'yu serbest bırak
//            vTaskDelay(pdMS_TO_TICKS(10));
//        }
//    }
//
//    // Bu kısımlar sonsuz döngüye girdiği için çalışmayacak.
//    ESP_ERROR_CHECK(adc_continuous_stop(handle));
//    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
//    vTaskDelete(NULL);
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
bool s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);
    return (mustYield == pdTRUE);
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = EXAMPLE_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20000,  // 20 kHz örnekleme
        .conv_mode = EXAMPLE_ADC_CONV_MODE,
        .format = EXAMPLE_ADC_OUTPUT_TYPE,
    };
    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void InitADC(void) 
{
    if (adc_handle != NULL) return;  // Zaten başlatıldıysa tekrar başlatma

    //continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &adc_handle);
    continuous_adc_init(channel, 1, &adc_handle);
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = NULL, // callback kullanılmayacak
    };

    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
}
/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void DeInitADC(void)
{
    if (adc_handle == NULL) return;  // Zaten durdurulmuşsa işlem yapma

    // ADC sürekli okumayı durdur
    esp_err_t err = adc_continuous_stop(adc_handle);
    if (err != ESP_OK) {
        ESP_LOGW("ADC", "adc_continuous_stop failed: %s", esp_err_to_name(err));
    }

    // ADC kaynaklarını serbest bırak
    err = adc_continuous_deinit(adc_handle);
    if (err != ESP_OK) {
        ESP_LOGW("ADC", "adc_continuous_deinit failed: %s", esp_err_to_name(err));
    }

    // Handle'ı temizle
    adc_handle = NULL;

    ESP_LOGI("ADC", "ADC deinit tamamlandi");
}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
uint32_t ReadADC_Sample(void) {
    uint8_t result[EXAMPLE_READ_LEN] = {0};
    uint32_t ret_num = 0;

    esp_err_t ret = adc_continuous_read(adc_handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
    if (ret != ESP_OK || ret_num < sizeof(adc_digi_output_data_t)) {
        return 0;
    }
    if (ret == ESP_OK && ret_num >= sizeof(adc_digi_output_data_t)) {
        adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[0];
        uint32_t data = EXAMPLE_ADC_GET_DATA(p);
        return data;
        //int32_t value = (int32_t)data - 2048;
        //return (uint32_t)sqrt((float)(data * data));
         // Basit RMS hesap
    }

    return 0; // Hata durumu
}



/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/

#define ADC_CHANNEL     ADC1_CHANNEL_6   // GPIO34
#define ADC_WIDTH_CFG   ADC_WIDTH_BIT_12
#define ADC_ATTEN_CFG   ADC_ATTEN_DB_12
static const char *TAG_ADC = "ADC_DRIVER";


// ADC başlatma
void ADC_Read_Init(void)
{
    esp_err_t ret;

    // ADC çözünürlüğü
    ret = adc1_config_width(ADC_WIDTH_CFG);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_ADC, "adc1_config_width failed");
        Alarm_Log("ADC width configuration failed", &DeviceTime);
        return;
    }

    // ADC giriş kanalını ayarla
    ret = adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_CFG);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_ADC, "adc1_config_channel_atten failed");
        Alarm_Log("ADC channel attenuation configuration failed", &DeviceTime);
        return;
    }

    ESP_LOGI(TAG_ADC, "ADC Initialized: channel=%d, width=%d, atten=%d", 
             ADC_CHANNEL, ADC_WIDTH_CFG, ADC_ATTEN_CFG);

}

// Tek örnek okuma
uint32_t ADC_Read_Sample(void)
{
    return adc1_get_raw(ADC_CHANNEL);
}

// Ortalama değer okuma (mikrofon gürültüsünü azaltmak için)
uint32_t ADC_Read_Average(uint16_t samples)
{
    uint64_t sum = 0;
    for (uint16_t i = 0; i < samples; i++) {
        sum += adc1_get_raw(ADC_CHANNEL);
    }
    return (uint32_t)(sum / samples);
}


