#include "../CFG.h"

#if MQTT_CLIENT_MODULE

#include <WiFi.h>
#include <PubSubClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "MqttClient.hpp"
#include <ArduinoJson.h>
#include "../App/App.hpp"

extern void App_handleNodeMQTTCommandRecived(const char *payload);

WiFiClient espClient;
PubSubClient client(espClient);

// FreeRTOS Queue for MQTT Messages
QueueHandle_t mqttQueue;

static bool enqueueMessage(const char *topic, const char *payload);
static void MqttClient_ConnectToBroker(void);
static void connectToNetwork_Wifi(void);

void MqttClient_Init(void)
{
    /*===================== Initialize WiFi (Connecting to the Network) =====================*/
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to WiFi...");
    delay(1500);
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("XXXXXXXXXXXXXXX WiFi Failed to Connect >>> Retrying in the Task XXXXXXXXXXXXXXX");
    }

    /* Get and print the MAC address */
    String macAddress = WiFi.macAddress();
    Serial.println("ESP32 MAC Address (Wi-Fi): " + macAddress);

    /*========================= Initialize MQTT =========================*/
    client.setServer(MQTT_SERVER_IP, MQTT_PORT);

    /* KeepAlive timer -->>
    1️⃣ On the CLIENT side : If NO activity for 60 seconds, client sends PINGREQ.If broker does NOT reply with PINGRESP within timeout:
        - Client declares disconnect - client.connected() becomes false - You must reconnect manually. 
    2️⃣ On the BROKER side :: If no data or no PINGREQ arrives within the keep-alive × 1.5 (usually)
        - Broker disconnects the client.
    */
    client.setKeepAlive(60);

    /*Whenever the broker sends me a message on a topic I subscribed to, call the function MqttClient_Callback and pass the message data to it*/
    client.setCallback(MqttClient_Callback);

    // Connect to MQTT Broker
    bool mqttConnected = client.connect(MQTT_CLIENT_ID, MQTT_USER_NAME, MQTT_PASSWORD);
    delay(1000); // Wait for a second to ensure connection is established
    Serial.print("Connecting to MQTT...");
    if (mqttConnected)
    {
        Serial.println(" Connected!");
        /*here you can subscribe to any topics*/
        String subTopic = String(SUBSCRIBE_TOPIC_NODE_ID) + "/#"; // "/#" mean any thing will start with  " SUBSCRIBE_TOPIC_NODE_ID/ "
        client.subscribe(subTopic.c_str());
        delay(500); // Wait for a 500ms to ensure subscription is established
        Serial.print("Subscribed to topic: ");
        Serial.println(subTopic);
        
    }
    else
    {
        Serial.print(" Failed, rc=");
        Serial.print(client.state());
        Serial.println("Will Retrying in the Task");
    }

    // Create MQTT Queue
    mqttQueue = xQueueCreate(QUEUE_SIZE, sizeof(mqttMessage_t));
    Serial.printf("MQTT Queue Size: %d bytes\n", QUEUE_SIZE * sizeof(mqttMessage_t));
}

void MqttClient_Task(void)
{
    mqttMessage_t message;
    // make sure you are connected to network
    if (WiFi.status() == WL_CONNECTED)
    {
        // Ensure MQTT is connected before attempting to send
        if (!client.connected())
        {
            Serial.println("MQTT Disconnected, reconnecting...");
            MqttClient_ConnectToBroker(); // try to connect if failed try after 500 ms again.
        }
        else
        {
            /*Call client.loop() is required to 
                -Receive incoming MQTT messages -MQTT KeepAlive (PINGREQ / PINGRESP) -Detect broken or dropped connections
            */ 
            client.loop();

            /*get a message from mqttQueue and publish it 
              0 --> I will not want to wait any time to receive the message. if not return so No message in queue right now
            */
            if (xQueueReceive(mqttQueue, &message, 0) == pdPASS)
            {
                // Try to publish the message
                if (!client.publish(message.topic, message.payload))
                {
                    Serial.printf("Failed to publish to %s: %s. Retrying...\n", message.topic, message.payload);
                    // Places the message at the front of the queue → It will be processed first in the next loop.
                    xQueueSendToFront(mqttQueue, &message, NO_DELAY);
                }

            }
            else
            {
                // Serial.println("the Queue is empty (No message in queue right now)");
            }
        }
    }
    else
    {
        Serial.println("WiFi Disconnected, reconnecting...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 seconds delay before try to connect to network again
    }
}

// Connect to MQTT Broker
static void MqttClient_ConnectToBroker(void)
{
    Serial.print("Connecting to MQTT Broker...");
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER_NAME, MQTT_PASSWORD))
    {
        Serial.println("MqttClient_ConnectToBroker Successfully!");
        client.setKeepAlive(60);
        /*subscribe to  topics*/
        String subTopic = String(SUBSCRIBE_TOPIC_NODE_ID) + "/#";
        client.subscribe(subTopic.c_str());
        Serial.print("Subscribed to topic: ");
        Serial.println(subTopic);
    }
    else
    {
        Serial.print(" Failed, rc=");
        Serial.print(client.state());
        Serial.println(" Retrying in 0.5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(500)); // Wait before retrying
    }
}

