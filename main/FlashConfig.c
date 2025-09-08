/*
 * FlashConfig.c
 *
 *  Created on: 28 Nis 2025
 *      Author: metesepetcioglu
 */

#include "FlashConfig.h"
#include "Plan.h"
#include "mongoose_glue.h"
#include "wifi.h"

extern struct defaultConfiguration s_defaultConfiguration;
extern struct alt1Configuration s_alt1Configuration; 
extern struct alt2Configuration s_alt2Configuration; 
extern struct alt3Configuration s_alt3Configuration; 

/*******************************************************************************
* Function Name              : writeFlash
* Description                : Writes data to flash memory
* Input                     : const char *key, const void *data, size_t length
* Output                    : None
* Return                    : esp_err_t (ESP_OK on success, error code otherwise)
*******************************************************************************/
esp_err_t writeFlash(const char *key, const void *data, size_t length)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS storage with write access
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    // Write data to NVS
    err = nvs_set_blob(nvs_handle, key, data, length);
    if (err == ESP_OK) {
        // Commit changes
        err = nvs_commit(nvs_handle);
    }

    // Close NVS handle
    nvs_close(nvs_handle);
    return err;
}
/*******************************************************************************   
* Description                : Reads data from flash memory
* Input                     : const char *key, void *data, size_t *length
* Output                    : None
* Return                    : esp_err_t (ESP_OK on success, error code otherwise)
*******************************************************************************/
esp_err_t readFlash(const char *key, void *data, size_t *length)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS storage with read access
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    // Read data from NVS
    err = nvs_get_blob(nvs_handle, key, data, length);

    // Close NVS handle
    nvs_close(nvs_handle);
    return err;
}

/*******************************************************************************   
* Description               : FlashInit
* Input                     : None
* Output                    : None
* Return                    : None
*******************************************************************************/
esp_err_t FlashInit(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        printf("NVS init failed: %s\n", esp_err_to_name(err));
        return err;
        //ALARMLARI TAMAMLA
    }

    return ESP_OK;
}
/*******************************************************************************   
* Description               : None
* Input                     : None
* Output                    : None
* Return                    : None
*******************************************************************************/

// Flash’a yaz
void saveDefaultToFlash(void) {
 

}


/*******************************************************************************   
* Description               : None
* Input                     : None
* Output                    : None
* Return                    : None
*******************************************************************************/
void CheckInternalFlash(void) 
{

}


