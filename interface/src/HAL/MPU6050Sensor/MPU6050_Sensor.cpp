#include "MPU6050_Sensor.h"
#include "../../MCAL/I2C_Driver/I2C_Driver.h"
#include <MPU6050_light.h>
#include <Wire.h>
#include <math.h>

static MPU6050 mpu(Wire);
static bool mpuInitialized = false;

bool MPU6050_Init(void)
{
    I2C_Begin(21, 22, 100000);
    Serial.println("Initializing MPU6050...");

    if (mpu.begin() != 0)
    {
        Serial.println("❌ MPU6050 NOT FOUND");
        mpuInitialized = false;
        return false;
    }

    Serial.println("✅ MPU6050 FOUND");
    mpu.calcOffsets();
    mpuInitialized = true;
    return true;
}

void MPU6050_Update(void)
{
    if (!mpuInitialized) return;
    mpu.update();
}

bool MPU6050_DetectFall(void)
{
    if (!mpuInitialized) return false;

    MPU6050_Update();

    float ax = mpu.getAccX();
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();

    float totalAcc = sqrt(ax * ax + ay * ay + az * az);
    return (totalAcc < 0.5f);
}
float MPU6050_GetMotionMagnitude(void)
{
    float ax = MPU6050_GetAccX();
    float ay = MPU6050_GetAccY();
    float az = MPU6050_GetAccZ();
    return sqrt(ax*ax + ay*ay + az*az);
}

// ===== Getters =====
float MPU6050_GetAccX(void){ return mpuInitialized ? mpu.getAccX() : 0.0f; }
float MPU6050_GetAccY(void){ return mpuInitialized ? mpu.getAccY() : 0.0f; }
float MPU6050_GetAccZ(void){ return mpuInitialized ? mpu.getAccZ() : 0.0f; }

float MPU6050_GetGyroX(void){ return mpuInitialized ? mpu.getGyroX() : 0.0f; }
float MPU6050_GetGyroY(void){ return mpuInitialized ? mpu.getGyroY() : 0.0f; }
float MPU6050_GetGyroZ(void){ return mpuInitialized ? mpu.getGyroZ() : 0.0f; }
