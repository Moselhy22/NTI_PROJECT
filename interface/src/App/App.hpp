#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include "../HAL/TempSensor/BodyTempSensor.h"

#include "../HAL/GasSensor/GasSensor.h"
#include "../HAL/MPU6050Sensor/MPU6050_Sensor.h"
#include "../HAL/GPSSensor/GPS_Sensor.h"
#include "../HAL/HeartRateSensor/HeartRateSensor.h"
#include "../MQTTClient/MqttClient.hpp"
#include "../MCAL/I2C_Driver/I2C_Driver.h"
#include <WiFi.h>
// Node info
//#define MQTT_CLIENT_ID    "HealthMonitor_01"
//#define NODE_ID           "HealthMonitor_01"
//#define SUBSCRIBE_TOPIC_NODE_ID "SubHealthMonitor_01"



/**test** */
#define MQTT_CLIENT_ID    "sensors/data"
#define NODE_ID           "sensors/data"
#define SUBSCRIBE_TOPIC_NODE_ID "sensors/data"





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
