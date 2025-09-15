/*
 * SpeakerDriver.c
 *
 *  Created on: 7 Mar 2025
 *      Author: metesepetcioglu
 */

#include "SpeakerDriver.h"
#include "SD_SPI.h"
#include "driver/dac_types.h"
#include "driver/i2s_common.h"
#include "driver/ledc.h"
#include "driver/i2s_std.h"
#include <stdint.h>
#include "esp_timer.h"
#include "driver/dac_continuous.h" // Yeni DAC sürücüsü için başlık dosyası
#include "driver/gpio.h"
#include "driver/dac_continuous.h"
#include <errno.h>     // errno değişkeni

//#include "i2s_stream.h"


i2s_chan_handle_t tx_handle;

/* Get the default channel configuration by the helper macro.
 * This helper macro is defined in `i2s_common.h` and shared by all the I2S communication modes.
 * It can help to specify the I2S role and port ID */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);

bool I2S_Channel_Enable = false;
bool I2S_Init_Enable = false;
volatile float volume_factor = 0.02;  // Başlangıç seviyesi
bool StopPlayWav = false; // Global değişken
volatile bool is_idle_finished;
volatile bool is_counter_voice_played;
volatile bool is_voice_played;
#define WAV_FILE_PATH "/sdcard/%d.wav"
#define BUFFER_SIZE_I2S      4096    
    

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
// ** I2S Başlatma **
void init_i2s(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t num_channels)
{
    /* Allocate a new TX channel and get the handle of this channel */
    if(I2S_Channel_Enable==false)
    {
		 i2s_new_channel(&chan_cfg, &tx_handle, NULL);
		 I2S_Channel_Enable = true;
	}
   
   i2s_data_bit_width_t bit_width;
    if (bits_per_sample == 8) {
        bit_width = I2S_DATA_BIT_WIDTH_8BIT;
    } else if (bits_per_sample == 16) {
        bit_width = I2S_DATA_BIT_WIDTH_16BIT;
    } else if (bits_per_sample == 24) {
        bit_width = I2S_DATA_BIT_WIDTH_24BIT;
    }  else if (bits_per_sample == 32) {
        bit_width = I2S_DATA_BIT_WIDTH_32BIT;
    } else {
        printf("Desteklenmeyen bit derinligi: %u\n", bits_per_sample);
        return;
    }
    
  
    /* Kanal sayısına göre stereo veya mono yapılandırma */
    i2s_slot_mode_t slot_mode = (num_channels == 1) ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO;
    i2s_std_config_t std_cfg = {
    .clk_cfg      = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
    .slot_cfg     = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(bit_width, slot_mode),
    .gpio_cfg     = {
    .mclk         = I2S_GPIO_UNUSED,
    .bclk         = I2S_BCLK,
    .ws           = I2S_WS,
    .dout         = I2S_DOUT,
    .din          = I2S_GPIO_UNUSED,
    .invert_flags = {
    .mclk_inv     = false,
    .bclk_inv     = false,
    .ws_inv       = false,
        },
    },
};

      /* Initialize the channel */
      if(I2S_Init_Enable == false)
      {
	   i2s_channel_init_std_mode(tx_handle, &std_cfg); 
       I2S_Init_Enable = true;
      }
      else
      {
	  i2s_channel_reconfig_std_clock(tx_handle, &std_cfg.clk_cfg);
	  i2s_channel_reconfig_std_gpio(tx_handle, &std_cfg.gpio_cfg);
	  i2s_channel_reconfig_std_slot(tx_handle, &std_cfg.slot_cfg);
      }
 i2s_channel_enable(tx_handle);
}



