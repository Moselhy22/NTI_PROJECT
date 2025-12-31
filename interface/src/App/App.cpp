/*
#include "../CFG.h"
#if APP_MODULE

#include "App.hpp"
#include <Arduino.h>
#include "../MqttClient/MqttClient.hpp"

/* ================= Timers ================= 
static unsigned long lastTask500ms = 0;
static unsigned long lastTask1sec  = 0;
static unsigned long lastTask5sec  = 0;

/* ================= Counter-based throttling ================= 
static uint8_t heartCounter = 0;
static uint8_t motionCounter = 0;
static uint8_t gpsCounter = 0;

/* ================= App Init ================= 
void App_Init(void)
{
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("   SMART HEALTH MONITOR SYSTEM START");
    Serial.println("========================================");

    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait until Wi-Fi is connected
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("✅ Wi-Fi Connected!");
    Serial.print("ESP32 MAC Address: ");
    Serial.println(WiFi.macAddress());

    I2C_Begin(21, 22, 100000);
    Serial.println("I2C Bus Initialized");

    BodyTempSensor_Init();
    MPU6050_Init();
    HeartRateSensor_Init();
    GPS_Init();
    MqttClient_Init();

    Serial.println("System Initialization Done ✔️");
}

/* ================= Scheduler ================= 
void App_RunTasks(void)
{
    unsigned long now = millis();

    /* ========== 500 ms – HEART SENSOR ========== 
    if (now - lastTask500ms >= 500)
    {
        HeartRateSensor_Update();
        long ir      = HeartRateSensor_ReadIR();
        int contact  = HeartRateSensor_ReadContactQuality();
        float spO2   = HeartRateSensor_ReadSpO2();

        Serial.println("\n🫀 HEART SENSOR");
        Serial.println("--------------------------------");
        Serial.print("IR Signal       : "); Serial.println(ir);
        Serial.print("Contact Quality : "); Serial.print(contact); Serial.println(" %");
        Serial.print("SpO2 Estimate   : "); Serial.print(spO2,1); Serial.println(" %");

        /* Throttled MQTT publishing 
        if (heartCounter % 1 == 0) MqttClient_PublishMessage("HealthMonitor_01/heart/ir", (uint32_t)ir);
        if (heartCounter % 2 == 0) MqttClient_PublishMessage("HealthMonitor_01/heart/contact", contact);
        if (heartCounter % 3 == 0) MqttClient_PublishMessage("HealthMonitor_01/heart/spo2", spO2);

        heartCounter = (heartCounter + 1) % 6; // Cycle counter to prevent overflow
        lastTask500ms = now;
    }

    /* ========== 1 sec – TEMP + MOTION ========== 
    if (now - lastTask1sec >= TASK_1SEC_INTERVAL)
    {
        Serial.println("\n🌡️ BODY & MOTION");
        Serial.println("--------------------------------");

        /* ---- Body Temp ---- 
        float bodyTemp = 0;
        if (BodyTempSensor_Read(&bodyTemp))
        {
            Serial.print("Body Temp : ");
            Serial.print(bodyTemp,1);
            Serial.println(" °C");

            if (motionCounter % 2 == 0) MqttClient_PublishMessage("HealthMonitor_01/body_temp", bodyTemp);
        }
        else
        {
            Serial.println("Body Temp : READ ERROR");
        }

        /* ---- MPU6050 ----
        MPU6050_Update();

        float ax = MPU6050_GetAccX();
        float ay = MPU6050_GetAccY();
        float az = MPU6050_GetAccZ();

        float gx = MPU6050_GetGyroX();
        float gy = MPU6050_GetGyroY();
        float gz = MPU6050_GetGyroZ();

        bool fall = MPU6050_DetectFall();

        Serial.print("ACC  : "); Serial.print(ax,2); Serial.print(", "); Serial.print(ay,2); Serial.print(", "); Serial.println(az,2);
        Serial.print("GYRO : "); Serial.print(gx,1); Serial.print(", "); Serial.print(gy,1); Serial.print(", "); Serial.println(gz,1);
        Serial.print("Fall : "); Serial.println(fall ? "YES ⚠️" : "NO");

        /* Throttled MQTT publishing 
        if (motionCounter % 1 == 0)
        {
            MqttClient_PublishMessage("HealthMonitor_01/motion/acc/x", ax);
            MqttClient_PublishMessage("HealthMonitor_01/motion/acc/y", ay);
            MqttClient_PublishMessage("HealthMonitor_01/motion/acc/z", az);

            MqttClient_PublishMessage("HealthMonitor_01/motion/gyro/x", gx);
            MqttClient_PublishMessage("HealthMonitor_01/motion/gyro/y", gy);
            MqttClient_PublishMessage("HealthMonitor_01/motion/gyro/z", gz);

            MqttClient_PublishMessage("HealthMonitor_01/motion/fall", fall);
        }

        motionCounter = (motionCounter + 1) % 6;
        lastTask1sec = now;
    }

    /* ========== 5 sec – GPS ========== 
    if (now - lastTask5sec >= TASK_5SEC_INTERVAL)
    {
        GPS_Update();

        Serial.println("\n📍 GPS");
        Serial.println("--------------------------------");

        if (gpsCounter % 1 == 0) MqttClient_PublishMessage("HealthMonitor_01/gps/sat", GPS_GetSatellites());

        if (GPS_IsFixAvailable())
        {
            float lat, lon;
            GPS_GetLocation(&lat, &lon);

            Serial.print("Latitude  : "); Serial.println(lat,6);
            Serial.print("Longitude : "); Serial.println(lon,6);
            Serial.print("Speed     : "); Serial.print(GPS_GetSpeedKmph()); Serial.println(" km/h");

            if (gpsCounter % 2 == 0)
            {
                MqttClient_PublishMessage("HealthMonitor_01/gps/lat", lat);
                MqttClient_PublishMessage("HealthMonitor_01/gps/lon", lon);
                MqttClient_PublishMessage("HealthMonitor_01/gps/speed", GPS_GetSpeedKmph());
            }
        }
        else
        {
            Serial.println("Waiting for GPS fix...");
        }

        gpsCounter = (gpsCounter + 1) % 6;
        lastTask5sec = now;
    }

    /* MQTT background task
    MqttClient_Task();
}

/* ================= MQTT Commands ================= 
void App_HandleMQTTCommand(const char *payload)
{
    if (strcmp(payload, "led on") == 0)
        digitalWrite(LED_PIN, HIGH);
    else if (strcmp(payload, "led off") == 0)
        digitalWrite(LED_PIN, LOW);
}

void App_handleNodeMQTTCommandRecived(const char *payload)
{
    // Forward it to your existing handler
    App_HandleMQTTCommand(payload);
}

#endif

*/
#include "../CFG.h"
#if APP_MODULE

