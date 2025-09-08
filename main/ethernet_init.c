/*
 * ethernet_init.c
 *
 *  Created on: 7 Mar 2025
 *      Author: metesepetcioglu
 */

#include "ethernet_init.h"
#include "wifi.h"
#include "Alarms.h"
#include "SystemTime.h"

// Başka dosyalardan erişmek için extern tanımları
extern esp_eth_handle_t *eth_handles;
extern uint8_t eth_port_cnt;
esp_netif_t *eth_netif;
bool EthConnectedFlag = false;
bool Ethernet_Hardware_Fault = false;
bool g_ethernet_available = true;
const char *TAG_ETH = "example_eth_init";
const char *TAGETH = "eth_server";
const char *TAGSDinETH = "example";
extern TaskHandle_t mongoose_task_handle;
//extern void Wifi_Exit_Soft_Ap_Config(void);

#if CONFIG_EXAMPLE_USE_SPI_ETHERNET
#include "driver/spi_master.h"
#endif // CONFIG_EXAMPLE_USE_SPI_ETHERNET

#if CONFIG_EXAMPLE_SPI_ETHERNETS_NUM
#define SPI_ETHERNETS_NUM           CONFIG_EXAMPLE_SPI_ETHERNETS_NUM
#else
#define SPI_ETHERNETS_NUM           0
#endif

#if CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET
#define INTERNAL_ETHERNETS_NUM      1
#else
#define INTERNAL_ETHERNETS_NUM      0
#endif

#define INIT_SPI_ETH_MODULE_CONFIG(eth_module_config, num)                                      \
    do {                                                                                        \
        eth_module_config[num].spi_cs_gpio = CONFIG_EXAMPLE_ETH_SPI_CS ##num## _GPIO;           \
        eth_module_config[num].int_gpio = CONFIG_EXAMPLE_ETH_SPI_INT ##num## _GPIO;             \
        eth_module_config[num].polling_ms = CONFIG_EXAMPLE_ETH_SPI_POLLING ##num## _MS;         \
        eth_module_config[num].phy_reset_gpio = CONFIG_EXAMPLE_ETH_SPI_PHY_RST ##num## _GPIO;   \
        eth_module_config[num].phy_addr = CONFIG_EXAMPLE_ETH_SPI_PHY_ADDR ##num;                \
    } while(0)

typedef struct {
    uint8_t spi_cs_gpio;
    int8_t int_gpio;
    uint32_t polling_ms;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
    uint8_t *mac_addr;
}spi_eth_module_config_t;


#if CONFIG_EXAMPLE_USE_SPI_ETHERNET
static bool gpio_isr_svc_init_by_eth = false; // indicates that we initialized the GPIO ISR service
#endif // CONFIG_EXAMPLE_USE_SPI_ETHERNET


#if CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET
/**
 * @brief Internal ESP32 Ethernet initialization
 *
 * @param[out] mac_out optionally returns Ethernet MAC object
 * @param[out] phy_out optionally returns Ethernet PHY object
 * @return
 *          - esp_eth_handle_t if init succeeded
 *          - NULL if init failed
 */
