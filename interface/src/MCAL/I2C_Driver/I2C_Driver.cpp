#include "I2C_Driver.h"

void I2C_Begin(uint8_t sda, uint8_t scl, uint32_t freq)
{
    Wire.begin(sda, scl);
    Wire.setClock(freq);
}
