/*
 * main.h
 *
 *  Created on: 20 Mar 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_


#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "mongoose_glue.h"
#include "mongoose.h"
#include "ethernet_init.h"
#include "SD_SPI.h"
//#include "wifi.h"
#include "driver/adc_types_legacy.h"
#include "esp_adc/adc_continuous.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
/////////////////////////////////////////////////////////////////////////////////////////
/***************************************THREADS******************************************/

#define APPLICATON_VERSION "v.0.0.0.6"

//void read_file(const char *filename);

#endif /* MAIN_MAIN_H_ */
