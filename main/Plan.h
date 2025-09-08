/*
 * Plan.h
 *
 *  Created on: 30 Nis 2025
 *      Author: metesepetcioglu
 */

#ifndef MAIN_PLAN_H_
#define MAIN_PLAN_H_

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "SystemTime.h"

#define maxPlan 4
#define maxDays 7
#define maxSpecialDays 10 // Kaç özel gün tanımlanabilir?
#define SlotsPerDay 96  // 24 saat * 4 (15 dk'lık dilimler)
#define voiceFile 100

/////////////////////////////////////////////////////////////////////////////////////////
/***************************************STRUCTURES***************************************/


struct xCurrentConfiguration {
  bool isIdleActive;
  char idleSound[50];
  int idleMinVolume;
  int idleMaxVolume;
  bool idleContAfterReq;
  bool isReqActive;
  char reqSound1[50];
  char reqSound2[50];
  int reqPlayPeriod;
  int reqMinVolume;
  int reqMaxVolume;
  bool isGreenActive;
  char greenSound[50];
  int greenMinVolume;
  int greenMaxVolume;
  int greenCountFrom;
  int greenCountTo;
  char greenAction[50];
};

void CheckPlanConfig(void);
void GetCurrentConfiguration(void) ;
const char* getMonthAbbreviation(uint8_t month);
void GetCurrentPlan(void);

//uint8_t get_active_plan_index(xConfig* config, rtc_time_t* DeviceTime);
#endif /* MAIN_PLAN_H_ */
