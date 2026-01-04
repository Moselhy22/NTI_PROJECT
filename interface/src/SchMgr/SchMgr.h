#ifndef SCHMGR_H
#define SCHMGR_H

#include "../CFG.h"

void App_Task_1min(void *pvParameters);
void App_Task_1sec(void *pvParameters);
void App_Task_30sec(void *pvParameters);

#if MQTT_CLIENT_MODULE
void MQTTClient_AppTask(void *pvParameters);
void MQTTClient_AppTask_Gard();
#endif // when MQTT_CLIENT_MODULE is enable

#endif // SCHMGR_H