/*******************************************************************************
* Function Name  			: play_wav
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void play_wav_idle(const char *file_path) {
	
	FILE *wav_file = fopen(file_path, "rb");
    if (!wav_file) {
        printf("WAV dosyasi acilamadi: %s (errno=%d -> %s)\n",
           file_path, errno, strerror(errno));
    return;
    }

    uint8_t header[WAV_HEADER_SIZE];
    if (fread(header, 1, WAV_HEADER_SIZE, wav_file) != WAV_HEADER_SIZE) {
        printf("WAV basligi okunamadi veya eksik!\n");
        fclose(wav_file);
        return;
    }
   // Validate the WAV header
    if (strncmp((char *)header, "RIFF", 4) != 0 || strncmp((char *)&header[8], "WAVE", 4) != 0) {
        printf("Gecersiz WAV dosyasi!\n");
        fclose(wav_file);
        return;
    }
    uint32_t sample_rate = *(uint32_t*)&header[24];  // 24. bayttan itibaren örnekleme hızı
    uint16_t bits_per_sample = *(uint16_t*)&header[34];  // 34. bayttan itibaren bit derinliği
    uint16_t num_channels = *(uint16_t*)&header[22]; 

    if (num_channels == 1) {
     printf("Ses dosyasi MONO\n");
    } else if (num_channels == 2) {
    printf("Ses dosyasi STEREO\n");
    } else {
    num_channels = 1;
    printf("Bilinmeyen kanal sayisi: %u\n", num_channels);
   }

    printf("Ornekleme Hizi: %" PRIu32 " Hz, Bit Derinligi: %" PRIu16 " bit\n", sample_rate, bits_per_sample);
    init_i2s(sample_rate, bits_per_sample,num_channels);  // I2S başlat

    uint8_t *buffer = (uint8_t *)malloc(BUFFER_SIZE_I2S);
    size_t bytes_read, bytes_written;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE_I2S, wav_file)) > 0) {
		
       if (bits_per_sample == 16) {
        int16_t *sample_buffer = (int16_t *)buffer;
        size_t sample_count = bytes_read / 2;

         for (size_t i = 0; i < sample_count; i++) {
            int32_t sample = (int32_t)(sample_buffer[i] * volume_factor);

            // 16-bit için sınır kontrolü
            if (sample > INT16_MAX) sample = INT16_MAX;
            if (sample < INT16_MIN) sample = INT16_MIN;

            sample_buffer[i] = (int16_t)sample;
         }
        } else if (bits_per_sample == 8) {
        uint8_t *sample_buffer = buffer;
        size_t sample_count = bytes_read;

        for (size_t i = 0; i < sample_count; i++) {
            int16_t sample = (int16_t)(sample_buffer[i] - 128); // 8-bit PCM offset düzeltme
            sample = (int16_t)(sample * volume_factor);

            if (sample < -128) sample = -128;
            if (sample > 127) sample = 127;

            sample_buffer[i] = (uint8_t)(sample + 128);
          }
         } else if (bits_per_sample == 24) {
            size_t sample_count = bytes_read / 3;

        for (size_t i = 0; i < sample_count; i++) {
            int32_t sample = (buffer[i * 3] | (buffer[i * 3 + 1] << 8) | (buffer[i * 3 + 2] << 16));

            if (sample & 0x800000) sample |= 0xFF000000; // İşaret genişletme (sign extension)

            sample = (int32_t)(sample * volume_factor);

            if (sample > 8388607) sample = 8388607; // 24-bit maksimum
            if (sample < -8388608) sample = -8388608; // 24-bit minimum

            buffer[i * 3] = sample & 0xFF;
            buffer[i * 3 + 1] = (sample >> 8) & 0xFF;
            buffer[i * 3 + 2] = (sample >> 16) & 0xFF;
        }
        } else if (bits_per_sample == 32) {
        int32_t *sample_buffer = (int32_t *)buffer;
        size_t sample_count = bytes_read / 4;

        for (size_t i = 0; i < sample_count; i++) {
            int64_t sample = (int64_t)(sample_buffer[i] * volume_factor);

            if (sample > INT32_MAX) sample = INT32_MAX;
            if (sample < INT32_MIN) sample = INT32_MIN;
            sample_buffer[i] = (int32_t)sample;
        }
    }
       
        // I2S'e yaz
        size_t bytes_to_write = bytes_read;
        esp_err_t ret = i2s_channel_write(tx_handle, buffer, bytes_to_write, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            printf("I2S yazma hatası!\n");
            break;
        }
    }
    i2s_channel_disable(tx_handle);
    free(buffer);
    fclose(wav_file);
    printf("WAV dosyasi oynatildi ve buffer bosaltildi!\n");
 
}
 


/*******************************************************************************
* Function Name  			: play_wav
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void play_wav(const char *file_path) {
	
	FILE *wav_file = fopen(file_path, "rb");
    if (!wav_file) {
        printf("WAV dosyasi acilamadi: %s (errno=%d -> %s)\n",file_path, errno, strerror(errno));
    return;
    }
    uint8_t header[WAV_HEADER_SIZE];
    if (fread(header, 1, WAV_HEADER_SIZE, wav_file) != WAV_HEADER_SIZE) {
        printf("WAV basligi okunamadi veya eksik!\n");
        fclose(wav_file);
        return;
    }
   // Validate the WAV header
    if (strncmp((char *)header, "RIFF", 4) != 0 || strncmp((char *)&header[8], "WAVE", 4) != 0) {
        printf("Gecersiz WAV dosyasi!\n");
        fclose(wav_file);
        return;
    }
    uint32_t sample_rate = *(uint32_t*)&header[24];  // 24. bayttan itibaren örnekleme hızı
    uint16_t bits_per_sample = *(uint16_t*)&header[34];  // 34. bayttan itibaren bit derinliği
    uint16_t num_channels = *(uint16_t*)&header[22]; 

    if (num_channels == 1) {
     printf("Ses dosyasi MONO\n");
    } else if (num_channels == 2) {
    printf("Ses dosyasi STEREO\n");
    } else {
    num_channels = 1;
    printf("Bilinmeyen kanal sayisi: %u\n", num_channels);
   }

    printf("Ornekleme Hizi: %" PRIu32 " Hz, Bit Derinligi: %" PRIu16 " bit\n", sample_rate, bits_per_sample);
    init_i2s(sample_rate, bits_per_sample,num_channels);  // I2S başlat

    uint8_t *buffer = (uint8_t *)malloc(BUFFER_SIZE_I2S);
    size_t bytes_read, bytes_written;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE_I2S, wav_file)) > 0) {
		
       if (bits_per_sample == 16) {
        int16_t *sample_buffer = (int16_t *)buffer;
        size_t sample_count = bytes_read / 2;

         for (size_t i = 0; i < sample_count; i++) {
            int32_t sample = (int32_t)(sample_buffer[i] * volume_factor);

            // 16-bit için sınır kontrolü
            if (sample > INT16_MAX) sample = INT16_MAX;
            if (sample < INT16_MIN) sample = INT16_MIN;

            sample_buffer[i] = (int16_t)sample;
         }
        } else if (bits_per_sample == 8) {
        uint8_t *sample_buffer = buffer;
        size_t sample_count = bytes_read;

        for (size_t i = 0; i < sample_count; i++) {
            int16_t sample = (int16_t)(sample_buffer[i] - 128); // 8-bit PCM offset düzeltme
            sample = (int16_t)(sample * volume_factor);

            if (sample < -128) sample = -128;
            if (sample > 127) sample = 127;

            sample_buffer[i] = (uint8_t)(sample + 128);
          }
         } else if (bits_per_sample == 24) {
            size_t sample_count = bytes_read / 3;

        for (size_t i = 0; i < sample_count; i++) {
            int32_t sample = (buffer[i * 3] | (buffer[i * 3 + 1] << 8) | (buffer[i * 3 + 2] << 16));

            if (sample & 0x800000) sample |= 0xFF000000; // İşaret genişletme (sign extension)

            sample = (int32_t)(sample * volume_factor);

            if (sample > 8388607) sample = 8388607; // 24-bit maksimum
            if (sample < -8388608) sample = -8388608; // 24-bit minimum

            buffer[i * 3] = sample & 0xFF;
            buffer[i * 3 + 1] = (sample >> 8) & 0xFF;
            buffer[i * 3 + 2] = (sample >> 16) & 0xFF;
        }
        } else if (bits_per_sample == 32) {
        int32_t *sample_buffer = (int32_t *)buffer;
        size_t sample_count = bytes_read / 4;

        for (size_t i = 0; i < sample_count; i++) {
            int64_t sample = (int64_t)(sample_buffer[i] * volume_factor);

            if (sample > INT32_MAX) sample = INT32_MAX;
            if (sample < INT32_MIN) sample = INT32_MIN;
            sample_buffer[i] = (int32_t)sample;
        }
    }
       
        // I2S'e yaz
        size_t bytes_to_write = bytes_read;
        esp_err_t ret = i2s_channel_write(tx_handle, buffer, bytes_to_write, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            printf("I2S yazma hatası!\n");
            break;
        }
    }
        // *** KRİTİK: I2S BUFFER'INI TAM BOŞALT ***
    printf("I2S buffer bosaltiliyor...\n");
    
    // Flush buffer ile sıfır gönder
    uint8_t *zero_buffer = (uint8_t *)calloc(BUFFER_SIZE_I2S, 1);
    if (zero_buffer != NULL) {
        size_t zero_written;
        esp_err_t flush_ret = i2s_channel_write(tx_handle, zero_buffer, BUFFER_SIZE_I2S, &zero_written, 
                                               pdMS_TO_TICKS(500));  // 500ms timeout
        printf("Flush buffer yazildi: %zu bytes (ret: %s)\n", zero_written, esp_err_to_name(flush_ret));
        free(zero_buffer);
    }
    
    // Buffer'ın tamamen boşalması için bekle
    vTaskDelay(pdMS_TO_TICKS(200)); // 200ms bekle - SES TAMAMEN BİTSİN
    
    i2s_channel_disable(tx_handle);
    free(buffer);
    fclose(wav_file);
    printf("WAV dosyasi oynatildi ve buffer bosaltildi!\n");
}
 


/*******************************************************************************
* Function Name  			: CheckWavDuration
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
float  CheckWavDuration(char *file_path)
{
    FILE *wav_file = fopen(file_path, "rb");
    if (!wav_file) {
        printf("Hata: WAV dosyasi acilamadi! -> %s\n", file_path);
        return 0;
    }

    // Gerekli başlık bilgilerini okumak için bir tampon
    uint8_t header[44];
    if (fread(header, 1, 12, wav_file) != 12) {
        printf("Hata: Gecersiz RIFF basligi!\n");
        fclose(wav_file);
        return 0;
    }

    // RIFF ve WAVE format kontrolü
    if (strncmp((char*)header, "RIFF", 4) != 0 || strncmp((char*)&header[8], "WAVE", 4) != 0) {
        printf("Hata: Gecerli bir WAV dosyasi degil!\n");
        fclose(wav_file);
        return 0;
    }

    // "fmt " bölümünden temel bilgileri oku
    // Bu bölüm genellikle dosyanın başında yer alır.
    fseek(wav_file, 12, SEEK_SET); // "fmt " chunk'ının başlangıcına git
    
    if (fread(header, 1, 24, wav_file) != 24) { // "fmt " chunk'ını oku
         printf("Hata: 'fmt ' bolumu okunamadi!\n");
         fclose(wav_file);
         return 0;
    }
    
    uint32_t sample_rate = *(uint32_t*)&header[12]; // 12. byte'tan itibaren (fmt chunk'ının 8. byte'ı)
    uint16_t num_channels = *(uint16_t*)&header[10];
    uint16_t bits_per_sample = *(uint16_t*)&header[22];

    // "data" chunk'ını arama döngüsü
    char chunk_id[5] = {0};
    uint32_t data_size = 0;

    while (fread(chunk_id, 1, 4, wav_file) == 4) {
        // chunk boyutunu oku
        uint32_t chunk_size;
        if (fread(&chunk_size, sizeof(uint32_t), 1, wav_file) != 1) {
            printf("Hata: Chunk boyutu okunamadi!\n");
            break;
        }

        if (strncmp(chunk_id, "data", 4) == 0) {
            data_size = chunk_size;
            break; // "data" bölümünü bulduk, döngüden çık
        } else {
            // Bu "data" bölümü değil, chunk_size kadar ilerle
            fseek(wav_file, chunk_size, SEEK_CUR);
        }
    }

    fclose(wav_file);

    if (data_size == 0) {
        printf("Hata: 'data' bolumu bulunamadi veya boyutu sifir!\n");
        return 0;
    }

    // Hesaplamalar
    uint32_t bytes_per_sample = (bits_per_sample / 8) * num_channels;
    if (bytes_per_sample == 0 || sample_rate == 0) {
        printf("Hata: Gecersiz WAV parametreleri! (Ornekleme Hizi/Bit Derinligi sifir olamaz)\n");
        return 0;
    }

    float duration_seconds = (float)data_size / (float)(sample_rate * bytes_per_sample);
    printf("WAV Suresi: %.2f saniye\n", duration_seconds);

    // Süreyi en yakın saniyeye yuvarla
    return (float)(duration_seconds + 0.5f);
}

/*******************************************************************************
* Function Name  			: play_countdown_audio
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void play_countdown_audio(int countdown_value) {
    char file_path[50];
    snprintf(file_path, sizeof(file_path), WAV_FILE_PATH, countdown_value);
    printf("Oynatilan dosya: %s\n", file_path);  // Terminale yazdır
    play_wav(file_path);
}


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
void play_wav_counter(const char* file_path) {
   FILE *wav_file = fopen(file_path, "rb");
    if (!wav_file) {
        printf("WAV dosyasi acilamadi!\n");
        return;
    }

    uint8_t header[WAV_HEADER_SIZE];
    if(fread(header, 1, WAV_HEADER_SIZE, wav_file) != WAV_HEADER_SIZE) {
        printf("WAV basligi okunamadi!\n");
        fclose(wav_file);
        return;
    }

    uint32_t sample_rate = *(uint32_t*)&header[24];
    uint16_t bits_per_sample = *(uint16_t*)&header[34];
    uint16_t num_channels = *(uint16_t*)&header[22];
    uint32_t byte_rate = *(uint32_t*)&header[28];

    printf("Ornekleme Hizi: %" PRIu32 " Hz, Bit Derinligi: %" PRIu16 " bit, Kanallar: %d\n", 
          sample_rate, bits_per_sample, num_channels);

    init_i2s(sample_rate, bits_per_sample, num_channels);

    uint8_t *buffer = (uint8_t *)malloc(BUFFER_SIZE_I2S);
    if(!buffer) {
        printf("Bellek ayrilamadi!\n");
        fclose(wav_file);
        return;
    }

    StopPlayWav = false;
    uint32_t start_time = esp_timer_get_time(); // Mikrosaniye cinsinden başlangıç zamanı
    uint32_t target_duration = 950000; // 850 ms mikrosaniye cinsinden
    size_t bytes_read, bytes_written;
    is_counter_voice_played = false;
    while (!StopPlayWav && (bytes_read = fread(buffer, 1, BUFFER_SIZE_I2S, wav_file)) > 0) {
        // Geçen süreyi kontrol et
        uint32_t elapsed_time = (esp_timer_get_time() - start_time);
        if(elapsed_time >= target_duration) {
            printf("850 ms ulasildi, durduruluyor.\n");
            StopPlayWav = true;
        }

        // Ses verilerini işle
        if (bits_per_sample == 16) {
            int16_t *sample_buffer = (int16_t *)buffer;
            size_t sample_count = bytes_read / 2;
            for (size_t i = 0; i < sample_count; i++) {
                int32_t sample = (int32_t)(sample_buffer[i] * volume_factor);
                sample_buffer[i] = (int16_t)(sample > INT16_MAX ? INT16_MAX : 
                                           (sample < INT16_MIN ? INT16_MIN : sample));
            }
        } else if (bits_per_sample == 8) {
        uint8_t *sample_buffer = buffer;
        size_t sample_count = bytes_read;

        for (size_t i = 0; i < sample_count; i++) {
            int16_t sample = (int16_t)(sample_buffer[i] - 128); // 8-bit PCM offset düzeltme
            sample = (int16_t)(sample * volume_factor);

            if (sample < -128) sample = -128;
            if (sample > 127) sample = 127;

            sample_buffer[i] = (uint8_t)(sample + 128);
          }
         } else if (bits_per_sample == 24) {
            size_t sample_count = bytes_read / 3;

        for (size_t i = 0; i < sample_count; i++) {
            int32_t sample = (buffer[i * 3] | (buffer[i * 3 + 1] << 8) | (buffer[i * 3 + 2] << 16));

            if (sample & 0x800000) sample |= 0xFF000000; // İşaret genişletme (sign extension)

            sample = (int32_t)(sample * volume_factor);

            if (sample > 8388607) sample = 8388607; // 24-bit maksimum
            if (sample < -8388608) sample = -8388608; // 24-bit minimum

            buffer[i * 3] = sample & 0xFF;
            buffer[i * 3 + 1] = (sample >> 8) & 0xFF;
            buffer[i * 3 + 2] = (sample >> 16) & 0xFF;
        }
        } else if (bits_per_sample == 32) {
        int32_t *sample_buffer = (int32_t *)buffer;
        size_t sample_count = bytes_read / 4;

        for (size_t i = 0; i < sample_count; i++) {
            int64_t sample = (int64_t)(sample_buffer[i] * volume_factor);

            if (sample > INT32_MAX) sample = INT32_MAX;
            if (sample < INT32_MIN) sample = INT32_MIN;
            sample_buffer[i] = (int32_t)sample;
        }
    }
       // Eğer durdurma bayrağı set edildiyse çık
        if (StopPlayWav) 
        {
		printf("PlayWav fonksiyonu durduruldu!\n");	
		break;
		}
        // Diğer bit derinlikleri için benzer işlemler...
        
        // I2S'e yaz
        esp_err_t ret = i2s_channel_write(tx_handle, buffer, bytes_read, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            printf("I2S yazma hatasi: %d\n", ret);
            break;
        }

        if (StopPlayWav) {
            printf("Oynatma durduruldu!\n");
            break;
        }
    }

    // Temizlik
    free(buffer);
    fclose(wav_file);
    i2s_channel_disable(tx_handle);
    printf("Oynatma tamamlandi.\n");
    is_counter_voice_played = true;
}

 


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/

#define DAC_CHANNEL_1_GPIO 25
static const char *TAG_DAC = "WAV_PLAYER";
#define DAC_OUTPUT_PIN DAC_CHAN_0_GPIO_NUM // DAC_CHAN_0 için GPIO numarası (GPIO25)

typedef struct {
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint16_t num_channels;
} wav_header_t;

// WAV başlığını okuma fonksiyonu
bool read_wav_header(FILE* f, wav_header_t* header) {
    uint8_t header_buf[44];
    if (fread(header_buf, 1, 44, f) != 44) {
        ESP_LOGE(TAG_DAC, "WAV basligi okunamadi");
        return false;
    }

  // Örnekleme hızı (byte 24-27) - Little Endian
    header->sample_rate = (uint32_t)header_buf[24] | 
                        ((uint32_t)header_buf[25] << 8) |
                        ((uint32_t)header_buf[26] << 16) |
                        ((uint32_t)header_buf[27] << 24);
    
    // Bit derinliği (byte 34-35) - Little Endian
    header->bits_per_sample = (uint16_t)header_buf[34] | 
                            ((uint16_t)header_buf[35] << 8);
    
    // Kanal sayısı (byte 22-23) - Little Endian
    header->num_channels = (uint16_t)header_buf[22] | 
                         ((uint16_t)header_buf[23] << 8);

    ESP_LOGI(TAG_DAC, "WAV Bilgileri: %"PRIu32" Hz, %"PRIu16"-bit, %"PRIu16" kanal",
            header->sample_rate, header->bits_per_sample, header->num_channels);

    return true;
}
 