static esp_eth_handle_t eth_init_internal(esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out)
{
    esp_eth_handle_t ret = NULL;

    // Init common MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    // Update PHY config based on board specific configuration
    phy_config.phy_addr = CONFIG_EXAMPLE_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;
    // Init vendor specific MAC config to default
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    // Update vendor specific MAC config based on board configuration
    esp32_emac_config.smi_gpio.mdc_num = CONFIG_EXAMPLE_ETH_MDC_GPIO;
    esp32_emac_config.smi_gpio.mdio_num = CONFIG_EXAMPLE_ETH_MDIO_GPIO;
#if CONFIG_EXAMPLE_USE_SPI_ETHERNET
    // The DMA is shared resource between EMAC and the SPI. Therefore, adjust
    // EMAC DMA burst length when SPI Ethernet is used along with EMAC.
    esp32_emac_config.dma_burst_len = ETH_DMA_BURST_LEN_4;
#endif // CONFIG_EXAMPLE_USE_SPI_ETHERNET
    // Create new ESP32 Ethernet MAC instance
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    // Create new PHY instance based on board configuration
#if CONFIG_EXAMPLE_ETH_PHY_IP101
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_RTL8201
    esp_eth_phy_t *phy = esp_eth_phy_new_rtl8201(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_LAN87XX
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_DP83848
    esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_KSZ80XX
    esp_eth_phy_t *phy = esp_eth_phy_new_ksz80xx(&phy_config);
#endif
    // Init Ethernet driver to default and install it
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_GOTO_ON_FALSE(esp_eth_driver_install(&config, &eth_handle) == ESP_OK, NULL,
                        err, TAG, "Ethernet driver install failed");

    if (mac_out != NULL) {
        *mac_out = mac;
    }
    if (phy_out != NULL) {
        *phy_out = phy;
    }
    return eth_handle;
err:
    if (eth_handle != NULL) {
        esp_eth_driver_uninstall(eth_handle);
    }
    if (mac != NULL) {
        mac->del(mac);
    }
    if (phy != NULL) {
        phy->del(phy);
    }
    return ret;
}
#endif // CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET

#if CONFIG_EXAMPLE_USE_SPI_ETHERNET
/**
 * @brief SPI bus initialization (to be used by Ethernet SPI modules)
 *
 * @return
 *          - ESP_OK on success
 */
 /*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
static esp_err_t spi_bus_init(void)
{
    esp_err_t ret = ESP_OK;

#if (CONFIG_EXAMPLE_ETH_SPI_INT0_GPIO >= 0) || (CONFIG_EXAMPLE_ETH_SPI_INT1_GPIO > 0)
    // Install GPIO ISR handler to be able to service SPI Eth modules interrupts
    ret = gpio_install_isr_service(0);
    if (ret == ESP_OK) {
        gpio_isr_svc_init_by_eth = true;
    } else if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG_ETH, "GPIO ISR handler has already been installed");
        ret = ESP_OK;  // Already installed, safe to continue
    } else {
        ESP_LOGE(TAG_ETH, "Failed to install GPIO ISR handler: %s", esp_err_to_name(ret));
        return ret; // Continue with caution or handle upper layer
    }
#endif

    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_EXAMPLE_ETH_SPI_MISO_GPIO,
        .mosi_io_num = CONFIG_EXAMPLE_ETH_SPI_MOSI_GPIO,
        .sclk_io_num = CONFIG_EXAMPLE_ETH_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_ETH, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret; // SPI bus kurulamadı ama sistem çökmeden devam edebilir
    }
    return ESP_OK;
}


/**
 * @brief Ethernet SPI modules initialization
 *
 * @param[in] spi_eth_module_config specific SPI Ethernet module configuration
 * @param[out] mac_out optionally returns Ethernet MAC object
 * @param[out] phy_out optionally returns Ethernet PHY object
 * @return
 *          - esp_eth_handle_t if init succeeded
 *          - NULL if init failed
 */
 
 /*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/ 
static esp_eth_handle_t eth_init_spi(spi_eth_module_config_t *spi_eth_module_config, 
                                   esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out) 
{
    esp_eth_handle_t eth_handle = NULL;
    esp_err_t res;

    // Try to initialize but don't fail if hardware isn't present
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = spi_eth_module_config->phy_addr;
    phy_config.reset_gpio_num = spi_eth_module_config->phy_reset_gpio;

    spi_device_interface_config_t spi_devcfg = {
        .mode = 0,
        .clock_speed_hz = CONFIG_EXAMPLE_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .queue_size = 20,
        .spics_io_num = spi_eth_module_config->spi_cs_gpio
    };

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(CONFIG_EXAMPLE_ETH_SPI_HOST, &spi_devcfg);
    w5500_config.int_gpio_num = spi_eth_module_config->int_gpio;
    w5500_config.poll_period_ms = spi_eth_module_config->polling_ms;

    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    if (!mac) {
        ESP_LOGW("ETH", "Failed to create W5500 MAC instance");
        return NULL;
    }

    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
    if (!phy) {
        ESP_LOGW("ETH", "Failed to create W5500 PHY instance");
        mac->del(mac);
        return NULL;
    }

    esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac, phy);
    res = esp_eth_driver_install(&eth_config_spi, &eth_handle);
    if (res != ESP_OK) {
        ESP_LOGW("ETH", "SPI Ethernet driver install failed (hardware may not be present): %s", 
                esp_err_to_name(res));
        mac->del(mac);
        phy->del(phy);
        return NULL;
    }

    if (spi_eth_module_config->mac_addr != NULL) {
        esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, spi_eth_module_config->mac_addr);
    }

    if (mac_out != NULL) *mac_out = mac;
    if (phy_out != NULL) *phy_out = phy;
    
    return eth_handle;
}
#endif // CONFIG_EXAMPLE_USE_SPI_ETHERNET

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
esp_err_t example_eth_init(esp_eth_handle_t *eth_handles_out[], uint8_t *eth_cnt_out) {
    esp_err_t ret = ESP_OK;
    esp_eth_handle_t *eth_handles = NULL;
    uint8_t eth_cnt = 0;

    // Initialize with no Ethernet interfaces by default
    *eth_handles_out = NULL;
    *eth_cnt_out = 0;

#if CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET || CONFIG_EXAMPLE_USE_SPI_ETHERNET
    ESP_GOTO_ON_FALSE(eth_handles_out != NULL && eth_cnt_out != NULL, ESP_ERR_INVALID_ARG,
                        err, TAG_ETH, "invalid arguments: initialized handles array or number of interfaces");
    
    eth_handles = calloc(SPI_ETHERNETS_NUM + INTERNAL_ETHERNETS_NUM, sizeof(esp_eth_handle_t));
    ESP_GOTO_ON_FALSE(eth_handles != NULL, ESP_ERR_NO_MEM, err, TAG_ETH, "no memory");

#if CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET
    eth_handles[eth_cnt] = eth_init_internal(NULL, NULL);
    if (eth_handles[eth_cnt]) {
        eth_cnt++;
    } else {
        ESP_LOGW(TAG_ETH, "Internal Ethernet init failed - hardware may not be present");
    }
#endif //CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET

#if CONFIG_EXAMPLE_USE_SPI_ETHERNET
    // Try SPI bus init but continue if it fails
    if (spi_bus_init() != ESP_OK) {
        ESP_LOGW(TAG_ETH, "SPI bus init failed - SPI Ethernet may not be available");
    } else {
        spi_eth_module_config_t spi_eth_module_config[CONFIG_EXAMPLE_SPI_ETHERNETS_NUM] = { 0 };
        INIT_SPI_ETH_MODULE_CONFIG(spi_eth_module_config, 0);
        
        uint8_t base_mac_addr[ETH_ADDR_LEN];
        if (esp_efuse_mac_get_default(base_mac_addr) == ESP_OK) {
            uint8_t local_mac_1[ETH_ADDR_LEN];
            esp_derive_local_mac(local_mac_1, base_mac_addr);
            spi_eth_module_config[0].mac_addr = local_mac_1;
        }

        eth_handles[eth_cnt] = eth_init_spi(&spi_eth_module_config[0], NULL, NULL);
        if (eth_handles[eth_cnt]) {
            eth_cnt++;
        } else {
            ESP_LOGW(TAG_ETH, "SPI Ethernet init failed - hardware may not be present");
        }
    }
#endif // CONFIG_EXAMPLE_USE_SPI_ETHERNET

    if (eth_cnt > 0) {
        *eth_handles_out = eth_handles;
        *eth_cnt_out = eth_cnt;
    } else {
        free(eth_handles);
    }
#else
    ESP_LOGD(TAG_ETH, "no Ethernet device selected to init");
#endif // CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET || CONFIG_EXAMPLE_USE_SPI_ETHERNET

    return ret;

#if CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET || CONFIG_EXAMPLE_USE_SPI_ETHERNET
err:
    free(eth_handles);
    return ret;
#endif
}

esp_err_t example_eth_deinit(esp_eth_handle_t *eth_handles, uint8_t eth_cnt)
{
    ESP_RETURN_ON_FALSE(eth_handles != NULL, ESP_ERR_INVALID_ARG, TAG_ETH, "array of Ethernet handles cannot be NULL");
    for (int i = 0; i < eth_cnt; i++) {
        esp_eth_mac_t *mac = NULL;
        esp_eth_phy_t *phy = NULL;
        if (eth_handles[i] != NULL) {
            esp_eth_get_mac_instance(eth_handles[i], &mac);
            esp_eth_get_phy_instance(eth_handles[i], &phy);
            ESP_RETURN_ON_ERROR(esp_eth_driver_uninstall(eth_handles[i]), TAG_ETH, "Ethernet %p uninstall failed", eth_handles[i]);
        }
        if (mac != NULL) {
            mac->del(mac);
        }
        if (phy != NULL) {
            phy->del(phy);
        }
    }
#if CONFIG_EXAMPLE_USE_SPI_ETHERNET
    spi_bus_free(CONFIG_EXAMPLE_ETH_SPI_HOST);
#if (CONFIG_EXAMPLE_ETH_SPI_INT0_GPIO >= 0) || (CONFIG_EXAMPLE_ETH_SPI_INT1_GPIO > 0)
    // We installed the GPIO ISR service so let's uninstall it too.
    // BE CAREFUL HERE though since the service might be used by other functionality!
    if (gpio_isr_svc_init_by_eth) {
        ESP_LOGW(TAG_ETH, "uninstalling GPIO ISR service!");
        gpio_uninstall_isr_service();
    }
#endif
#endif //CONFIG_EXAMPLE_USE_SPI_ETHERNET
    free(eth_handles);
    return ESP_OK;
}

 const char *TAG = "eth_example";

/** Event handler for Ethernet events */
 
// **Ethernet Event Handler**
/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
    if (event_id == ETHERNET_EVENT_CONNECTED) {
        EthConnectedFlag = true;
        ESP_LOGI("ETH", "Ethernet baglandi!");
        //vTaskResume(mongoose_task_handle);
        wifi_deinit_ap();  // WiFi'yi kapat
    }  else if (event_id == ETHERNET_EVENT_DISCONNECTED) {
    EthConnectedFlag = false;
    ESP_LOGW("ETH", "Ethernet baglantisi kesildi!");
    wifi_init_ap();   // AP modunu başlat

    //vTaskSuspend(mongoose_task_handle);
}
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
/** Event handler for IP_EVENT_ETH_GOT_IP */
 void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
// **Ethernet Başlatma**
esp_err_t ETHapp_main(void) {
    esp_err_t res;
    
    // Initialize networking and event loop
    res = esp_netif_init();
    if (res != ESP_OK) {
        ESP_LOGE("ETH", "esp_netif_init() failed: %s", esp_err_to_name(res));
            Alarm_Log("ETH Socket failed", &DeviceTime);
        return res;
    }

    res = esp_event_loop_create_default();
    if (res != ESP_OK && res != ESP_ERR_INVALID_STATE) {
        ESP_LOGE("ETH", "esp_event_loop_create_default() failed: %s", esp_err_to_name(res));
            Alarm_Log("ETH Socket failed", &DeviceTime);
        return res;
    }

    // Skip Ethernet initialization if no hardware is detected
    g_ethernet_available = false;
    
#if CONFIG_EXAMPLE_USE_SPI_ETHERNET
    // Try to initialize Ethernet, but don't fail if hardware isn't present
    res = example_eth_init(&eth_handles, &eth_port_cnt);
    if (res == ESP_OK && eth_port_cnt > 0) {
        g_ethernet_available = true;
        
        // Only proceed with Ethernet setup if hardware was detected
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netif = esp_netif_new(&cfg);
        if (!eth_netif) {
            ESP_LOGE("ETH", "esp_netif_new() failed");
            Alarm_Log("ETH Socket failed", &DeviceTime);
            return ESP_FAIL;
        }

        esp_eth_netif_glue_handle_t eth_netif_glue = esp_eth_new_netif_glue(eth_handles[0]);
        if (!eth_netif_glue) {
            ESP_LOGE("ETH", "esp_eth_new_netif_glue() failed");
            Alarm_Log("ETH Socket failed", &DeviceTime);

            return ESP_FAIL;
        }

        res = esp_netif_attach(eth_netif, eth_netif_glue);
        if (res != ESP_OK) {
            ESP_LOGE("ETH", "esp_netif_attach() failed: %s", esp_err_to_name(res));
            Alarm_Log("ETH Socket failed", &DeviceTime);

            return res;
        }

        // Event handlers
        esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL);
        esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL);

        // Start Ethernet
        for (int i = 0; i < eth_port_cnt; i++) {
            res = esp_eth_start(eth_handles[i]);
            if (res != ESP_OK) {
                ESP_LOGE("ETH", "esp_eth_start() port %d failed: %s", i, esp_err_to_name(res));
                Alarm_Log("ETH Socket failed", &DeviceTime);

                return res;
            }
        }
        
        ESP_LOGI("ETH", "Ethernet initialized successfully");
    } else {
        ESP_LOGW("ETH", "No Ethernet hardware detected, skipping initialization");
        Alarm_Log("ETH Socket failed", &DeviceTime);

    }
#else
    ESP_LOGW("ETH", "Ethernet support not configured, skipping initialization");
#endif

    return ESP_OK;
}