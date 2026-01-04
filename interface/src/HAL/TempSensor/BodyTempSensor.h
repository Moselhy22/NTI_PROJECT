#ifndef BODY_TEMP_SENSOR_H
#define BODY_TEMP_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include <stdint.h>
#include "../HAL_Common.h"
// Initialize MAX30105 object (only once)


// Init & Update functions
bool BodyTempSensorInit();
void BodyTempSensor_Update(); // Reads and updates internal variables

// Getter functions
float BodyTempSensor_ReadC(); // Returns temperature in Celsius
float BodyTempSensor_ReadF(); // Returns temperature in Fahrenheit

#endif
