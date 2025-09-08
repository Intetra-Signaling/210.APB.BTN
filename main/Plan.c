/*
 * Plan.c
 *
 *  Created on: 30 Nis 2025
 *      Author: metesepetcioglu
 */
 
#include "Plan.h"
#include "mongoose_glue.h"
struct xCurrentConfiguration CurrentConfiguration;
char CurrentPlan; //0,1,2,3 olabilir.



void GetCurrentPlan(void)
{
    const char* dayPlan = NULL;
    // Günlük plan (haftalık)
    switch (DeviceTime.day_of_week) {
        case 0: dayPlan = s_sunday.time; break;
        case 1: dayPlan = s_monday.time; break;
        case 2: dayPlan = s_tuesday.time; break;
        case 3: dayPlan = s_wednesday.time; break;
        case 4: dayPlan = s_thursday.time; break;
        case 5: dayPlan = s_friday.time; break;
        case 6: dayPlan = s_saturday.time; break;
        default: dayPlan = NULL; break;
    }
    
    // Özel gün planı var mı kontrol et
    const char* holidayPlan = NULL;
    // Tarihi karşılaştırmak için buffer
    char todayString[6];
    snprintf(todayString, sizeof(todayString), "%s%02d", getMonthAbbreviation(DeviceTime.month), DeviceTime.day);
	// Her bir özel günü kontrol et
	    const char* holidays[] = {
	        s_holidays.holiday1, s_holidays.holiday2, s_holidays.holiday3, s_holidays.holiday4, s_holidays.holiday5,
	        s_holidays.holiday6, s_holidays.holiday7, s_holidays.holiday8, s_holidays.holiday9, s_holidays.holiday10
	    };
	    
	    for (int i = 0; i < 10; i++) {
        if (strncmp(holidays[i], todayString, 5) == 0) {
            holidayPlan = holidays[i] + 5; // İlk 5 bayt tarih, sonrası 96 karakterlik plan
            break;
        }
    }
    
        if (holidayPlan != NULL && strlen(holidayPlan) >= 96) {
        dayPlan = holidayPlan; // Özel gün baskın olur
    }
	    
    if (dayPlan == NULL || strlen(dayPlan) < 96) {
        CurrentPlan = '0'; // geçersiz gün planı varsa varsayılan olarak plan 0
        return;
    }

    // Şu anki zamanın index'ini hesapla (her biri 15 dakikalık slot)
    int index = (DeviceTime.hours * 60 + DeviceTime.minutes) / 15;

    if (index < 0 || index >= 96) {
        CurrentPlan = '0'; // beklenmeyen bir şey olursa default plan
        return;
    }

    char planChar = dayPlan[index];

    // '0'–'3' karakterleri dışında bir şeyse varsayılana dön
    if (planChar < '0' || planChar > '3') {
        CurrentPlan = '0';
    } else {
        CurrentPlan = planChar;
    }
    printf( "%c\n", CurrentPlan);
    
    GetCurrentConfiguration();
}


