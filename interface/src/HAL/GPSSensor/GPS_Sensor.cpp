#include "GPS_Sensor.h"
#include "../../MCAL/UART_Driver/UART_Driver.h"

GPSSensor gpsSensor;

GPSSensor::GPSSensor() : initialized(false), gpsSerial(nullptr), lastValidFix(0), status(SENSOR_NOT_CONNECTED) {}

bool GPSSensor::init(uint8_t uart_num, uint32_t baud_rate, uint8_t rx_pin, uint8_t tx_pin) {
    // Initialize UART for GPS
    if (!UART_Driver::begin(uart_num, baud_rate, rx_pin, tx_pin)) {
        status = SENSOR_ERROR;
        return false;
    }
    
    initialized = true;
    status = SENSOR_OK;
    Serial.println("GPS Sensor Initialized");
    return true;
}

void GPSSensor::update(void) {
    if (!initialized) return;
    
    // Read from GPS UART (UART2)
    while (UART_Driver::available(2)) {
        char c = UART_Driver::read(2);
        gps.encode(c);
    }
    
    if (gps.location.isValid()) {
        lastValidFix = millis();
    }
}

bool GPSSensor::read(float* latitude, float* longitude) {
    update();
    
    if (gps.location.isValid()) {
        *latitude = gps.location.lat();
        *longitude = gps.location.lng();
        return true;
    }
    
    // Return last known position if recent
    if (gps.location.isValid() && (millis() - lastValidFix < 30000)) { // 30 seconds
        *latitude = gps.location.lat();
        *longitude = gps.location.lng();
        return true;
    }
    
    return false;
}

bool GPSSensor::isFixed(void) {
    update();
    return gps.location.isValid();
}

int GPSSensor::getSatellites(void) {
    update();
    return gps.satellites.value();
}

float GPSSensor::getSpeed(void) {
    update();
    return gps.speed.kmph();
}

float GPSSensor::getAltitude(void) {
    update();
    return gps.altitude.meters();
}

Sensor_Status_t GPSSensor::getStatus(void) {
    update();
    
    if (!initialized) return SENSOR_NOT_CONNECTED;
    
    if (gps.location.isValid()) {
        return SENSOR_OK;
    } else if (gps.satellites.value() > 0) {
        return SENSOR_CALIBRATING;
    } else {
        return SENSOR_ERROR;
    }
}

// C-style interface
void GPS_Init(void) {
    gpsSensor.init();
}

bool GPS_Read(float* latitude, float* longitude) {
    return gpsSensor.read(latitude, longitude);
}

Sensor_Status_t GPS_GetStatus(void) {
    return gpsSensor.getStatus();
}

int GPS_GetSatellites(void) {
    return gpsSensor.getSatellites();
}