/*******************************************************************************
* Function Name              : loadConfigurationsFromFlash
* Description                : Loads all configurations from flash memory
* Input                      : None
* Output                     : None
* Return                     : esp_err_t (ESP_OK if all loaded successfully)
*******************************************************************************/
esp_err_t loadConfigurationsFromFlash(void)
{
    esp_err_t ret = ESP_OK;
    size_t required_size;

    // 1. Load default configuration
    required_size = sizeof(struct defaultConfiguration);
    if(readFlash("defconf", &s_defaultConfiguration, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for default_config\n");
        ret = ESP_FAIL;
    }

    // 2. Load alt1 configuration
    required_size = sizeof(struct alt1Configuration);
    if(readFlash("alt1conf", &s_alt1Configuration, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for alt1_config\n");
        ret = ESP_FAIL;
    }

    // 3. Load alt2 configuration
    required_size = sizeof(struct alt1Configuration);
    if(readFlash("alt2conf", &s_alt2Configuration, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for alt2_config\n");
        ret = ESP_FAIL;
    }

    // 4. Load alt3 configuration
    required_size = sizeof(struct alt3Configuration);
    if(readFlash("alt3conf", &s_alt3Configuration, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for alt3_config\n");
        ret = ESP_FAIL;
    }
    // 5. Load system info (device name and comment)
    required_size = sizeof(s_systemInfo.deviceName);
    if(readFlash("device_name", s_systemInfo.deviceName, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for device_name\n");
        ret = ESP_FAIL;
    }
      required_size = sizeof(s_systemInfo.deviceComment);
    if(readFlash("device_comment", s_systemInfo.deviceComment, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for device_comment\n");
        ret = ESP_FAIL;
    }
//    // 6. Load WiFi settings
//    required_size = sizeof(s_wifiSettings.ssid);
//    if(readFlash(WIFI_SSID_KEY, s_wifiSettings.ssid, &required_size) != ESP_OK)
//    {
//        printf("Using DEFAULT values for WiFi SSID\n");
//        ret = ESP_FAIL;
//    }
    // 7. Load audio configuration
    required_size = sizeof(struct audioConfig);
    if(readFlash("audio_configs", &s_audioConfig, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for audio_configs\n");
        ret = ESP_FAIL;
    }
    // 8. Load calendar configurations
    required_size = sizeof(struct sunday);
    if(readFlash("sunday", &s_sunday, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for sunday\n");
        ret = ESP_FAIL;
    }

    required_size = sizeof(struct monday);
    if(readFlash("monday", &s_monday, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for monday\n");
        ret = ESP_FAIL;
    }

    required_size = sizeof(struct tuesday);
    if(readFlash("tuesday", &s_tuesday, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for tuesday\n");
        ret = ESP_FAIL;
    }

    required_size = sizeof(struct wednesday);
    if(readFlash("wednesday", &s_wednesday, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for wednesday\n");
        ret = ESP_FAIL;
    }

    required_size = sizeof(struct thursday);
    if(readFlash("thursday", &s_thursday, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for thursday\n");
        ret = ESP_FAIL;
    }

    required_size = sizeof(struct friday);
    if(readFlash("friday", &s_friday, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for friday\n");
        ret = ESP_FAIL;
    }

    required_size = sizeof(struct saturday);
    if(readFlash("saturday", &s_saturday, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for saturday\n");
        ret = ESP_FAIL;
    }

    required_size = sizeof(struct holidays);
    if(readFlash("holidays", &s_holidays, &required_size) != ESP_OK)
    {
        printf("Using DEFAULT values for holidays\n");
        ret = ESP_FAIL;
    }

    return ret;
    
}

/*******************************************************************************
* Function Name              : loadConfigurationsFromFlash
* Description                : Loads all configurations from flash memory
* Input                      : None
* Output                     : None
* Return                     : esp_err_t (ESP_OK if all loaded successfully)
*******************************************************************************/
void loadWifiSettings(void) {

    size_t ssid_len = sizeof(s_wifiSettings.ssid);
    esp_err_t ret;
    
    // Flash'tan SSID'yi oku
    ret = readFlash(WIFI_SSID_KEY, s_wifiSettings.ssid, &ssid_len);
    
    // Okuma başarısız olduysa veya SSID boşsa
    if (ret != ESP_OK || ssid_len == 0 || s_wifiSettings.ssid[0] == '\0') {
		ESP_LOGI("WiFi", "Varsayilan WiFi baslatildi (Flash'ta SSID yok)");
        wifi_init_ap(); // Varsayılan WiFi ayarları
        
    } else {
		ESP_LOGI("WiFi", "Ozel WiFi baslatildi: %s", s_wifiSettings.ssid);
        wifi_init_ap_mg(); // Flash'tan okunan SSID ile başlat
        
    }
}

/*******************************************************************************
* Function Name              : None
* Description                : None
* Input                      : None
* Output                     : None
* Return                     : None
*******************************************************************************/
void convert_config_to_alt(struct configuration *src, struct alt1Configuration *dest) {
    dest->isIdleActive = src->isIdleActive;
    strcpy(dest->idleSound, src->idleSound);
    dest->idleMinVolume = src->idleMinVolume;
    dest->idleMaxVolume = src->idleMaxVolume;
    dest->idleContAfterReq = src->idleContAfterReq;
    dest->isReqActive = src->isReqActive;
    strcpy(dest->reqSound1, src->reqSound1);
    strcpy(dest->reqSound2, src->reqSound2);
    dest->reqPlayPeriod = src->reqPlayPeriod;
    dest->reqMinVolume = src->reqMinVolume;
    dest->reqMaxVolume = src->reqMaxVolume;
    dest->isGreenActive = src->isGreenActive;
    strcpy(dest->greenSound, src->greenSound);
    dest->greenMinVolume = src->greenMinVolume;
    dest->greenMaxVolume = src->greenMaxVolume;
    dest->greenCountFrom = src->greenCountFrom;
    dest->greenCountTo = src->greenCountTo;
    strcpy(dest->greenAction, src->greenAction);
}
void convert_config_to_alt3(struct configuration *src, struct alt3Configuration *dest) {
  dest->isIdleActive = src->isIdleActive;
  strcpy(dest->idleSound, src->idleSound);
  dest->idleMinVolume = src->idleMinVolume;
  dest->idleMaxVolume = src->idleMaxVolume;
  dest->idleContAfterReq = src->idleContAfterReq;
  dest->isReqActive = src->isReqActive;
  strcpy(dest->reqSound1, src->reqSound1);
  strcpy(dest->reqSound2, src->reqSound2);
  dest->reqPlayPeriod = src->reqPlayPeriod;
  dest->reqMinVolume = src->reqMinVolume;
  dest->reqMaxVolume = src->reqMaxVolume;
  dest->isGreenActive = src->isGreenActive ? 1 : 0;  // bool to int
  strcpy(dest->greenSound, src->greenSound);
  dest->greenMinVolume = src->greenMinVolume;
  dest->greenMaxVolume = src->greenMaxVolume;
  dest->greenCountFrom = src->greenCountFrom;
  dest->greenCountTo = src->greenCountTo;
  strcpy(dest->greenAction, src->greenAction);
}



