/*
 * wifi.h
 *
 *  Created on: 22 Nis 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

#include "esp_wifi.h"
#include "mongoose.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "ethernet_init.h"

void wifi_deinit_ap(void);
void wifi_init_ap(void);
void wifi_init(const char *ssid, const char *pass);
extern bool WifiConnectedFlag;
void generate_unique_ssid(char *ssid, size_t max_len);
void wifi_init_ap_mg(void);
#endif /* MAIN_WIFI_H_ */
