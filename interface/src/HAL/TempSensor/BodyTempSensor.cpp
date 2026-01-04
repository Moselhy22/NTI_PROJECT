// BodyTempSensor.cpp - MINIMAL WORKING VERSION
#include "BodyTempSensor.h"
#include <Wire.h>

static float temperatureC = 0.0f;

bool BodyTempSensorInit(void)
{
    Serial.println("Initializing MAX30105 Temperature Sensor...");
    
    // Initialize sensor
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
    {
        Serial.println("MAX30105 not found!");
        return false;
    }
    
    particleSensor.setup();
    particleSensor.enableDIETEMPRDY();
    
    Serial.println("✅ Temperature Sensor Ready");
    return true;
}

void BodyTempSensor_Update(void)
{
    // Read temperature from MAX30105
    temperatureC = particleSensor.readTemperature();
}

float BodyTempSensor_ReadC()
{
    return temperatureC;
}

float BodyTempSensor_ReadF()
{
    return temperatureC * 9.0/5.0 + 32.0;
}