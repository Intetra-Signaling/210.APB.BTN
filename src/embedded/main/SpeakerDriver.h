/*
 * SpeakerDriver.h
 *
 *  Created on: 7 Mar 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_SPEAKERDRIVER_H_
#define MAIN_SPEAKERDRIVER_H_

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"	
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_err.h"
 
// I2S Pinleri
//#define I2S_BCLK      26
//#define I2S_WS        27
//#define I2S_DOUT      17
#define I2S_BCLK      27
#define I2S_WS        26
#define I2S_DOUT      4
#define WAV_HEADER_SIZE  44

extern volatile bool is_idle_finished;
extern volatile bool is_voice_played;

void play_wav(const char *file_path);
float CheckWavDuration(char *file_path);
extern bool StopPlayWav; // Global değişken
void play_countdown_audio(int countdown_value);
void play_wav_counter(const char* filename);
void play_wav_idle(const char *file_path);
void play_wav_dac(const char* path);
void play_wav_blocking(const char* filename);
bool is_audio_hardware_busy();
void start_audio_playback(const char* file_path, float duration);
#endif /* MAIN_SPEAKERDRIVER_H_ */
