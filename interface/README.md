NTI Health Monitoring System – Code Interface Guide

This repository contains the embedded firmware for the NTI Health Monitoring System, developed for ESP32 using FreeRTOS and on-device AI inference.

The project follows a modular interface-based design, where each module exposes a clear API and is isolated from implementation details.

1. Entry Point Interface
pro.ino

This is the system entry point.

Public Responsibilities:
void setup();
void loop();

Behavior:

Initializes Serial communication

Calls App_Init() to initialize all system modules

RTOS handles execution (loop remains empty)


2. Application Interface
src/App/App.hpp

This is the main system interface used by pro.ino.

Public API:
void App_Init(void);
void App_RunTasks(void);
void App_HandleMQTTCommand(const char *payload);
void App_main_1sec(void);

Function	            Responsibility
App_Init()	            Initialize sensors, AI model, WiFi, MQTT, and scheduler
App_RunTasks()	        Execute periodic application tasks
App_HandleMQTTCommand()	Handle cloud control messages



3. AI Health Model Interface
src/AI/HealthModel/HealthModel.h

This module exposes the AI inference interface.

Public API:
bool HealthModel_Init();
bool HealthModel_Run(float *input, float *output);

Description:

Initializes TensorFlow Lite Micro

Runs on-device inference

Accepts processed sensor features

Produces health classification output

Embedded Model:

Stored in model_data.h as a C array

Loaded at runtime


4. Sensor Interfaces (HAL Layer)

Each sensor has its own independent interface.

Example: Body Temperature Sensor
BodyTempSensor.h
bool BodyTempSensor_Init(void);
float BodyTempSensor_ReadCelsius(void);

Example: Heart Rate Sensor
HeartRateSensor.h
bool HeartRateSensor_Init(void);
uint16_t HeartRateSensor_ReadBPM(void);

Example: MPU6050 Sensor
MPU6050_Sensor.h
bool MPU6050_Init(void);
void MPU6050_Read(float *ax, float *ay, float *az);

Design Rule:

HAL modules do not contain application logic

Each HAL module exposes:

Init()

Read() functions only


5. HAL Common Interface
src/HAL/HAL_Common.h

Provides shared utilities used by all HAL modules:

Delay handling

Common error codes

Hardware helpers


6. MCAL Interface
src/MCAL/I2C_Driver/I2C_Driver.h

Low-level hardware interface.

Public API:
bool I2C_Init(void);
bool I2C_Read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
bool I2C_Write(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

Description:

Direct hardware communication

Used internally by HAL only

Never accessed by App layer directly


7. MQTT Client Interface
src/MQTTClient/MqttClient.hpp

Handles all cloud communication.

Public API:
bool MqttClient_Init(void);
bool MqttClient_PublishMessage(const char *topic, double value);
const char* MqttClient_GetLastMessage(void);

Responsibilities:

WiFi connection

MQTT broker connection

Publishing sensor and AI data

Receiving control commands


8. Scheduler Interface
src/SchMgr/SchMgr.h

Defines RTOS task timing.

Example:
#define TASK_100MS_INTERVAL   100
#define TASK_1SEC_INTERVAL   1000
#define TASK_5SEC_INTERVAL   5000


Used by the App layer to schedule periodic execution safely.



9. Configuration Interface
src/CFG.h

Central build-time configuration file.

Controls:

Module enable / disable

Feature flags

System-wide macros

Example:

#define APP_MODULE ENABLE
#define MQTT_CLIENT_MODULE ENABLE



10. Interface Design Principles

Each module exposes a clear public API

No cross-layer dependency violations

HAL never calls App

App never touches MCAL

AI model is fully isolated

RTOS controls execution flow



Summary

This project is designed as an interface-driven embedded system, making it:

Easy to maintain

Easy to scale

Safe for real-time execution

Suitable for medical and IoT applications





