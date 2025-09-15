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


/* WiFi application settings */
#define WIFI_AP_BEACON_INTERVAL		100										// Access Point beacon: 100 milliseconds recommended
#define DEVICE_WIFI_AP_IP					"192.168.1.2"							// Access Point default IP
#define DEVICE_WIFI_AP_GATEWAY				"192.168.1.1"							// Access Point default Gateway (should be the same as the IP)
#define DEVICE_WIFI_AP_NETMASK				"255.255.255.0"							// Access Point netmask
#define WIFI_AP_BANDWIDTH			WIFI_BW_HT20							// Access Point bandwidth 20 MHz (40 MHz is the other option)
#define WIFI_STA_POWER_SAVE			WIFI_PS_NONE							// Power save not used
#define MAX_SSID_LENGTH				32										// IEEE standard maximum
#define MAX_PASSWORD_LENGTH			64										// IEEE standard maximum
#define MAX_CONNECTION_RETRIES		1000u									// Retry number on disconnect
#define WIFI_CHANNEL 1
#define MAX_STA_CONN 4
#define WIFI_SSID_PREFIX "ESP32_AP_"
#define WIFI_PASS "12345678"
#define MAX_SSID_LENGTH 32
#define WIFI_STARTED_BIT BIT0
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1


void wifi_deinit_ap(void);
void wifi_init_ap(void);
void wifi_init(const char *ssid, const char *pass);
extern bool WifiConnectedFlag;
void generate_unique_ssid(char *ssid, size_t max_len);
void wifi_init_ap_mg(void);
#endif /* MAIN_WIFI_H_ */
