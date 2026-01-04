#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <Wire.h>

void I2C_Begin(uint8_t sda, uint8_t scl, uint32_t freq = 100000);

#endif

