#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

/* ================= GPS Pins ================= */
#define GPS_RX 16
#define GPS_TX 17

/* ================= Public API ================= */
void GPS_Init(void);
void GPS_Update(void);

bool GPS_IsLocationUpdated(void);

double GPS_GetLatitude(void);
double GPS_GetLongitude(void);
uint32_t GPS_GetSatellites(void);
uint32_t GPS_GetHDOP(void);

uint8_t GPS_GetHour(void);
uint8_t GPS_GetMinute(void);
uint8_t GPS_GetSecond(void);

#endif // GPS_SENSOR_H