void GetCurrentConfiguration(void) 
{
    if(CurrentPlan == '0') {
        CurrentConfiguration.isIdleActive      = s_defaultConfiguration.isIdleActive;
        strcpy(CurrentConfiguration.idleSound, s_defaultConfiguration.idleSound);
        CurrentConfiguration.idleMinVolume     = s_defaultConfiguration.idleMinVolume;
        CurrentConfiguration.idleMaxVolume     = s_defaultConfiguration.idleMaxVolume;
        CurrentConfiguration.idleContAfterReq  = s_defaultConfiguration.idleContAfterReq;
        CurrentConfiguration.isReqActive       = s_defaultConfiguration.isReqActive;
        strcpy(CurrentConfiguration.reqSound1, s_defaultConfiguration.reqSound1);
        strcpy(CurrentConfiguration.reqSound2, s_defaultConfiguration.reqSound2);
        CurrentConfiguration.reqPlayPeriod     = s_defaultConfiguration.reqPlayPeriod;
        CurrentConfiguration.reqMinVolume      = s_defaultConfiguration.reqMinVolume;
        CurrentConfiguration.reqMaxVolume      = s_defaultConfiguration.reqMaxVolume;
        CurrentConfiguration.isGreenActive     = s_defaultConfiguration.isGreenActive;
        strcpy(CurrentConfiguration.greenSound, s_defaultConfiguration.greenSound);
        CurrentConfiguration.greenMinVolume    = s_defaultConfiguration.greenMinVolume;
        CurrentConfiguration.greenMaxVolume    = s_defaultConfiguration.greenMaxVolume;
        CurrentConfiguration.greenCountFrom    = s_defaultConfiguration.greenCountFrom;
        CurrentConfiguration.greenCountTo      = s_defaultConfiguration.greenCountTo;
        strcpy(CurrentConfiguration.greenAction, s_defaultConfiguration.greenAction);
    }
    else if(CurrentPlan == '1') {
        CurrentConfiguration.isIdleActive      = s_alt1Configuration.isIdleActive;
        strcpy(CurrentConfiguration.idleSound, s_alt1Configuration.idleSound);
        CurrentConfiguration.idleMinVolume     = s_alt1Configuration.idleMinVolume;
        CurrentConfiguration.idleMaxVolume     = s_alt1Configuration.idleMaxVolume;
        CurrentConfiguration.idleContAfterReq  = s_alt1Configuration.idleContAfterReq;
        CurrentConfiguration.isReqActive       = s_alt1Configuration.isReqActive;
        strcpy(CurrentConfiguration.reqSound1, s_alt1Configuration.reqSound1);
        strcpy(CurrentConfiguration.reqSound2, s_alt1Configuration.reqSound2);
        CurrentConfiguration.reqPlayPeriod     = s_alt1Configuration.reqPlayPeriod;
        CurrentConfiguration.reqMinVolume      = s_alt1Configuration.reqMinVolume;
        CurrentConfiguration.reqMaxVolume      = s_alt1Configuration.reqMaxVolume;
        CurrentConfiguration.isGreenActive     = s_alt1Configuration.isGreenActive;
        strcpy(CurrentConfiguration.greenSound, s_alt1Configuration.greenSound);
        CurrentConfiguration.greenMinVolume    = s_alt1Configuration.greenMinVolume;
        CurrentConfiguration.greenMaxVolume    = s_alt1Configuration.greenMaxVolume;
        CurrentConfiguration.greenCountFrom    = s_alt1Configuration.greenCountFrom;
        CurrentConfiguration.greenCountTo      = s_alt1Configuration.greenCountTo;
        strcpy(CurrentConfiguration.greenAction, s_alt1Configuration.greenAction);
    }
    else if(CurrentPlan == '2') {
        CurrentConfiguration.isIdleActive      = s_alt2Configuration.isIdleActive;
        strcpy(CurrentConfiguration.idleSound, s_alt2Configuration.idleSound);
        CurrentConfiguration.idleMinVolume     = s_alt2Configuration.idleMinVolume;
        CurrentConfiguration.idleMaxVolume     = s_alt2Configuration.idleMaxVolume;
        CurrentConfiguration.idleContAfterReq  = s_alt2Configuration.idleContAfterReq;
        CurrentConfiguration.isReqActive       = s_alt2Configuration.isReqActive;
        strcpy(CurrentConfiguration.reqSound1, s_alt2Configuration.reqSound1);
        strcpy(CurrentConfiguration.reqSound2, s_alt2Configuration.reqSound2);
        CurrentConfiguration.reqPlayPeriod     = s_alt2Configuration.reqPlayPeriod;
        CurrentConfiguration.reqMinVolume      = s_alt2Configuration.reqMinVolume;
        CurrentConfiguration.reqMaxVolume      = s_alt2Configuration.reqMaxVolume;
        CurrentConfiguration.isGreenActive     = s_alt2Configuration.isGreenActive;
        strcpy(CurrentConfiguration.greenSound, s_alt2Configuration.greenSound);
        CurrentConfiguration.greenMinVolume    = s_alt2Configuration.greenMinVolume;
        CurrentConfiguration.greenMaxVolume    = s_alt2Configuration.greenMaxVolume;
        CurrentConfiguration.greenCountFrom    = s_alt2Configuration.greenCountFrom;
        CurrentConfiguration.greenCountTo      = s_alt2Configuration.greenCountTo;
        strcpy(CurrentConfiguration.greenAction, s_alt2Configuration.greenAction);
    }
    else if(CurrentPlan == '3') {
        CurrentConfiguration.isIdleActive      = s_alt3Configuration.isIdleActive;
        strcpy(CurrentConfiguration.idleSound, s_alt3Configuration.idleSound);
        CurrentConfiguration.idleMinVolume     = s_alt3Configuration.idleMinVolume;
        CurrentConfiguration.idleMaxVolume     = s_alt3Configuration.idleMaxVolume;
        CurrentConfiguration.idleContAfterReq  = s_alt3Configuration.idleContAfterReq;
        CurrentConfiguration.isReqActive       = s_alt3Configuration.isReqActive;
        strcpy(CurrentConfiguration.reqSound1, s_alt3Configuration.reqSound1);
        strcpy(CurrentConfiguration.reqSound2, s_alt3Configuration.reqSound2);
        CurrentConfiguration.reqPlayPeriod     = s_alt3Configuration.reqPlayPeriod;
        CurrentConfiguration.reqMinVolume      = s_alt3Configuration.reqMinVolume;
        CurrentConfiguration.reqMaxVolume      = s_alt3Configuration.reqMaxVolume;
        CurrentConfiguration.isGreenActive     = s_alt3Configuration.isGreenActive;
        strcpy(CurrentConfiguration.greenSound, s_alt3Configuration.greenSound);
        CurrentConfiguration.greenMinVolume    = s_alt3Configuration.greenMinVolume;
        CurrentConfiguration.greenMaxVolume    = s_alt3Configuration.greenMaxVolume;
        CurrentConfiguration.greenCountFrom    = s_alt3Configuration.greenCountFrom;
        CurrentConfiguration.greenCountTo      = s_alt3Configuration.greenCountTo;
        strcpy(CurrentConfiguration.greenAction, s_alt3Configuration.greenAction);
    }
}


const char* getMonthAbbreviation(uint8_t month)
{
    static const char* months[] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"
    };
    if (month >= 1 && month <= 12) {
        return months[month - 1];
    }
    return "unk"; // bilinmeyen
}
