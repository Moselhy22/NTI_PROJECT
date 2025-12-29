#include "MPU6050_Sensor.h"
#include "../../MCAL/I2C_Driver/I2C_Driver.h"
#include <MPU6050_light.h>
#include <Wire.h>
#include <math.h>

static MPU6050 mpu(Wire);
static bool mpuInitialized = false;

void MPU6050_Init(void)
{
    I2C_Begin(21, 22, 100000);
    Serial.println("Initializing MPU6050...");

    if (mpu.begin() != 0)
    {
        Serial.println("❌ MPU6050 NOT FOUND");
        mpuInitialized = false;
        return;
    }

    Serial.println("✅ MPU6050 FOUND");
    mpu.calcOffsets();
    mpuInitialized = true;
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
    return totalAcc < 0.5; // example threshold
}

// ===== Getters =====
float MPU6050_GetAccX(void) { return mpu.getAccX(); }
float MPU6050_GetAccY(void) { return mpu.getAccY(); }
float MPU6050_GetAccZ(void) { return mpu.getAccZ(); }

float MPU6050_GetGyroX(void) { return mpu.getGyroX(); }
float MPU6050_GetGyroY(void) { return mpu.getGyroY(); }
float MPU6050_GetGyroZ(void) { return mpu.getGyroZ(); }
