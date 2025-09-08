/*
 * ethernet_init.h
 *
 *  Created on: 7 Mar 2025
 *      Author: metesepetcioglu
 */
#ifndef MAIN_ETHERNET_INIT_H_
#define MAIN_ETHERNET_INIT_H_

#pragma once



#include "esp_eth_driver.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "driver/spi_master.h"
#include "esp_event.h"
#include "lwip/sockets.h"
#include "esp_wifi.h"

#define SERVER_PORT 4001
//#define CONFIG_EXAMPLE_USE_W5500 y
#define CONFIG_EXAMPLE_ETH_SPI_HOST 2
#define CONFIG_EXAMPLE_ETH_SPI_SCLK_GPIO 18
#define CONFIG_EXAMPLE_ETH_SPI_MOSI_GPIO 19
#define CONFIG_EXAMPLE_ETH_SPI_MISO_GPIO 23
#define CONFIG_EXAMPLE_ETH_SPI_CLOCK_MHZ 16
#define CONFIG_EXAMPLE_ETH_SPI_CS0_GPIO 5
#define CONFIG_EXAMPLE_ETH_SPI_INT0_GPIO 33
#define CONFIG_EXAMPLE_ETH_SPI_POLLING0_MS 0
#define CONFIG_EXAMPLE_ETH_SPI_PHY_RST0_GPIO -1
#define CONFIG_EXAMPLE_ETH_SPI_PHY_ADDR0 1
#define CONFIG_EXAMPLE_SPI_ETHERNETS_NUM 1
//#define CONFIG_EXAMPLE_USE_SPI_ETHERNET y
#define CONFIG_ENV_GPIO_RANGE_MIN 0
#define CONFIG_ENV_GPIO_RANGE_MAX 39
#define CONFIG_ENV_GPIO_IN_RANGE_MAX 39
#define CONFIG_ENV_GPIO_OUT_RANGE_MAX 33
#define CONFIG_EXAMPLE_ETH_DEINIT_AFTER_S -1
#define CONFIG_EXAMPLE_USE_SPI_ETHERNET 1
#define CONFIG_EXAMPLE_SPI_ETHERNETS_NUM 1
#define CONFIG_EXAMPLE_USE_W5500 1
#define FILE_PATH "/sdcard/received.wav"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Ethernet driver based on Espressif IoT Development Framework Configuration
 *
 * @param[out] eth_handles_out array of initialized Ethernet driver handles
 * @param[out] eth_cnt_out number of initialized Ethernets
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_INVALID_ARG when passed invalid pointers
 *          - ESP_ERR_NO_MEM when there is no memory to allocate for Ethernet driver handles array
 *          - ESP_FAIL on any other failure
 */
esp_err_t example_eth_init(esp_eth_handle_t *eth_handles_out[], uint8_t *eth_cnt_out);

/**
 * @brief De-initialize array of Ethernet drivers
 * @note All Ethernet drivers in the array must be stopped prior calling this function.
 *
 * @param[in] eth_handles array of Ethernet drivers to be de-initialized
 * @param[in] eth_cnt number of Ethernets drivers to be de-initialized
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_INVALID_ARG when passed invalid pointers
 */
esp_err_t example_eth_deinit(esp_eth_handle_t *eth_handles, uint8_t eth_cnt);

#ifdef __cplusplus
}
#endif


void eth_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
void got_ip_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
esp_err_t ETHapp_main(void);
extern bool EthConnectedFlag;
#endif /* MAIN_ETHERNET_INIT_H_ */
