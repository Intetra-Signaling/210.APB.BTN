/*
 * SystemTime.c
 *
 *  Created on: 29 Nis 2025
 *      Author: metesepetcioglu
 */

#include "SystemTime.h"
#include "Alarms.h"
 



#define TAG_RTC "MCP7940N"

uint32_t currentTime_epoch;  // Epoch zaman saniye cinsinden
rtc_time_t DeviceTime;


/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
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

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
 uint8_t bcd_to_dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
 uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
esp_err_t mcp7940n_init() {
    rtc_time_t dummy;
    return mcp7940n_get_time(&dummy);  // sadece bağlantı testi için
}

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
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




/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
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

/*******************************************************************************
* Function Name  			: None
* Description    			: None
* Input         			: None
* Output        			: None
* Return        			: None
*******************************************************************************/
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