/*
#ifndef HEALTH_MODEL_H
#define HEALTH_MODEL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float bpm;
    float temp;
    float motion;
    bool  fall;
} HealthModelInput_t;

typedef struct
{
    float score;
    const char* state;
} HealthModelOutput_t;

/* API 
void HealthModel_Init(void);
void HealthModel_Run(const HealthModelInput_t* in,
                     HealthModelOutput_t* out);

#ifdef __cplusplus
}
#endif

#endif
*/

#ifndef HEALTH_MODEL_H
#define HEALTH_MODEL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sensor sample structure
typedef struct {
    float hr;      // Heart Rate (BPM)
    float temp;    // Temperature (°C)
    float accX;    // Accelerometer X
    float accY;    // Accelerometer Y
    float accZ;    // Accelerometer Z
} SensorSample_t;

// Model input (128 samples buffer)
typedef struct {
    SensorSample_t samples[128];  // Buffer of 128 time samples
    int sample_count;             // Current samples collected
    bool buffer_ready;            // True when 128 samples collected
} HealthModelInput_t;

// Model output
typedef struct {
    float score;            // Raw model output (0.0-1.0)
    const char* state;      // "NORMAL", "ABNORMAL", "SEIZURE", "REST"
    bool alert;             // True if emergency alert needed
} HealthModelOutput_t;

/* API */
void HealthModel_Init(void);
void HealthModel_AddSample(float hr, float temp, float accX, float accY, float accZ);
bool HealthModel_IsBufferReady(void);
void HealthModel_Run(HealthModelOutput_t* out);
void HealthModel_ResetBuffer(void);

#ifdef __cplusplus
}
#endif

#endif