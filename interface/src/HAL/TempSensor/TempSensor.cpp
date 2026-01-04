#include "TempSensor.h"
#include "DHT.h"

#define DHT_PIN   4
#define DHT_TYPE  DHT11

static DHT dht(DHT_PIN, DHT_TYPE);

void TempSensor_Init(void){ dht.begin(); }

bool TempSensor_Read(float &temperature, float &humidity)
{
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    if (isnan(temperature) || isnan(humidity)) return false;
    return true;
}
