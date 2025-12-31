/*
#ifndef APP_H
#define APP_H

#include <Arduino.h>

#define MQTT_CLIENT_ID  "HealthMonitor_01"
#define NODE_ID         "HealthMonitor_01"
#define SUBSCRIBE_TOPIC_NODE_ID "SubHealthMonitor_01"

// LED pins for different status
#define STATUS_LED_PIN  23
#define ALERT_LED_PIN   22
#define HEARTBEAT_LED   2   // Built-in LED

// Task priorities
#define TASK_PRIORITY_HIGH   3
#define TASK_PRIORITY_MEDIUM 2
#define TASK_PRIORITY_LOW    1

typedef enum {
    STATE_SAFE = 0,
    STATE_WARNING,
    STATE_DANGER,
    STATE_CRITICAL
} SafetyState_t;

// Health data structure
typedef struct {
    float temperature;
    float heartRate;
    float spO2;
    int gasPPM;
    bool fallDetected;
    float latitude;
    float longitude;
    SafetyState_t overallState;
    uint32_t timestamp;
} HealthData_t;

// Task function prototypes
void App_Init(void);
void App_Task_100ms(void *pvParameters);     // Fast sensors (HR, SpO2)
void App_Task_1sec(void *pvParameters);      // Normal sensors (Temp, Gas)
void App_Task_5sec(void *pvParameters);      // GPS and state evaluation
void App_Task_Telemetry(void *pvParameters); // MQTT Publishing

void App_handleNodeMQTTCommandRecived(const char *payload);
SafetyState_t App_EvaluateOverallState(HealthData_t *data);
void App_PublishHealthData(HealthData_t *data);

extern HealthData_t currentHealthData;

#endif
*/
#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include "../HAL/TempSensor/TempSensor.h"
#include "../HAL/GasSensor/GasSensor.h"
#include "../HAL/MPU6050Sensor/MPU6050_Sensor.h"
#include "../HAL/GPSSensor/GPS_Sensor.h"
#include "../HAL/HeartRateSensor/HeartRateSensor.h"
#include "../MQTTClient/MqttClient.hpp"
#include "../MCAL/I2C_Driver/I2C_Driver.h"
#include <WiFi.h>
// Node info
#define MQTT_CLIENT_ID    "HealthMonitor_01"
#define NODE_ID           "HealthMonitor_01"
#define SUBSCRIBE_TOPIC_NODE_ID "SubHealthMonitor_01"



#define LED_PIN 23


// Task intervals (ms)
#define TASK_100MS_INTERVAL 100
#define TASK_1SEC_INTERVAL 1000
#define TASK_5SEC_INTERVAL 5000

// App functions
void App_Init(void);                               // Initialize sensors and peripherals
void App_RunTasks(void);                           // Run all scheduled tasks
void App_HandleMQTTCommand(const char *payload);  // Handle incoming MQTT commands
void App_main_1sec();
#endif
