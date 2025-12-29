/*
#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "../../CFG.h"
#include "../MCAL_Common.h"

class I2C_Driver {
public:
    static bool begin(uint32_t frequency = MCAL_I2C_CLOCK_SPEED);
    static bool writeByte(uint8_t address, uint8_t reg, uint8_t data);
    static bool writeBytes(uint8_t address, uint8_t reg, uint8_t* data, uint8_t length);
    static uint8_t readByte(uint8_t address, uint8_t reg);
    static bool readBytes(uint8_t address, uint8_t reg, uint8_t* buffer, uint8_t length);
    static bool isDeviceConnected(uint8_t address);
    
private:
    static bool initialized;
};

#endif
*/
#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <Wire.h>

void I2C_Begin(uint8_t sda, uint8_t scl, uint32_t freq = 100000);

#endif

