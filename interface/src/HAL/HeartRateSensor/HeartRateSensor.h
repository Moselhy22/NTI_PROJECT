#ifndef HEART_RATE_SENSOR_H
#define HEART_RATE_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"   // مهم جدًا
#include "../HAL_Common.h"
void HeartRateSensor_Update(void);
void HeartRateSensor_Init(void);
long HeartRateSensor_ReadIR(void);

int  HeartRateSensor_ReadContactQuality(void);  // <-- keep contact quality
bool HeartRateSensor_IsInitialized(void);
float HeartRateSensor_ReadBPM(void);
float HeartRateSensor_ReadAvgBPM(void);

float HeartRateSensor_ReadSpO2(void); 

#endif
