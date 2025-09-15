/*
 * FlashConfig.h
 *
 *  Created on: 28 Nis 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_FLASHCONFIG_H_
#define MAIN_FLASHCONFIG_H_


#include "nvs_flash.h"
#include "nvs.h"

#define FLASH_DATA_LENGTH 100  // 100 byte sabiti
#define WIFI_SSID_KEY "ssid"

esp_err_t writeFlash(const char *key, const void *data, size_t length);
esp_err_t readFlash(const char *key, void *data, size_t *length);
esp_err_t FlashInit(void);
esp_err_t saveToFlash100Bytes(const char *key, const uint8_t *buffer);
esp_err_t loadFromFlash100Bytes(const char *key, uint8_t *buffer);
void saveDefaultToFlash(void);
void CheckInternalFlash(void);
esp_err_t loadConfigurationsFromFlash(void);
void loadWifiSettings(void);
void glue_load_combinedConfiguration();
bool ClearConfigsFromFlash(void);

#endif /* MAIN_FLASHCONFIG_H_ */
