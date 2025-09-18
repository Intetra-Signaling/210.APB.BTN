/*
 * ADCConfig.c
 *
 *  Created on: 16 Eyl 2025
 *
 * @file
 * @brief Provides API functions to initialize, read, and deinitialize the ADC (Analog-to-Digital Converter) for ESP32.
 *
 * This module manages ADC operations including continuous and single conversion modes, configuration, sample reading,
 * averaging for noise reduction, and safe resource management. All key routines for interacting with ADC hardware are implemented here.
 *
 * @company    INTETRA
 * @version    v.0.0.0.1
 * @creator    Mete SEPETCIOGLU
 * @update     Mete SEPETCIOGLU
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

#define ADC_CHANNEL     ADC1_CHANNEL_6   // GPIO34
#define ADC_WIDTH_CFG   ADC_WIDTH_BIT_12
#define ADC_ATTEN_CFG   ADC_ATTEN_DB_12
static const char *TAG_ADC = "ADC_DRIVER";

 
 /**
 * @brief ADC continuous conversion done callback.
 *
 * This callback function is called from ISR context when the ADC continuous driver
 * completes the required number of conversions.
 *
 * It notifies the associated FreeRTOS task using vTaskNotifyGiveFromISR.
 *
 * @param[in]  handle    ADC continuous driver handle.
 * @param[in]  edata     Pointer to event data (conversion results, unused here).
 * @param[in]  user_data User-provided data (unused here).
 *
 * @return true if a higher priority task was woken and a context switch should be requested, false otherwise.
 */
bool s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);
    return (mustYield == pdTRUE);
}
 
 
 /**
 * @brief Initializes the ADC in continuous conversion mode.
 *
 * This function sets up the ADC driver for continuous sampling on the specified channels,
 * configures the buffer and frame sizes, and applies sampling parameters such as frequency,
 * conversion mode, output format, attenuation, bit width, and unit for each channel.
 *
 * @param[in]  channel      Pointer to array of ADC channel identifiers.
 * @param[in]  channel_num  Number of channels to configure.
 * @param[out] out_handle   Pointer to ADC continuous driver handle (will be set by this function).
 *
 * @note Uses ESP_ERROR_CHECK for error handling; will abort on failure.
 */
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
 
 
 /**
 * @brief Initializes the ADC in continuous conversion mode.
 *
 * This function sets up the ADC hardware and driver for continuous sampling.
 * If the ADC has already been initialized, the function will exit immediately.
 * It configures the conversion channels, registers event callbacks (here, not used),
 * and starts continuous ADC sampling using the ESP-IDF ADC driver.
 *
 * @note Uses ESP_ERROR_CHECK for error handling; will abort on failure.
 */
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
 
 
 /**
 * @brief Deinitializes the ADC continuous conversion driver and releases resources.
 *
 * Stops the ADC continuous conversion if it is running, deinitializes the ADC driver,
 * and releases related resources. If the ADC is already stopped, the function returns immediately.
 * After cleanup, the global ADC handle is set to NULL.
 *
 * @note Logs warnings if stopping or deinitializing fails. 
 */
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


/**
 * @brief Reads a single sample from the ADC in continuous mode.
 *
 * Attempts to read one conversion frame from the ADC driver. Checks for read errors and ensures
 * the returned data is valid. If successful, extracts and returns the ADC data using the
 * EXAMPLE_ADC_GET_DATA macro. Returns 0 in case of error or invalid data.
 *
 * @return The ADC sample value read from the hardware, or 0 on failure.
 */
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


/**
 * @brief Initializes the ADC in single conversion mode.
 *
 * Configures the ADC width and channel attenuation for single sample readings.
 * If configuration fails, logs an error and triggers an alarm event.
 *
 * @note Uses global configuration macros for width and attenuation.
 *
 * @return None.
 */
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


/**
 * @brief Reads a single raw sample from the ADC channel.
 *
 * Retrieves one raw conversion value from the configured ADC channel using single mode.
 *
 * @return The raw ADC sample value.
 */
uint32_t ADC_Read_Sample(void)
{
    return adc1_get_raw(ADC_CHANNEL);
}



/**
 * @brief Reads and returns the average value of multiple ADC samples.
 *
 * Obtains a specified number of raw ADC samples from the configured channel and computes their average.
 * This method helps reduce noise (e.g., microphone input) by averaging.
 *
 * @param[in] samples Number of samples to read and average.
 * @return Averaged ADC value, or 0 if samples is zero.
 */uint32_t ADC_Read_Average(uint16_t samples)
{
    uint64_t sum = 0;
    for (uint16_t i = 0; i < samples; i++) {
        sum += adc1_get_raw(ADC_CHANNEL);
    }
    return (uint32_t)(sum / samples);
}


