#include "GasSensor.h"

static uint8_t gsPin;
static int gsThreshold;
static int gsValue;

void GasSensor_Init(uint8_t pin, int threshold)
{
    gsPin = pin;
    gsThreshold = threshold;
    gsValue = 0;
    pinMode(gsPin, INPUT);
    Serial.begin(115200);
    delay(1000);
    Serial.println("MQ-5 Gas Sensor Test");
    Serial.println("Preheating sensor...");
    delay(30000);
}

int GasSensor_Read(void)
{
    gsValue = analogRead(gsPin);
    Serial.print("MQ-5 Analog Value: "); Serial.println(gsValue);
    return gsValue;
}
