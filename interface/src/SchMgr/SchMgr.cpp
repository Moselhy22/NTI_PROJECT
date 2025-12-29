/*
#include "SchMgr.h"
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../CFG.h"
#include "../SchMgr/SchMgr.h"
#include "../MQTTClient/MQTTClient.hpp"
#include "../App/App.hpp"

#if MQTT_CLIENT_MODULE
void MQTTClient_AppTask(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 100 / portTICK_PERIOD_MS; // 100 ms

    xLastWakeTime = xTaskGetTickCount();

    while (true)
    {
        MqttClient_Task();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
#endif // when MQTT_CLIENT_MODULE is enable

void App_Task_1sec(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 1000 / portTICK_PERIOD_MS; // 1000 ms

    xLastWakeTime = xTaskGetTickCount();
    while (true)
    {
#if APP_MODULE
        App_main_1sec();
#endif

#if ARDUINOCLOUD_MODULE
        ArduinoCloud_Task();
#endif
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
*/

#include "SchMgr.h"
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../CFG.h"
#include "../SchMgr/SchMgr.h"
#include "../MQTTClient/MQTTClient.hpp"
#include "../App/App.hpp"

#if MQTT_CLIENT_MODULE
void MQTTClient_AppTask(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 100 / portTICK_PERIOD_MS; // 100 ms

    xLastWakeTime = xTaskGetTickCount();

    while (true)
    {
        MqttClient_Task();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
#endif // when MQTT_CLIENT_MODULE is enable

void App_Task_1sec(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 1000 / portTICK_PERIOD_MS; // 1000 ms

    xLastWakeTime = xTaskGetTickCount();
    while (true)
    {
#if APP_MODULE
        App_main_1sec();
#endif

#if ARDUINOCLOUD_MODULE
        ArduinoCloud_Task();
#endif
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}