#include "App.hpp"
#include <Arduino.h>
#include "../MqttClient/MqttClient.hpp"

/* ================= Timers ================= */
static unsigned long lastTask500ms = 0;
static unsigned long lastTask1sec  = 0;
static unsigned long lastTask5sec  = 0;

/* ================= Publish Throttling =================
   Prevent MQTT queue overflow */
static uint8_t heartCounter  = 0;
static uint8_t motionCounter = 0;
static uint8_t gpsCounter    = 0;

/* ================= App Init ================= */
void App_Init(void)
{
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("   SMART HEALTH MONITOR SYSTEM START");
    Serial.println("========================================");

    /* -------- WiFi -------- */
    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("\n✅ Wi-Fi Connected!");
    Serial.print("ESP32 MAC Address: ");
    Serial.println(WiFi.macAddress());

    /* -------- I2C -------- */
    I2C_Begin(21, 22, 100000);
    Serial.println("I2C Bus Initialized");

    /* -------- Sensors -------- */
    BodyTempSensor_Init();      // REMOVE if sensor not connected
    MPU6050_Init();             // REMOVE if sensor not connected
    HeartRateSensor_Init();
    GPS_Init();

    /* -------- MQTT -------- */
    MqttClient_Init();

    Serial.println("System Initialization Done ✔️");
}

