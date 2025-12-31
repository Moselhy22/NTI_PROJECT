#ifndef _MQTTCLIENT_HPP
#define _MQTTCLIENT_HPP

#include "../CFG.h"

#if MQTT_CLIENT_MODULE



/* =================== Network Wi-Fi Credentials ===================*/ 
#define WIFI_SSID  "REDMI Turbo 4 Pro"  
#define WIFI_PASSWORD  "1234567899"
/*===================== MQTT Broker Credentials =====================*/ 
#define MQTT_SERVER_IP  "10.213.81.89" // the ip of the device running the broker   
#define MQTT_PORT  1883              
#define MQTT_USER_NAME  "Abdelrahman-Sayed"
#define MQTT_PASSWORD  "17221079"
// FreeRTOS Queue for MQTT Messages
#define QUEUE_SIZE 50
#define MQTT_PUBLISH_VALUE_WITH_JSON ENABLE

// Struct to hold queued MQTT messages
typedef struct {
    char topic[50];
    char payload[100];
}mqttMessage_t;

#define NO_DELAY  0

void MqttClient_Init(void);
void MqttClient_Task(void);


bool MqttClient_PublishMessage(const char *topic, const char *value);
bool MqttClient_PublishMessage(const char *topic, float value);
bool MqttClient_PublishMessage(const char *topic, bool value);
bool MqttClient_PublishMessage(const char *topic, uint8_t value);
bool MqttClient_PublishMessage(const char *topic, uint16_t value);
bool MqttClient_PublishMessage(const char *topic, uint32_t value);
bool MqttClient_PublishMessage(const char *topic, const String &value);
bool MqttClient_PublishMessage(const char *topic, char value);
bool MqttClient_PublishMessage(const char *topic, int value);
bool MqttClient_PublishMessage(const char *topic, double value);

// MqttClient_Callback function for received messages
void MqttClient_Callback(char *topic, byte *payload, unsigned int length);
// disconnect from the broker
void MqttClient_Disconnect(void);


#endif // MQTT_CLIENT_MODULE
#endif // _MQTTCLIENT_HPP