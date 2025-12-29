#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Arduino.h>

void TempSensor_Init(void);
bool TempSensor_Read(float &temperature, float &humidity);

#endif
