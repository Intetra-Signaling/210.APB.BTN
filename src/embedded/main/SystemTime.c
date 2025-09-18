/*
 * SystemTime.c
 *
 *  Created on: 29 Nis 2025
 *      Author: metesepetcioglu
 *
 * @file
 * @brief Provides functions for system time management using the MCP7940N RTC over I2C.
 *
 * This module includes I2C initialization, BCD conversion routines, and high-level functions
 * for reading and setting time from the MCP7940N real-time clock. All error handling and logging
 * for RTC communication and time validation is managed here.
 *
 * @company    INTETRA
 * @version    v.0.0.0.1
 * @creator    Mete SEPETCIOGLU
 * @update     Mete SEPETCIOGLU
 */


#include "SystemTime.h"
#include "Alarms.h"
 
 
#define TAG_RTC "MCP7940N"
uint32_t currentTime_epoch;  // Epoch zaman saniye cinsinden
rtc_time_t DeviceTime;



/**
 * @brief Initializes the I2C master interface with predefined configuration.
 *
 * Configures the I2C peripheral as master mode, sets the SDA and SCL pin numbers,
 * enables internal pull-up resistors for both lines, and sets the I2C clock speed.
 * Installs the I2C driver for subsequent communication with I2C devices.
 *
 * Example pins and speed are defined by macros:
 *   - I2C_MASTER_NUM: I2C port number
 *   - I2C_MASTER_SDA_IO: SDA pin number
 *   - I2C_MASTER_SCL_IO: SCL pin number
 *
 * Typical use-case: Call once during system initialization before any I2C communication.
 *
 * @return None
 */
void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

 /**
 * @brief Converts a BCD (Binary Coded Decimal) value to its decimal representation.
 *
 * Takes a single byte in BCD format and returns its equivalent decimal value.
 * BCD encoding stores each decimal digit in a nibble (4 bits).
 *
 * @param val The BCD value to convert.
 * @return uint8_t The decimal representation of the input BCD value.
 *
 * Example:
 *   bcd_to_dec(0x25); // returns 25
 */
 uint8_t bcd_to_dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

 /**
 * @brief Converts a decimal value to its BCD (Binary Coded Decimal) representation.
 *
 * Takes a decimal value (0-99) and returns its equivalent BCD-encoded byte.
 * In BCD encoding, the tens digit is stored in the upper nibble and the units digit in the lower nibble.
 *
 * @param val The decimal value to convert (0-99).
 * @return uint8_t The BCD representation of the input decimal value.
 *
 * Example:
 *   dec_to_bcd(25); // returns 0x25
 */
 uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}


/**
 * @brief Initializes the MCP7940N RTC module by testing communication.
 *
 * Attempts to read the current time from the MCP7940N real-time clock.
 * This function is typically used to verify I2C connectivity and RTC presence.
 *
 * @return
 *   - ESP_OK if communication and read were successful.
 *   - An error code (esp_err_t) if communication failed.
 */ 
esp_err_t mcp7940n_init() {
    rtc_time_t dummy;
    return mcp7940n_get_time(&dummy);  // sadece bağlantı testi için
}

 /**
 * @brief Sets the time on the MCP7940N RTC device.
 *
 * Validates the input time structure, converts the time fields to BCD format,
 * and writes the data to the MCP7940N over I2C. The oscillator bit is enabled
 * in the seconds register to start the RTC if necessary.
 *
 * @param[in] time Pointer to the rtc_time_t structure containing the time to set.
 *                 The structure fields must be within valid ranges:
 *                 - seconds: 0-59
 *                 - minutes: 0-59
 *                 - hours: 0-23
 *                 - day: 1-31
 *                 - month: 1-12
 *                 - year: 0-99 (since 2000)
 *                 - day_of_week: 1-7
 *
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if input is invalid
 *   - Other esp_err_t codes from I2C transaction
 */
esp_err_t mcp7940n_set_time(const rtc_time_t *time) {
    // Validate input time
    if (time == NULL) {
        Alarm_Log("RTC set time: NULL pointer", NULL);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate time values
    if (time->seconds > 59 || time->minutes > 59 || time->hours > 23 ||
        time->day < 1 || time->day > 31 || time->month < 1 || time->month > 12 ||
        time->year > 99 || time->day_of_week < 1 || time->day_of_week > 7) {
        Alarm_Log("RTC set time: Invalid time values", NULL);
        return ESP_ERR_INVALID_ARG;
    }
   
    uint8_t data[8];
    data[0] = 0x00;  // start register
    data[1] = dec_to_bcd(time->seconds) | 0x80;  // Ensure oscillator is enabled
    data[2] = dec_to_bcd(time->minutes);
    data[3] = dec_to_bcd(time->hours);
    data[4] = dec_to_bcd(time->day_of_week);
    data[5] = dec_to_bcd(time->day);
    data[6] = dec_to_bcd(time->month);
    data[7] = dec_to_bcd(time->year);

    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, MCP7940N_ADDR, 
                       data, 8, 1000 / portTICK_PERIOD_MS);

    if (ret != ESP_OK) {
        Alarm_Log("RTC set time: I2C write failed", NULL);
    } 

    return ret;
}


 /**
 * @brief Reads the current time from the MCP7940N RTC device.
 *
 * Communicates with the MCP7940N over I2C to read time registers, converts BCD values to decimal,
 * validates time fields, and calculates the corresponding epoch time. Updates the global
 * currentTime_epoch variable if successful. Logs errors for communication, oscillator status, or invalid data.
 *
 * @param[out] time Pointer to rtc_time_t structure to be filled with the current time.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_STATE if the oscillator is stopped
 *   - ESP_ERR_INVALID_RESPONSE if time values are out of range
 *   - ESP_ERR_INVALID_ARG if epoch conversion fails
 *   - Other esp_err_t codes from I2C transaction
 */
