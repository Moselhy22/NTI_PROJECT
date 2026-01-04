#ifndef MCAL_COMMON_H
#define MCAL_COMMON_H

#include <Arduino.h>
#include <Wire.h>

// Common MCAL types and macros
typedef enum {
    MCAL_OK = 0,
    MCAL_ERROR,
    MCAL_BUSY,
    MCAL_TIMEOUT
} MCAL_Status_t;

// I2C Configuration
#define MCAL_I2C_CLOCK_SPEED 400000  // 400kHz
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// Initialize all MCAL peripherals
void MCAL_Init(void);

#endif