/*
 * MichADCRead.h
 *
 *  Created on: 15 Nis 2025
 *      Author: metesepetcioglu
 */
 
#include "main.h"
#include "driver/adc_types_legacy.h"
#include "esp_adc/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "driver/adc.h"
#include "Alarms.h"

#ifndef MAIN_MICHADCREAD_H_
#define MAIN_MICHADCREAD_H_


#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_12
#define EXAMPLE_ADC_BIT_WIDTH               ADC_BITWIDTH_12
#define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define EXAMPLE_READ_LEN                    64   
#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type1.data)
#define MOVING_SIZE 5
#define ADC_SAMPLE_COUNT 10
#define ADC_AVERAGE_COUNT 10 

extern const char *TAGADC;
extern TaskHandle_t s_task_handle;
extern adc_channel_t channel[1];
extern uint8_t adc_sample_index;
extern uint32_t adc_samples[ADC_SAMPLE_COUNT];
extern uint32_t g_adcAverage;
 
bool s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data);
void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle);
void ADC_TASK(void *pvParameters);
void InitADC(void);
uint32_t ReadADC_Sample(void);
extern adc_continuous_handle_t adc_handle;
void DeInitADC(void);
void ADC_Read_Init(void);
uint32_t ADC_Read_Average(uint16_t samples);
#endif /* MAIN_MICHADCREAD_H_ */
