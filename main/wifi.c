#include "wifi.h"
#include "esp_system.h"
#include "mongoose_glue.h"
static EventGroupHandle_t s_wifi_event_group;

esp_netif_t* esp_netif_ap  = NULL;
extern TaskHandle_t mongoose_task_handle;
bool WifiConnectedFlag = false;
static bool wifi_initialized = false;
char unique_ssid[MAX_SSID_LENGTH] = {0};

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
//static void event_handler(void *arg, esp_event_base_t event_base,
//                          int32_t event_id, void *event_data) {
//  static int retry_count = 0;
//  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//    esp_wifi_connect();
//  } else if (event_base == WIFI_EVENT &&
//             event_id == WIFI_EVENT_STA_DISCONNECTED) {
//    esp_wifi_connect();
//    retry_count++;
//    MG_INFO(("Connecting to the AP fail, attempt #%d", retry_count));
//  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
//    MG_INFO(("Got IP ADDRESS: " IPSTR, IP2STR(&event->ip_info.ip)));
//    retry_count = 0;
//    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//  }
//}

static void mg_wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  static int retry_count = 0;
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
    retry_count++;
    MG_INFO(("Connecting to the AP fail, attempt #%d", retry_count));
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    MG_INFO(("Got IP ADDRESS: " IPSTR, IP2STR(&event->ip_info.ip)));
    retry_count = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}


static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    // 🔹 Common events for both AP and STA modes
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            // ================= STA (Client) Mode Events =================
            case WIFI_EVENT_STA_START:
                MG_INFO(("WiFi STA mode started"));
                esp_wifi_connect(); // Auto-connect to the configured AP
                break;

            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
                MG_INFO(("Connected to AP: %s (BSSID: " MACSTR ")",
                        event->ssid, MAC2STR(event->bssid)));
                WifiConnectedFlag = true;
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
                MG_INFO(("Disconnected from AP (Reason: %d)", event->reason));
                WifiConnectedFlag = false;
                
                // Attempt to reconnect after a delay
                vTaskDelay(pdMS_TO_TICKS(500));
                esp_wifi_connect();
                break;
            }

            // ================= AP Mode Events =================
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                MG_INFO(("Device connected to our AP! MAC: " MACSTR, MAC2STR(event->mac)));
                break;
            }

            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
                MG_INFO(("Device disconnected from our AP! MAC: " MACSTR, MAC2STR(event->mac)));
                break;
            }

            default:
                break;
        }
    }

    // 🔹 IP Assignment Events (common for both modes)
    else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                MG_INFO(("Got IP: " IPSTR, IP2STR(&event->ip_info.ip)));
                break;
            }

            case IP_EVENT_AP_STAIPASSIGNED:
                MG_INFO(("Assigned IP to a station"));
                break;

            default:
                break;
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
void wifi_init(const char *ssid, const char *pass) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  s_wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &mg_wifi_event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &mg_wifi_event_handler, NULL, &instance_got_ip));
  wifi_config_t wc = {};
  strncpy((char *) wc.sta.ssid, ssid, sizeof(wc.sta.ssid));
  strncpy((char *) wc.sta.password, pass, sizeof(wc.sta.password));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
  ESP_ERROR_CHECK(esp_wifi_start());

  MG_INFO(("Trying to connect to SSID:%s pass:%s", ssid, pass));
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    MG_INFO(("connected to ap SSID:%s pass:%s", ssid, pass));
  } else if (bits & WIFI_FAIL_BIT) {
    MG_ERROR(("Failed to connect to SSID:%s, pass:%s", ssid, pass));
  } else {
    MG_ERROR(("UNEXPECTED EVENT"));
  }
}



/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
//// **WiFi Access Point Başlatma**
void wifi_init_ap(void) {
    generate_unique_ssid(unique_ssid, sizeof(unique_ssid));
    
    if (EthConnectedFlag) {
        ESP_LOGI("WiFi", "Ethernet bagli, WiFi baslatilamiyor.");
        return;  // Ethernet varsa WiFi açma
    }

    wifi_deinit_ap();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_ap = esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
      
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = {0},
            // .ssid_len = strlen(unique_ssid),
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = WIFI_CHANNEL
        },
    };
    
    
	strncpy((char *)wifi_config.ap.ssid, unique_ssid, sizeof(wifi_config.ap.ssid));
	wifi_config.ap.ssid_len = strlen(unique_ssid);

    
    // Configure DHCP for the AP
	esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

	esp_netif_dhcps_stop(esp_netif_ap);					///> must call this first

	inet_pton(AF_INET, DEVICE_WIFI_AP_IP, &ap_ip_info.ip);		///> Assign access point's static IP, GW, and netmask
	inet_pton(AF_INET, DEVICE_WIFI_AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, DEVICE_WIFI_AP_NETMASK, &ap_ip_info.netmask);


    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));			///> Statically configure the network interface
	ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));						///> Start the AP DHCP server (for connecting stations e.g. your mobile device)
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WiFi", "WiFi AP baslatildi! SSID: %s, sifre: %s", unique_ssid, WIFI_PASS);
    strncpy(s_wifiSettings.ssid, unique_ssid, sizeof(s_wifiSettings.ssid) - 1);
    s_wifiSettings.ssid[sizeof(s_wifiSettings.ssid) - 1] = '\0';
}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
bool is_network_connected(void) {
    return EthConnectedFlag || WifiConnectedFlag;
}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/

void wifi_deinit_ap(void) {
    if (wifi_initialized) {
        esp_wifi_stop();
        esp_wifi_deinit();
        wifi_initialized = false;
    }
    if (esp_netif_ap) {
        esp_netif_destroy(esp_netif_ap);
        esp_netif_ap = NULL;
    }
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
// MAC adresini kullanarak benzersiz SSID oluştur
void generate_unique_ssid(char *ssid, size_t max_len) {
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
    
    snprintf(ssid, max_len, "%s%02X%02X%02X", 
             WIFI_SSID_PREFIX, mac[3], mac[4], mac[5]);
}
/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
//// **WiFi Access Point Başlatma**
void wifi_init_ap_mg(void) {
    if (EthConnectedFlag) {
        ESP_LOGI("WiFi", "Ethernet bagli, WiFi baslatilamiyor.");
        return;  // Ethernet varsa WiFi açma
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_ap = esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
      
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",  
            .ssid_len = strlen(s_wifiSettings.ssid),
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = WIFI_CHANNEL
        },
    };
	// Then set the SSID separately to avoid pointer conversion issues
	strncpy((char *)wifi_config.ap.ssid, s_wifiSettings.ssid, sizeof(wifi_config.ap.ssid));
	wifi_config.ap.ssid_len = strlen(s_wifiSettings.ssid);

    // Configure DHCP for the AP
	esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

	esp_netif_dhcps_stop(esp_netif_ap);					///> must call this first

	inet_pton(AF_INET, DEVICE_WIFI_AP_IP, &ap_ip_info.ip);		///> Assign access point's static IP, GW, and netmask
	inet_pton(AF_INET, DEVICE_WIFI_AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, DEVICE_WIFI_AP_NETMASK, &ap_ip_info.netmask);


    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));			///> Statically configure the network interface
	ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));						///> Start the AP DHCP server (for connecting stations e.g. your mobile device)
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WiFi", "WiFi AP baslatildi! SSID: %s, sifre: %s", s_wifiSettings.ssid, WIFI_PASS);
  
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void System_Wifi_Init(void)
{
	// WiFi ayarlarını yükle
    struct wifiSettings settings;
}