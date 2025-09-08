/*
 * SD_SPI.c
 *
 *  Created on: 27 Şub 2025
 *      Author: metesepetcioglu
 */


/* SD card and FAT filesystem example.
   This example uses SPI peripheral to communicate with SD card.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "SD_SPI.h"
#include "mongoose.h"
#include "Alarms.h"
#include "SpeakerDriver.h"

#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif
static const char *TAG_SD = "SD_CHECK";

#define EXAMPLE_MAX_CHAR_SIZE    64

const char *TAGSD = "example";





#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
const char* names[] = {"CLK ", "MOSI", "MISO", "CS  "};
const int pins[] = {CONFIG_EXAMPLE_PIN_CLK,
                    CONFIG_EXAMPLE_PIN_MOSI,
                    CONFIG_EXAMPLE_PIN_MISO,
                    CONFIG_EXAMPLE_PIN_CS};

const int pin_count = sizeof(pins)/sizeof(pins[0]);
#if CONFIG_EXAMPLE_ENABLE_ADC_FEATURE
const int adc_channels[] = {CONFIG_EXAMPLE_ADC_PIN_CLK,
                            CONFIG_EXAMPLE_ADC_PIN_MOSI,
                            CONFIG_EXAMPLE_ADC_PIN_MISO,
                            CONFIG_EXAMPLE_ADC_PIN_CS};
#endif //CONFIG_EXAMPLE_ENABLE_ADC_FEATURE

pin_configuration_t config = {
    .names = names,
    .pins = pins,
#if CONFIG_EXAMPLE_ENABLE_ADC_FEATURE
    .adc_channels = adc_channels,
#endif
};
#endif //CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration" menu.
// You can also change the pin assignments here by changing the following 4 lines.
#define PIN_NUM_MISO  CONFIG_EXAMPLE_PIN_MISO
#define PIN_NUM_MOSI  CONFIG_EXAMPLE_PIN_MOSI
#define PIN_NUM_CLK   CONFIG_EXAMPLE_PIN_CLK
#define PIN_NUM_CS    CONFIG_EXAMPLE_PIN_CS


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
esp_err_t s_example_write_file(const char *path, const char *data)
{
    ESP_LOGI(TAGSD, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAGSD, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fputs(data, f);
    fclose(f);
    ESP_LOGI(TAGSD, "File written");

    return ESP_OK;
}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
 esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAGSD, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAGSD, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAGSD, "Read from file: '%s'", line);

    return ESP_OK;
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void init_sd_card(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 10,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAGSD, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(TAGSD, "Using SPI peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // and the internal LDO power supply, we need to initialize the power supply first.
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    host.max_freq_khz = 15000;
  

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = CONFIG_EXAMPLE_SD_SPI_HOST;
    
    ret = spi_bus_initialize(CONFIG_EXAMPLE_SD_SPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAGSD, "Failed to initialize bus.");
        Alarm_Log("SD Card SPI bus init failed", &DeviceTime);
        return;
    }
    
    ESP_LOGI(TAGSD, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAGSD, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
                     Alarm_Log("SD Card mount failed (filesystem)", &DeviceTime);
        } else {
            ESP_LOGE(TAGSD, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
                     Alarm_Log("SD Card init failed", &DeviceTime);
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return;
    }
    ESP_LOGI(TAGSD, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.

    // First create a file.
    const char *file_hello = MOUNT_POINT"/hello.txt";
    char data[EXAMPLE_MAX_CHAR_SIZE];
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK) {
		Alarm_Log("SD Card write failed (hello.txt)", &DeviceTime);
        return;
    }

    const char *file_foo = MOUNT_POINT"/foo.txt";
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        if (unlink(file_foo) != 0) {
            Alarm_Log("SD Card file delete failed", &DeviceTime);
        }
    }

    // Rename original file
    ESP_LOGI(TAGSD, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAGSD, "Rename failed");
        Alarm_Log("SD Card file rename failed", &DeviceTime);
        return;
    }

    ret = s_example_read_file(file_foo);
    if (ret != ESP_OK) {
		Alarm_Log("SD Card read failed (foo.txt)", &DeviceTime);
        return;
    }

    // Format FATFS
#ifdef CONFIG_EXAMPLE_FORMAT_SD_CARD
    ret = esp_vfs_fat_sdcard_format(mount_point, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        return;
    }

    if (stat(file_foo, &st) == 0) {
        ESP_LOGI(TAG, "file still exists");
        return;
    } else {
        ESP_LOGI(TAG, "file doesn't exist, formatting done");
    }
#endif // CONFIG_EXAMPLE_FORMAT_SD_CARD

    const char *file_nihao = MOUNT_POINT"/nihao.txt";
    memset(data, 0, EXAMPLE_MAX_CHAR_SIZE);
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
    ret = s_example_write_file(file_nihao, data);
    if (ret != ESP_OK) {
		Alarm_Log("SD Card write failed (nihao.txt)", &DeviceTime);
        return;
    }

    //Open file for reading
    ret = s_example_read_file(file_nihao);
    if (ret != ESP_OK) {
		Alarm_Log("SD Card read failed (nihao.txt)", &DeviceTime);
        return;
    }

    // All done, unmount partition and disable SPI peripheral
     //  esp_vfs_fat_sdcard_unmount(mount_point, card);
   // ESP_LOGI(TAGSD, "Card unmounted");

    //deinitialize the bus after all devices are removed
   // spi_bus_free(host.slot);

    // Deinitialize the power control driver if it was used
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    ret = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete the on-chip LDO power control driver");
        return;
    }
#endif

// SD kart mount edildikten sonra:

}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void write_to_sd_card(void) {
  const char *filename = "/deneme1234596.txt";
  const char *data = "Merhaba SD Kart!\n";

  // Dosyaya veri yaz
  if (mg_file_write(&mg_fs_fat, filename, data, strlen(data))) {
    ESP_LOGE(TAGSD,"Veri basariyla yazildi");
  } else {
    ESP_LOGE(TAGSD,"Yazma hatasi");
  }
}
/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
bool is_sd_card_mounted() {
    size_t total = 0, used = 0;
    return (esp_vfs_fat_info(MOUNT_POINT, &total, &used) == ESP_OK);
}
/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/

void mg_sd_card_test(void) {
    if (!is_sd_card_mounted()) {
        MG_ERROR(("SD kart montaj edilmemis!"));
        return;
    }

    const char *test_path = MOUNT_POINT"/mongoose_test.txt";
    const char *test_data = "Mongoose FAT FS test verisi\n1234567890\n";

    // 1. Önce dosyayı POSIX ile oluştur
    FILE *posix_fp = fopen(test_path, "w");
    if (!posix_fp) {
        MG_ERROR(("POSIX fopen (yazma) hatasi: %d", errno));
        return;
    }
    size_t written = fwrite(test_data, 1, strlen(test_data), posix_fp);
    fclose(posix_fp);
    MG_INFO(("POSIX ile %d byte yazildi", written));

    // 2. Mongoose FS testleri
    MG_INFO(("Mongoose FS testi basliyor..."));

    // 2a. Dosya bilgisi alma testi
    size_t file_size = 0;
    if (mg_fs_posix.st(test_path, &file_size, NULL)) {
        MG_INFO(("Dosya boyutu (mg_fs_posix): %d bytes", file_size));
    } else {
        MG_ERROR(("mg_fs_posix.st hatasi"));
        return;
    }

    // 2b. Okuma testi
    void *fp = mg_fs_posix.op(test_path, MG_FS_READ);
    if (!fp) {
        MG_ERROR(("mg_fs_posix.op hatasi (okuma)"));
        return;
    }
    
    char buf[128] = {0};
    size_t read = mg_fs_posix.rd(fp, buf, sizeof(buf)-1);
    mg_fs_posix.cl(fp);
    
    if (read > 0) {
        MG_INFO(("mg_fs_posix ile okunan (%d byte): %.*s", read, read, buf));
    } else {
        MG_ERROR(("mg_fs_posix.rd hatasi"));
        return;
    }

    // 2c. Yazma testi (append)
    fp = mg_fs_posix.op(test_path, MG_FS_WRITE);
    if (!fp) {
        MG_ERROR(("mg_fs_posix.op hatasi (yazma)"));
        return;
    }
    // Pozisyonu sona al
    mg_fs_posix.sk(fp, file_size);
    const char *append_data = "\nEk veri\n";
    written = mg_fs_posix.wr(fp, append_data, strlen(append_data));
    mg_fs_posix.cl(fp);
    MG_INFO(("mg_fs_posix ile %d byte eklendi", written));

    // 3. Son kontrol
    posix_fp = fopen(test_path, "r");
    if (!posix_fp) {
        MG_ERROR(("POSIX fopen (okuma) hatasi"));
        return;
    }
    char final_buf[256] = {0};
    read = fread(final_buf, 1, sizeof(final_buf)-1, posix_fp);
    fclose(posix_fp);
    MG_INFO(("Son durum (%d byte): %.*s", read, read, final_buf));
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/

void CheckSDFiles(char *json_buffer, size_t buffer_size) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char path[512];
    char *ptr = json_buffer;
    size_t remaining = buffer_size;
    bool first_entry = true;

    // JSON array start
    int written = snprintf(ptr, remaining, "[");
    if (written < 0 || (size_t)written >= remaining) {
        // Handle error: Buffer too small or snprintf failed at start
        strncpy(json_buffer, "[]", buffer_size); // Ensure valid JSON in case of error
        if (buffer_size > 0) json_buffer[buffer_size - 1] = '\0';
        return;
    }
    ptr += written;
    remaining -= written;

    dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        // If directory cannot be opened, return an empty JSON array
        strncpy(json_buffer, "[]", buffer_size);
        if (buffer_size > 0) json_buffer[buffer_size - 1] = '\0';
        return;
    }

    // Iterate through directory entries
    while ((entry = readdir(dir)) != NULL) {
        char *ext = strrchr(entry->d_name, '.');
        // Check for .wav extension (case-insensitive)
        if (!ext || strcasecmp(ext, ".wav") != 0) {
            continue;
        }

        // Construct full path
        snprintf(path, sizeof(path), "%s/%s", MOUNT_POINT, entry->d_name);
        
        // Get file stats (modification time, size, etc.)
        if (stat(path, &file_stat) == 0) {
            float duration = CheckWavDuration(path); // This correctly returns a float

            struct tm *timeinfo = localtime(&file_stat.st_mtime);
            char modtime_str[32];
            strftime(modtime_str, sizeof(modtime_str), "%Y-%m-%d %H:%M", timeinfo);

            // Ensure there's enough space for the entry + comma (if not first) + closing bracket
            if (remaining < 200) { // Keep this buffer size check
                break; // Buffer is too small, stop adding entries
            }

            // Add comma separator if it's not the first entry
            if (!first_entry) {
                written = snprintf(ptr, remaining, ",");
                if (written < 0 || (size_t)written >= remaining) break;
                ptr += written;
                remaining -= written;
            }
            first_entry = false;

            // YENİ DÜZENLEME: Süreyi "X.YY Sec" formatında bir string olarak basıyoruz.
            // Bu durumda JSON'da "size":"0.57 Sec" şeklinde bir string değeri olacaktır.
            // Eğer JavaScript'te bu değeri sayı olarak kullanmanız gerekiyorsa
            // JavaScript tarafında parse etmeniz gerekebilir (parseFloat() ile).
            written = snprintf(ptr, remaining,
                "{\"name\":\"%s\",\"lastModified\":\"%s\",\"size\":\"%.2f Sec\"}", // Değişiklik burada: %.2f'nin etrafına tırnaklar ve " Sec" eklendi
                entry->d_name, modtime_str, duration); // duration hala float olarak geçiyor

            if (written < 0 || (size_t)written >= remaining) {
                break; // Error or buffer full
            }
            ptr += written;
            remaining -= written;
        }
    }
    closedir(dir);

    // JSON array end
    // Ensure there's space for the closing bracket and null terminator
    if (remaining > 0) {
        snprintf(ptr, remaining, "]");
    } else {
        // Fallback for extremely full buffer: attempt to close array if possible
        if (buffer_size > 0) {
            if (ptr > json_buffer && *(ptr - 1) == ',') {
                 *(ptr - 1) = ']';
                 *ptr = '\0';
            } else if (buffer_size >= 2) { // Ensure at least 2 bytes for "]\0"
                 json_buffer[buffer_size - 2] = ']';
                 json_buffer[buffer_size - 1] = '\0';
            } else if (buffer_size == 1) { // Only space for '\0'
                json_buffer[0] = '\0';
            } 
        }
    }
}


void checkFileName(const char *filename) {
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/sdcard/%s", filename);

    ESP_LOGI(TAG_SD, "Dosya kontrol ediliyor: %s", full_path);

    struct stat st;
    if (stat(full_path, &st) == 0) {
        // Dosya bulundu. Başarılı bir JSON yanıtı gönder.
        ESP_LOGI(TAG_SD, "Dosya bulundu.");
        mg_http_reply(NULL, 200, "Content-Type: application/json\r\n", "{\"success\":true, \"message\":\"Dosya bulundu.\", \"filename\":\"%s\"}\n", filename);
    } else {
        // Hata durumunda bir JSON yanıtı gönder.
        char response_buffer[256];
        if (errno == ENOENT) {
            // Dosya mevcut değilse
            ESP_LOGW(TAG_SD, "Dosya bulunamadi.");
            snprintf(response_buffer, sizeof(response_buffer), "{\"success\":false, \"error\":\"Dosya bulunamadi\", \"filename\":\"%s\"}\n", filename);
            mg_http_reply(NULL, 404, "Content-Type: application/json\r\n", "%s", response_buffer);
        } else {
            // Diğer hatalar (SD kart bağlı değil vb.)
            ESP_LOGE(TAG_SD, "Dosya kontrolu sirasinda hata olustu: %s", strerror(errno));
            snprintf(response_buffer, sizeof(response_buffer), "{\"success\":false, \"error\":\"Dosya kontrolu sirasinda hata olustu: %s\"}\n", strerror(errno));
            mg_http_reply(NULL, 500, "Content-Type: application/json\r\n", "%s", response_buffer);
        }
    }
}