esp_err_t mcp7940n_get_time(rtc_time_t *time) {
    uint8_t reg = 0x00;
    uint8_t data[7];
    rtc_time_t error_time = {0}; // Default time for error cases

    // Read time from RTC
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, MCP7940N_ADDR, 
                       &reg, 1, data, 7, 1000 / portTICK_PERIOD_MS);
    
    if (ret != ESP_OK) {
        // Log I2C communication error
        Alarm_Log("RTC module failed", &error_time);
        return ret;
    }

    // Validate RTC data (basic checks)
    if ((data[0] & 0x80) != 0) {  // Check if oscillator is running
        Alarm_Log("RTC oscillator stopped", &error_time);
        return ESP_ERR_INVALID_STATE;
    }

    // Extract time components
    time->seconds     = bcd_to_dec(data[0] & 0x7F);
    time->minutes     = bcd_to_dec(data[1]);
    time->hours       = bcd_to_dec(data[2] & 0x3F);
    time->day_of_week = bcd_to_dec(data[3]);
    time->day         = bcd_to_dec(data[4]);
    time->month       = bcd_to_dec(data[5]);
    time->year        = bcd_to_dec(data[6]);

    // Validate time values
    if (time->seconds > 59 || time->minutes > 59 || time->hours > 23 ||
        time->day < 1 || time->day > 31 || time->month < 1 || time->month > 12) {
        Alarm_Log("Invalid RTC time values", time);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Calculate epoch time
    struct tm t = {
        .tm_sec  = time->seconds,
        .tm_min  = time->minutes,
        .tm_hour = time->hours,
        .tm_mday = time->day,
        .tm_mon  = time->month - 1,
        .tm_year = 2000 + time->year - 1900
    };
    
    time_t epoch = mktime(&t);
    if (epoch == -1) {
        Alarm_Log("Epoch time conversion failed", time);
        return ESP_ERR_INVALID_ARG;
    }
    
    currentTime_epoch = (uint32_t)epoch; 
    return ESP_OK;
}
 
 
 
 /**
 * @brief Sets the MCP7940N RTC time using individual time parameters.
 *
 * Constructs a temporary rtc_time_t structure using the provided parameters,
 * validates all fields, and writes the time to the RTC device.
 * Logs errors if any parameter is out of range or if the I2C communication fails.
 * On success, the time is set and an info log is generated.
 *
 * @param[in] Weekday Day of week (1-7, where 1 = Sunday)
 * @param[in] Month   Month (1-12)
 * @param[in] Day     Day of month (1-31)
 * @param[in] Year    Year (0-99, since 2000)
 * @param[in] Hours   Hour (0-23)
 * @param[in] Minutes Minute (0-59)
 * @param[in] Seconds Second (0-59)
 * @return
 *   - ESP_OK if RTC time was set successfully
 *   - ESP_ERR_INVALID_ARG if any input is out of range
 *   - Other esp_err_t codes from mcp7940n_set_time or I2C transaction
 */
esp_err_t set_rtc_time(int Weekday, int Month, int Day, int Year, int Hours, int Minutes, int Seconds) {
    // Create temporary time structure for validation and logging
    rtc_time_t temp_time = {
        .seconds = Seconds,
        .minutes = Minutes,
        .hours = Hours,
        .day_of_week = Weekday,
        .day = Day,
        .month = Month,
        .year = Year
    };

    // Validate input parameters
    if (Weekday < 1 || Weekday > 7 || Month < 1 || Month > 12 || 
        Day < 1 || Day > 31 || Year < 0 || Year > 99 ||
        Hours < 0 || Hours > 23 || Minutes < 0 || Minutes > 59 || 
        Seconds < 0 || Seconds > 59) {
        Alarm_Log("set_rtc_time: Invalid parameters", &temp_time);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = mcp7940n_set_time(&temp_time);
    
    if (ret != ESP_OK) {
        Alarm_Log("set_rtc_time: Failed to set time", &temp_time);
    } else {
        Alarm_Log("set_rtc_time: Time set successfully", &temp_time);
    }

    return ret;
}