#include "GPS_Sensor.h"
#include <Arduino.h>

/* ================= Objects ================= */
static TinyGPSPlus gps;
static HardwareSerial gpsSerial(2);

/* ================= Init ================= */
void GPS_Init(void)
{
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
    Serial.println("Waiting for GPS signal...");
    
}

/* ================= Update ================= */
void GPS_Update(void)
{
    while (gpsSerial.available())
    {
        gps.encode(gpsSerial.read());
    }
}

/* ================= Data Access ================= */
bool GPS_IsLocationUpdated(void)
{
    return gps.location.isValid();
}


double GPS_GetLatitude(void)
{
    return gps.location.lat();
}

double GPS_GetLongitude(void)
{
    return gps.location.lng();
}

uint32_t GPS_GetSatellites(void)
{
    return gps.satellites.value();
}

uint32_t GPS_GetHDOP(void)
{
    return gps.hdop.value();
}

uint8_t GPS_GetHour(void)
{
    return gps.time.hour();
}

uint8_t GPS_GetMinute(void)
{
    return gps.time.minute();
}

uint8_t GPS_GetSecond(void)
{
    return gps.time.second();
}
