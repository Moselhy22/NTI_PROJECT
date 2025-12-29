#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H

#include "../HAL_Common.h"
#include <TinyGPSPlus.h>

class GPSSensor {
private:
    TinyGPSPlus gps;
    bool initialized;
    HardwareSerial* gpsSerial;
    unsigned long lastValidFix;
    
public:
    GPSSensor();
    
    bool init(uint8_t uart_num = 2, uint32_t baud_rate = 9600, uint8_t rx_pin = 16, uint8_t tx_pin = 17);
    bool read(float* latitude, float* longitude);
    bool isFixed(void);
    int getSatellites(void);
    float getSpeed(void);
    float getAltitude(void);
    Sensor_Status_t getStatus(void);
    
    void update(void);
    
private:
    Sensor_Status_t status;
};

// C-style interface
#ifdef __cplusplus
extern "C" {
#endif

void GPS_Init(void);
bool GPS_Read(float* latitude, float* longitude);
Sensor_Status_t GPS_GetStatus(void);
int GPS_GetSatellites(void);

#ifdef __cplusplus
}
#endif

#endif