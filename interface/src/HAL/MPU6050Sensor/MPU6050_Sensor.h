#ifndef MPU6050_SENSOR_H
#define MPU6050_SENSOR_H

#include <Arduino.h>

void MPU6050_Init(void);
void MPU6050_Update(void);
bool MPU6050_DetectFall(void);

// New getters for App.cpp
float MPU6050_GetAccX(void);
float MPU6050_GetAccY(void);
float MPU6050_GetAccZ(void);
float MPU6050_GetGyroX(void);
float MPU6050_GetGyroY(void);
float MPU6050_GetGyroZ(void);

#endif
