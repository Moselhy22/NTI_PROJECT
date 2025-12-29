#ifndef HEART_RATE_SENSOR_H
#define HEART_RATE_SENSOR_H

#include <Arduino.h>
#include "MAX30105.h"

void HeartRateSensor_Init(void);
void HeartRateSensor_Update(void);
long HeartRateSensor_ReadIR(void);
int  HeartRateSensor_ReadContactQuality(void);
float HeartRateSensor_ReadSpO2(void); // optional placeholder
bool HeartRateSensor_IsInitialized(void);

#endif

