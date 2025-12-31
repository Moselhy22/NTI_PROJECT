#define ENABLE  1
#define DISABLE 0

#define APP_MODULE           ENABLE
#define MQTT_CLIENT_MODULE   ENABLE
#define ARDUINOCLOUD_MODULE  DISABLE

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "src/CFG.h"
#include "src/App/App.hpp"
#include "src/MQTTClient/MqttClient.hpp"
#include "src/SchMgr/SchMgr.h"

// ===== Setup =====
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("NTI Health Monitoring Project Starting...");

    // Initialize all sensors and peripherals
    App_Init();

    // Optionally, connect to MQTT broker here
    // MqttClient_Init();
}

// ===== Main Loop =====
void loop() {
    // Run all scheduled tasks (multi-task scheduler)
    App_RunTasks();

    // Optional: handle incoming MQTT messages
    // const char* payload = MqttClient_GetLastMessage();
    // if (payload) App_HandleMQTTCommand(payload);
}