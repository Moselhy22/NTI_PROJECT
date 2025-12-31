#include "HeartRateSensor.h"
#include "../../MCAL/I2C_Driver/I2C_Driver.h"
#include <Wire.h>
#include "MAX30105.h"

// internal state
static MAX30105 particleSensor;
static bool hrInitialized = false;
static long irValue = 0;
static int contactQuality = 0;

void HeartRateSensor_Init(void)
{
    I2C_Begin(21, 22, 100000);

    Serial.println("Initializing MAX30102...");
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD))
    {
        Serial.println("❌ MAX30102 NOT FOUND");
        hrInitialized = false;
        return;
    }
    particleSensor.setup(); // default settings
    Serial.println("✅ MAX30102 FOUND");
    hrInitialized = true;
}

void HeartRateSensor_Update(void)
{
    if (!hrInitialized) return;

    irValue = particleSensor.getIR();
    long minVal = 0, maxVal = 120000;
    contactQuality = map(irValue, minVal, maxVal, 0, 100);
    if(contactQuality>100) contactQuality=100;
    if(contactQuality<0) contactQuality=0;
}

long HeartRateSensor_ReadIR(void){ return irValue; }
int  HeartRateSensor_ReadContactQuality(void){ return contactQuality; }
float HeartRateSensor_ReadSpO2(void){ return 0; } // placeholder
bool HeartRateSensor_IsInitialized(void){ return hrInitialized; }
