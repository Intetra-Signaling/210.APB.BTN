/*
 * SD_SPI.h
 *
 *  Created on: 27 Şub 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_SD_SPI_H_
#define MAIN_SD_SPI_H_

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
//#include "sd_test_io.h"


#define CONFIG_EXAMPLE_PIN_MOSI 12
#define CONFIG_EXAMPLE_PIN_MISO 13
#define CONFIG_EXAMPLE_PIN_CLK 14
#define CONFIG_EXAMPLE_PIN_CS 15
#define CONFIG_EXAMPLE_SD_SPI_HOST 1

//I2C pinleri -> IO21 SDA - IO22 SCL


#define MOUNT_POINT "/sdcard"





esp_err_t s_example_write_file(const char *path, const char *data);
esp_err_t s_example_read_file(const char *path);
void init_sd_card(void);
void write_to_sd_card(void);
bool is_sd_card_mounted();
void mg_sd_card_test(void);
void CheckSDFiles(char *json_buffer, size_t buffer_size);
void checkFileName(const char *filename);
bool DeleteSDFile(const char *file_path);
bool ClearAllSDFiles(void);

#endif /* MAIN_SD_SPI_H_ */
