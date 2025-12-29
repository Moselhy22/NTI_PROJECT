/*
#include "I2C_Driver.h"

bool I2C_Driver::initialized = false;

bool I2C_Driver::begin(uint32_t frequency) {
    if (!initialized) {
        Wire.begin();
        Wire.setClock(frequency);
        initialized = true;
        Serial.println("I2C Initialized");
    }
    return true;
}

bool I2C_Driver::writeByte(uint8_t address, uint8_t reg, uint8_t data) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(data);
    return (Wire.endTransmission() == 0);
}

bool I2C_Driver::writeBytes(uint8_t address, uint8_t reg, uint8_t* data, uint8_t length) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    for (uint8_t i = 0; i < length; i++) {
        Wire.write(data[i]);
    }
    return (Wire.endTransmission() == 0);
}

uint8_t I2C_Driver::readByte(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(address, (uint8_t)1);
    return Wire.read();
}

bool I2C_Driver::readBytes(uint8_t address, uint8_t reg, uint8_t* buffer, uint8_t length) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    
    Wire.requestFrom(address, length);
    uint8_t i = 0;
    while (Wire.available() && i < length) {
        buffer[i++] = Wire.read();
    }
    return (i == length);
}

bool I2C_Driver::isDeviceConnected(uint8_t address) {
    Wire.beginTransmission(address);
    return (Wire.endTransmission() == 0);
}
    */
#include "I2C_Driver.h"

void I2C_Begin(uint8_t sda, uint8_t scl, uint32_t freq)
{
    Wire.begin(sda, scl);
    Wire.setClock(freq);
}
