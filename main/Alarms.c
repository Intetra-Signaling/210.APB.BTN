/*
 * Alarms.c
 *
 *  Created on: 23 Tem 2025
 *
 * @file
 * @brief Provides API functions to log, retrieve, print, clear, remove, and persist alarm entries.
 *
 * This module manages a circular buffer of alarm entries, each including a message and timestamp.
 * It supports safe logging with duplicate prevention, time handling, flash persistence, and formatted output.
 * All key operations for interacting with alarm data are implemented here.
 *
 * @company    INTETRA
 * @version    v.0.0.0.1
 * @creator    Mete SEPETCIOGLU
 * @update     Mete SEPETCIOGLU
 */

#include "Alarms.h"
#include <string.h>
#include "FlashConfig.h"

AlarmEntry_t alarm_buffer[MAX_ALARMS];
uint8_t alarm_count = 0;
uint8_t alarm_index = 0;
AlarmJsonLog_t json_logs_buffer[MAX_LOGS_JSON_EXPORT];

/**
 * @brief      Retrieves alarm logs formatted as JSON structures.
 *
 * @param[out] out_count Pointer to a variable that will receive the number of exported logs.
 * @return     Pointer to an array of AlarmJsonLog_t containing the formatted alarm logs.
 *
 * @details
 * This function exports alarm log entries from the internal alarm buffer, formats their date and message fields safely for JSON representation,
 * and copies them into a static buffer. The number of logs exported is limited by MAX_LOGS_JSON_EXPORT.
 * If truncation occurs during date or message formatting, proper null-termination is ensured.
 * The log level is set to LOG_LEVEL_ERROR for all exported logs.
 */
const AlarmJsonLog_t* Alarm_GetJsonStructLogs(uint8_t *out_count) {
    uint8_t count = (alarm_count < MAX_LOGS_JSON_EXPORT) ? alarm_count : MAX_LOGS_JSON_EXPORT;

    for (uint8_t i = 0; i < count; i++) {
        AlarmEntry_t *entry = &alarm_buffer[i];

        // Safer date formatting with explicit buffer size checks
        int written = snprintf(json_logs_buffer[i].date, sizeof(json_logs_buffer[i].date),
                             "20%02u-%02u-%02u %02u:%02u:%02u",
                             entry->year, entry->month, entry->day,
                             entry->hours, entry->minutes, entry->seconds);
        
        // Ensure null-termination if truncation occurred
        if (written >= sizeof(json_logs_buffer[i].date)) {
            json_logs_buffer[i].date[sizeof(json_logs_buffer[i].date) - 1] = '\0';
        }

        // Safer message copying
        strncpy(json_logs_buffer[i].message, entry->message, sizeof(json_logs_buffer[i].message) - 1);
        json_logs_buffer[i].message[sizeof(json_logs_buffer[i].message) - 1] = '\0';

        json_logs_buffer[i].level = LOG_LEVEL_ERROR;
    }

    if (out_count) *out_count = count;
    return json_logs_buffer;
}

/**
 * @brief      Logs a new alarm message with an associated timestamp.
 *
 * @param[in]  msg   The alarm message to be logged.
 * @param[in]  time  Pointer to an rtc_time_t structure containing the timestamp for the log entry. If NULL, default time values are used.
 *
 * @details
 * This function adds a new alarm entry to the alarm buffer if the message does not already exist,
 * preventing duplicate logs. The message is copied safely with proper null-termination.
 * If the provided time pointer is NULL, default values are assigned to the time fields.
 * The function manages the circular buffer for alarms, updating the index and count accordingly.
 * Optionally, the alarm entry can be saved to flash by enabling the call to Alarm_SaveToFlash().
 */
void Alarm_Log(const char *msg, rtc_time_t *time) {
    // Check if the alarm message already exists to avoid duplicates
    for (uint8_t i = 0; i < alarm_count; i++) {
        if (strncmp(alarm_buffer[i].message, msg, MAX_ALARM_MSG_LEN) == 0) {
            return; // Already logged, exit
        }
    }

    // Add new alarm entry
    // Copy the message safely, ensuring null-termination
    strncpy(alarm_buffer[alarm_index].message, msg, MAX_ALARM_MSG_LEN - 1);
    alarm_buffer[alarm_index].message[MAX_ALARM_MSG_LEN - 1] = '\0';

    // IMPORTANT: Check if the 'time' pointer is NULL before dereferencing it
    if (time != NULL) {
        alarm_buffer[alarm_index].hours = time->hours;
        alarm_buffer[alarm_index].minutes = time->minutes;
        alarm_buffer[alarm_index].seconds = time->seconds;
        alarm_buffer[alarm_index].day = time->day;
        alarm_buffer[alarm_index].month = time->month;
        alarm_buffer[alarm_index].year = time->year;
    } else {
        // If 'time' is NULL, set default (e.g., zero) values for time components
        // This prevents the LoadProhibited error.
        alarm_buffer[alarm_index].hours = 0;
        alarm_buffer[alarm_index].minutes = 0;
        alarm_buffer[alarm_index].seconds = 0;
        alarm_buffer[alarm_index].day = 0;
        alarm_buffer[alarm_index].month = 0;
        alarm_buffer[alarm_index].year = 0; // Or a specific default year like 2000
    }

    // Update alarm count and index for circular buffer
    alarm_index = (alarm_index + 1) % MAX_ALARMS;
    if (alarm_count < MAX_ALARMS) {
        alarm_count++;
    }

    // Alarm_SaveToFlash();  // Re-enable if you want to save to flash after each log
}


