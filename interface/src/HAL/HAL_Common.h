#ifndef HAL_COMMON_H
#define HAL_COMMON_H
#include "MAX30105.h"
#include <Arduino.h>

// Sensor status types
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERROR,
    SENSOR_NOT_CONNECTED,
    SENSOR_CALIBRATING
} Sensor_Status_t;

// Common sensor data structure
typedef struct {
    float value;
    uint32_t timestamp;
    Sensor_Status_t status;
} Sensor_Data_t;

extern MAX30105 particleSensor; // single global object

#endif