// Function to add messages to the queue
static bool enqueueMessage(const char *topic, const char *payload)
{
    mqttMessage_t msg;
    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
    /*Places the message at the end of the queue → It will be processed after all other messages..
    pdMS_TO_TICKS(500) waiting up to 500ms for space to become available >> If the queue has space, the message is added immediately,.*/
    if (xQueueSend(mqttQueue, &msg, NO_DELAY) == pdPASS)
    {
        return true;
    }
    else
    {
        Serial.println("Queue Full! Message Dropped.");
        return false;
    }
}

// MqttClient_Callback function for received messages
void MqttClient_Callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("----->> Message received [");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (strstr(topic, SUBSCRIBE_TOPIC_NODE_ID) == topic)  // topic starts with "Node_1"
    {
        // Copy payload to a null-terminated string
        char msg[128] = {0};
        if (length >= sizeof(msg))
            length = sizeof(msg) - 1;
        memcpy(msg, payload, length);
        msg[length] = '\0';

        /*instade of this i can make queue for the requests recived from SUBSCRIBE_TOPIC and let the APP layer to process it as needed*/
        Serial.print("msg payload is sent to App_handleNodeMQTTCommandRecived :: ");
        Serial.println(msg);
        App_handleNodeMQTTCommandRecived(msg);
    }
}

// disconnect from the broker
void MqttClient_Disconnect(void)
{
    client.disconnect();
    Serial.println("MQTT Disconnected. Now idle.");
}

// MqttClient_PublishMessage
/* payload will be sent >>> Ex: {"value": 100} */
bool MqttClient_PublishMessage(const char *topic, int value)
{
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    char buffer[64];
    serializeJson(doc, buffer);
    return enqueueMessage(topic, buffer);
}

bool MqttClient_PublishMessage(const char *topic, float value)
{
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    char buffer[64];
    serializeJson(doc, buffer);
    return enqueueMessage(topic, buffer);
}

bool MqttClient_PublishMessage(const char *topic, bool value)
{
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    char buffer[64];
    serializeJson(doc, buffer);
    return enqueueMessage(topic, buffer);
}

bool MqttClient_PublishMessage(const char *topic, uint8_t value)
{
    return MqttClient_PublishMessage(topic, (int)value);
}

bool MqttClient_PublishMessage(const char *topic, uint16_t value)
{
    return MqttClient_PublishMessage(topic, (int)value);
}

bool MqttClient_PublishMessage(const char *topic, uint32_t value)
{
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    char buffer[64];
    serializeJson(doc, buffer);
    return enqueueMessage(topic, buffer);
}

bool MqttClient_PublishMessage(const char *topic, const String &value)
{
    StaticJsonDocument<64> doc;
    doc["string_value"] = value; // Store strings in "string_value"
    char buffer[64];
    serializeJson(doc, buffer);
    return enqueueMessage(topic, buffer);
}

bool MqttClient_PublishMessage(const char *topic, char value)
{
    StaticJsonDocument<64> doc;
    doc["value"] = String(value);
    char buffer[64];
    serializeJson(doc, buffer);
    return enqueueMessage(topic, buffer);
}

bool MqttClient_PublishMessage(const char *topic, double value)
{
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    char buffer[64];
    serializeJson(doc, buffer);
    return enqueueMessage(topic, buffer);
}

/* payload will be sent >>> Ex: {"value": "john"} */
bool MqttClient_PublishMessage(const char *topic, const char *value)
{
    StaticJsonDocument<64> doc;
    doc["string_value"] = value; // Store strings in "string_value"
    char buffer[64];
    serializeJson(doc, buffer);
    return enqueueMessage(topic, buffer);
}

#endif // MQTT_CLIENT_MODULE