/**
 * @brief      Saves the current alarm buffer data to flash memory.
 *
 * @return     esp_err_t Result of the flash write operation. Returns ESP_OK on success or an error code on failure.
 *
 * @details
 * This function copies the current alarm buffer, count, and index into a temporary structure,
 * and writes it to flash memory using a predefined flash key. The function returns the result
 * of the flash write operation, allowing the caller to check for success or failure.
 */
esp_err_t Alarm_SaveToFlash(void) {
    struct {
        AlarmEntry_t buffer[MAX_ALARMS];
        uint8_t count;
        uint8_t index;
    } flash_data;

    memcpy(flash_data.buffer, alarm_buffer, sizeof(alarm_buffer));
    flash_data.count = alarm_count;
    flash_data.index = alarm_index;

    return writeFlash(FLASH_ALARM_KEY, &flash_data, sizeof(flash_data));
}

/**
 * @brief      Saves the current alarm buffer to flash memory.
 *
 * @return     esp_err_t indicating the result of the flash write operation.
 *
 * @details
 * This function copies the current alarm buffer, count, and index into a temporary structure,
 * and writes it to flash memory using the specified flash key. 
 * It returns the result of the flash write operation.
 */
esp_err_t Alarm_LoadFromFlash(void) {
    struct {
        AlarmEntry_t buffer[MAX_ALARMS];
        uint8_t count;
        uint8_t index;
    } flash_data;

    size_t length = sizeof(flash_data);
    esp_err_t err = readFlash(FLASH_ALARM_KEY, &flash_data, &length);

    if (err == ESP_OK && length == sizeof(flash_data)) {
        memcpy(alarm_buffer, flash_data.buffer, sizeof(alarm_buffer));
        alarm_count = flash_data.count;
        alarm_index = flash_data.index;
    }

    return err;
}

 /**
 * @brief      Prints all alarm logs to the console.
 *
 * @details
 * This function iterates through the alarm buffer and prints each log entry,
 * including the date, time, and message, in a formatted manner.
 */
void Alarm_PrintAll(void) {
    printf("==== ALARM LOGS (%d adet) ====\n", alarm_count);
    for (uint8_t i = 0; i < alarm_count; i++) {
        AlarmEntry_t *entry = &alarm_buffer[i];
        printf("%02d/%02d/20%02d %02d:%02d:%02d | %s\n",
               entry->day, entry->month, entry->year,
               entry->hours, entry->minutes, entry->seconds,
               entry->message);
    }
}

/**
 * @brief      Clears all alarm logs from memory and optionally from flash.
 *
 * @details
 * This function resets the alarm buffer, count, and index to zero,
 * effectively removing all stored alarm logs. Optionally, it also erases
 * the alarm data from flash memory.
 */ 
void Alarm_ClearAll(void) {
    memset(alarm_buffer, 0, sizeof(alarm_buffer));
    alarm_count = 0;
    alarm_index = 0;
    writeFlash(FLASH_ALARM_KEY, NULL, 0);  // İsteğe bağlı: flash’tan da sil
}

/**
 * @brief      Retrieves a pointer to a specific alarm log entry by index.
 *
 * @param[in]  index  The index of the desired alarm log entry.
 * @return     Pointer to the AlarmEntry_t structure at the specified index,
 *             or NULL if the index is out of bounds.
 */ 
const AlarmEntry_t* Alarm_GetLog(uint8_t index) {
    if (index >= alarm_count) return NULL;
    return &alarm_buffer[index];
}
 
/**
 * @brief      Returns the current number of alarm logs stored.
 *
 * @return     The count of alarm logs in the buffer.
 */
uint8_t Alarm_GetCount(void) {
    return alarm_count;
}

 
/**
 * @brief      Removes an alarm log entry matching the specified message.
 *
 * @param[in]  msg  The message of the alarm log to remove.
 *
 * @details
 * This function searches for an alarm log matching the provided message.
 * If found, it shifts the remaining entries to fill the gap, updates the
 * alarm count and index, saves the updated buffer to flash, and prints a
 * confirmation message.
 */
void Alarm_Remove(const char *msg) {
    for (uint8_t i = 0; i < alarm_count; i++) {
        if (strncmp(alarm_buffer[i].message, msg, MAX_ALARM_MSG_LEN) == 0) {
            // Alarmı silmek için diziyi kaydır
            for (uint8_t j = i; j < alarm_count - 1; j++) {
                alarm_buffer[j] = alarm_buffer[j + 1];
            }
            alarm_count--;
            if (alarm_index == 0)
                alarm_index = MAX_ALARMS - 1;
            else
                alarm_index--;

            Alarm_SaveToFlash();  // Güncel durumu flash'a kaydet
            printf("[ALARM] '%s' alarmı silindi.\n", msg);
            return;
        }
    }
}

