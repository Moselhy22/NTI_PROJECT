#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <Arduino.h>

void GasSensor_Init(uint8_t pin, int threshold = 3000);
int  GasSensor_Read(void);

#endif