/* ================= Scheduler ================= */
void App_RunTasks(void)
{
    unsigned long now = millis();

    /* =====================================================
       🫀 HEART SENSOR – every 500 ms
       ===================================================== */
    if (now - lastTask500ms >= 500)
    {
        HeartRateSensor_Update();

        long  ir     = HeartRateSensor_ReadIR();
        int   contact= HeartRateSensor_ReadContactQuality();
        float spO2   = HeartRateSensor_ReadSpO2();

        Serial.println("\n🫀 HEART SENSOR");
        Serial.println("--------------------------------");
        Serial.print("IR Signal       : "); Serial.println(ir);
        Serial.print("Contact Quality : "); Serial.print(contact); Serial.println(" %");
        Serial.print("SpO2 Estimate   : "); Serial.print(spO2,1); Serial.println(" %");

        /* MQTT (throttled) */
        if (heartCounter % 1 == 0) MqttClient_PublishMessage(NODE_ID "/heart/ir", (uint32_t)ir);
        if (heartCounter % 2 == 0) MqttClient_PublishMessage(NODE_ID "/heart/contact", contact);
        if (heartCounter % 3 == 0) MqttClient_PublishMessage(NODE_ID "/heart/spo2", spO2);

        heartCounter = (heartCounter + 1) % 6;
        lastTask500ms = now;
    }

    /* =====================================================
       🌡️ BODY TEMP + MOTION – every 1 sec
       ===================================================== */
    if (now - lastTask1sec >= TASK_1SEC_INTERVAL)
    {
        Serial.println("\n🌡️ BODY & MOTION");
        Serial.println("--------------------------------");

        /* -------- Body Temperature -------- */
        float bodyTemp = 0;
        if (BodyTempSensor_Read(&bodyTemp))
        {
            Serial.print("Body Temp : ");
            Serial.print(bodyTemp,1);
            Serial.println(" °C");

            if (motionCounter % 2 == 0)
                MqttClient_PublishMessage(NODE_ID "/body_temp", bodyTemp);
        }
        else
        {
            Serial.println("Body Temp : READ ERROR");
        }

        /* -------- MPU6050 -------- */
        MPU6050_Update();

        float ax = MPU6050_GetAccX();
        float ay = MPU6050_GetAccY();
        float az = MPU6050_GetAccZ();

        float gx = MPU6050_GetGyroX();
        float gy = MPU6050_GetGyroY();
        float gz = MPU6050_GetGyroZ();

        bool fall = MPU6050_DetectFall();

        Serial.print("ACC  : "); Serial.print(ax,2); Serial.print(", ");
        Serial.print(ay,2); Serial.print(", "); Serial.println(az,2);

        Serial.print("GYRO : "); Serial.print(gx,1); Serial.print(", ");
        Serial.print(gy,1); Serial.print(", "); Serial.println(gz,1);

        Serial.print("Fall : "); Serial.println(fall ? "YES ⚠️" : "NO");

        if (motionCounter % 1 == 0)
        {
            MqttClient_PublishMessage(NODE_ID "/motion/acc/x", ax);
            MqttClient_PublishMessage(NODE_ID "/motion/acc/y", ay);
            MqttClient_PublishMessage(NODE_ID "/motion/acc/z", az);

            MqttClient_PublishMessage(NODE_ID "/motion/gyro/x", gx);
            MqttClient_PublishMessage(NODE_ID "/motion/gyro/y", gy);
            MqttClient_PublishMessage(NODE_ID "/motion/gyro/z", gz);

            MqttClient_PublishMessage(NODE_ID "/motion/fall", fall);
        }

        motionCounter = (motionCounter + 1) % 6;
        lastTask1sec = now;
    }

    //* ========== 5 sec – GPS ========== */
if (now - lastTask5sec >= TASK_5SEC_INTERVAL)
{
    GPS_Update();

    Serial.println("\n📍 GPS");
    Serial.println("--------------------------------");

    /* ---- Satellites always published ---- */
    MqttClient_PublishMessage(NODE_ID "/gps/sat", GPS_GetSatellites());

    /* ---- Location ---- */
    if (GPS_IsLocationUpdated())
    {
        double lat = GPS_GetLatitude();
        double lon = GPS_GetLongitude();

        Serial.print("Latitude  : ");
        Serial.println(lat, 6);

        Serial.print("Longitude : ");
        Serial.println(lon, 6);

        /* Throttled MQTT publishing */
        if (gpsCounter % 2 == 0)
        {
            MqttClient_PublishMessage(NODE_ID "/gps/lat", lat);
            MqttClient_PublishMessage(NODE_ID "/gps/lon", lon);
        }
    }
    else
    {
        Serial.println("Waiting for GPS fix...");
    }

    gpsCounter = (gpsCounter + 1) % 6;
    lastTask5sec = now;
}


    /* -------- MQTT Background -------- */
    MqttClient_Task();
}

/* ================= MQTT Commands ================= */
void App_HandleMQTTCommand(const char *payload)
{
    if (strcmp(payload, "led on") == 0)
        digitalWrite(LED_PIN, HIGH);
    else if (strcmp(payload, "led off") == 0)
        digitalWrite(LED_PIN, LOW);
}

void App_handleNodeMQTTCommandRecived(const char *payload)
{
    App_HandleMQTTCommand(payload);
}

#endif


