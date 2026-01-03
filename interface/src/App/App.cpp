/*
#include "../CFG.h"
#if APP_MODULE

#include "App.hpp"
#include <Arduino.h>
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../AI/HealthModel/HealthModel.h"
#include "../MqttClient/MqttClient.hpp"

/* ================= GLOBAL ================= 
#define NODE_ID "HealthMonitor_01"

// Debug mode
#define DEBUG_MODE 1
#define DEBUG_PRINT(msg) if(DEBUG_MODE) Serial.println(msg)
#define DEBUG_PRINTF(...) if(DEBUG_MODE) Serial.printf(__VA_ARGS__)

SemaphoreHandle_t i2cMutex;
SemaphoreHandle_t dataMutex;
QueueHandle_t mqttCmdQueue;
#define MQTT_CMD_QUEUE_SIZE 10

/* ===== Shared Data ===== 
typedef struct {
    float bpmAvg;
    float temp;
    float motion;
    bool  fall;
    float aiScore;
    char  aiState[32];
    bool  aiAlert;

    float accX, accY, accZ;
    float gyroX, gyroY, gyroZ;

    double gpsLat, gpsLon;
} HealthData_t;

static HealthData_t gData;

/* ================= WIFI ================= 
static void WiFi_Init(void) {
    Serial.print("🌐 Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retries = 0;
    
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("📡 IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ WiFi Connection Failed!");
    }
}

/* ================= TEST MODES ================= 
static bool testModeActive = false;
static int testModeType = 0;

/* ========================================================= */
/* ======================= TASKS =========================== */
/* ========================================================= 

// -------- HEART RATE --------
void Task_HeartRate(void *pv) {
    DEBUG_PRINT("❤️ Heart Rate Task Started");
    
    TickType_t lastWake = xTaskGetTickCount();
    static long lastIRValue = 0;
    
    while (1) {
        if (!testModeActive) {
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(20))) {
                HeartRateSensor_Update();
                long irValue = HeartRateSensor_ReadIR();
                xSemaphoreGive(i2cMutex);
                
                float currentBPM = HeartRateSensor_ReadAvgBPM();
                int contact = HeartRateSensor_ReadContactQuality();
                
                // Store in global data
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
                    if (contact > 50 && currentBPM > 0) {
                        gData.bpmAvg = currentBPM;
                    }
                    xSemaphoreGive(dataMutex);
                }

                // Debug info
                if (irValue < 50000) {
                    Serial.println("   👉 Put finger on sensor!");
                }
            }
        }
        
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(250));
    }
}

// -------- BODY TEMP --------
void Task_BodyTemp(void *pv) {
    DEBUG_PRINT("🌡 Temperature Task Started");
    
    unsigned long lastPrint = 0;
    
    while (1) {
        if (!testModeActive) {
            float rawTemp = 0;
            
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {
                BodyTempSensor_Update();
                rawTemp = BodyTempSensor_ReadC();
                xSemaphoreGive(i2cMutex);
            }
            
            // Apply calibration if needed
            float bodyTemp = rawTemp;
            
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
                gData.temp = bodyTemp;
                xSemaphoreGive(dataMutex);
            }
            
            if (millis() - lastPrint > 3000) {
                Serial.printf("🌡 Temperature: %.1f°C\n", bodyTemp);
                lastPrint = millis();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// -------- MOTION / MPU6050 --------
void Task_Motion(void *pv) {
    DEBUG_PRINT("🤖 Motion Task Started");
    
    while (1) {
        if (!testModeActive) {
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(20))) {
                MPU6050_Update();
                xSemaphoreGive(i2cMutex);
            }

            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
                gData.accX = MPU6050_GetAccX();
                gData.accY = MPU6050_GetAccY();
                gData.accZ = MPU6050_GetAccZ();

                gData.motion = MPU6050_GetMotionMagnitude();
                gData.fall   = MPU6050_DetectFall();
                xSemaphoreGive(dataMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// -------- HEALTH AI (TENSORFLOW 1D-CNN) --------
void Task_HealthAI(void *pv) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    HealthModel_Init();
    
    Serial.println("\n🧠 AI MODEL STATUS: READY");
    Serial.println("   Model Type: TensorFlow Lite 1D-CNN");
    Serial.println("   Input: 128 samples × [HR, Temp, AccX, AccY, AccZ]");
    Serial.println("   Expected Output: Health Score 0.0-1.0");
    Serial.println("────────────────────────────────────────\n");
    
    TickType_t lastSampleTime = xTaskGetTickCount();
    HealthModelOutput_t ai_output;
    static int inference_count = 0;
    
    while (1) {
        HealthData_t local;
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            local = gData;
            xSemaphoreGive(dataMutex);
        }
        
        // Apply test data if in test mode
        if (testModeActive) {
            // Override with test data
            switch(testModeType) {
                case 1: // Stable
                    local.bpmAvg = 72.0;
                    local.temp = 36.7;
                    local.accX = 0.01;
                    local.accY = 0.02;
                    local.accZ = 1.0;
                    break;
                case 2: // Active
                    local.bpmAvg = 110.0;
                    local.temp = 37.0;
                    local.accX = 0.3;
                    local.accY = 0.2;
                    local.accZ = 1.15;
                    break;
                case 3: // Fever
                    local.bpmAvg = 125.0;
                    local.temp = 39.2;
                    local.accX = 0.25;
                    local.accY = 0.15;
                    local.accZ = 1.1;
                    break;
                case 4: // Seizure
                    local.bpmAvg = 150.0;
                    local.temp = 37.8;
                    local.accX = 2.5;
                    local.accY = 2.0;
                    local.accZ = 2.5;
                    break;
                case 5: // Fall
                    local.bpmAvg = 120.0;
                    local.temp = 36.8;
                    local.accX = -2.8;
                    local.accY = 3.2;
                    local.accZ = 0.4;
                    break;
            }
            
            // Update global data
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10))) {
                gData.bpmAvg = local.bpmAvg;
                gData.temp = local.temp;
                gData.accX = local.accX;
                gData.accY = local.accY;
                gData.accZ = local.accZ;
                xSemaphoreGive(dataMutex);
            }
        }
        
        // Add sample every 20ms (2.56 seconds for 128 samples)
        if (xTaskGetTickCount() - lastSampleTime >= pdMS_TO_TICKS(20)) {
            HealthModel_AddSample(local.bpmAvg, local.temp, 
                                 local.accX, local.accY, local.accZ);
            lastSampleTime = xTaskGetTickCount();
        }
        
        // Run inference when buffer is ready
        if (HealthModel_IsBufferReady()) {
            inference_count++;
            
            // 🔥 Run TensorFlow Model
            HealthModel_Run(&ai_output);
            
            // Update global AI data
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10))) {
                gData.aiScore = ai_output.score * 100.0f;
                strncpy(gData.aiState, ai_output.state, sizeof(gData.aiState)-1);
                gData.aiState[sizeof(gData.aiState)-1] = '\0';
                gData.aiAlert = ai_output.alert;
                xSemaphoreGive(dataMutex);
            }
            
            // 🎨 ATTRACTIVE SERIAL OUTPUT
            Serial.println("\n");
            Serial.println("╔══════════════════════════════════════════════════╗");
            Serial.println("║              HEALTH AI ANALYSIS                  ║");
            Serial.println("╠══════════════════════════════════════════════════╣");
            
            // Sensor Data
            Serial.println("║  📊 SENSOR DATA                                ║");
            Serial.println("╠──────────────────────────────────────────────────╣");
            Serial.printf("║  ❤️  Heart Rate:    %7.1f BPM                  ║\n", local.bpmAvg);
            Serial.printf("║  🌡  Temperature:   %7.1f°C                    ║\n", local.temp);
            Serial.printf("║  🏃  Motion Level:  %7.2f g                     ║\n", local.motion);
            Serial.printf("║  📍  Accelerometer: X:%.2f Y:%.2f Z:%.2f      ║\n", 
                         local.accX, local.accY, local.accZ);
            Serial.println("╠══════════════════════════════════════════════════╣");
            
            // AI Results
            Serial.println("║  🧠 TENSORFLOW AI RESULTS                      ║");
            Serial.println("╠──────────────────────────────────────────────────╣");
            
            float score_percent = ai_output.score * 100.0f;
            
            // Progress bar visualization
            Serial.printf("║  Score: [");
            int bars = (int)(score_percent / 5);
            for (int i = 0; i < 20; i++) {
                if (i < bars) {
                    if (score_percent < 30) Serial.print("█");  // Green
                    else if (score_percent < 70) Serial.print("▓");  // Yellow
                    else Serial.print("▒");  // Red
                } else {
                    Serial.print("░");
                }
            }
            Serial.printf("] %.1f%%", score_percent);
            for (int i = 0; i < 7 - (int)log10(fabs(score_percent)+1); i++) Serial.print(" ");
            Serial.println(" ║");
            
            // State with emoji
            const char* emoji = "";
            if (strstr(ai_output.state, "RESTING")) emoji = "😴";
            else if (strstr(ai_output.state, "NORMAL")) emoji = "😊";
            else if (strstr(ai_output.state, "FEVER")) emoji = "🤒";
            else if (strstr(ai_output.state, "SEIZURE")) emoji = "⚠️";
            else if (strstr(ai_output.state, "CRITICAL")) emoji = "🚨";
            else emoji = "❓";
            
            Serial.printf("║  State:  %s %-30s ║\n", emoji, ai_output.state);
            Serial.printf("║  Alert:  %s                              ║\n", 
                         ai_output.alert ? "⚠️ YES - Attention Needed" : "✅ NO - All Good");
            
            // Test mode indicator
            if (testModeActive) {
                const char* test_names[] = {"", "Stable", "Active", "Fever", "Seizure", "Fall"};
                Serial.printf("║  Mode:   🧪 TEST MODE - %-25s ║\n", test_names[testModeType]);
            } else {
                Serial.printf("║  Mode:   📡 REAL SENSORS                       ║\n");
            }
            
            // Risk Assessment
            Serial.println("╠══════════════════════════════════════════════════╣");
            Serial.println("║  📈 RISK ASSESSMENT                              ║");
            Serial.println("╠──────────────────────────────────────────────────╣");
            
            if (score_percent < 15) {
                Serial.println("║  🟢 LOW RISK: Healthy resting state              ║");
            } else if (score_percent < 40) {
                Serial.println("║  🟡 MODERATE: Normal activity level             ║");
            } else if (score_percent < 70) {
                Serial.println("║  🟠 ELEVATED: Medical attention suggested       ║");
            } else if (score_percent < 90) {
                Serial.println("║  🔴 HIGH: Immediate medical check recommended  ║");
            } else {
                Serial.println("║  🚨 CRITICAL: Emergency medical attention needed║");
            }
            
            Serial.printf("║  Inference #%d | Samples: 128                   ║\n", inference_count);
            
            // Alert section
            if (ai_output.alert) {
                Serial.println("╠══════════════════════════════════════════════════╣");
                Serial.println("║  🚨 MEDICAL ALERT DETECTED!                     ║");
                Serial.println("╠──────────────────────────────────────────────────╣");
                static bool alert_blink = false;
                if (alert_blink) {
                    Serial.println("║  🔴 🚑 🏥  URGENT: Check patient immediately!  ║");
                } else {
                    Serial.println("║  ⚠️  ⚠️  ⚠️   ALERT: Abnormal health detected!   ║");
                }
                alert_blink = !alert_blink;
            }
            
            Serial.println("╚══════════════════════════════════════════════════╝");
            Serial.println();
            
            // Reset buffer for next collection
            HealthModel_ResetBuffer();
            
            // Auto-exit test mode after showing results
            if (testModeActive) {
                testModeActive = false;
                testModeType = 0;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// -------- GPS --------
void Task_GPS(void *pv) {
    DEBUG_PRINT("📍 GPS Task Started");
    
    while (1) {
        GPS_Update();

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            if (GPS_IsLocationUpdated()) {
                gData.gpsLat = GPS_GetLatitude();
                gData.gpsLon = GPS_GetLongitude();
            }
            xSemaphoreGive(dataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void Task_MQTT(void *pv) {
    HealthData_t snap;
    char cmdBuffer[64];
    static unsigned long counter = 0;
    static bool firstPublishDone = false;
    
    DEBUG_PRINT("📡 MQTT Task Started");
    
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        // Process MQTT messages
        MqttClient_Task();
        
        counter++;
        
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            snap = gData;
            xSemaphoreGive(dataMutex);
        }
        
        // PUBLISH DATA REGULARLY USING COUNTER
        // Publish heart rate every 2 iterations
        if (counter % 2 == 0) {
            MqttClient_PublishMessage(NODE_ID "/heart/bpm", snap.bpmAvg);
        }
        
        // Publish temperature every 3 iterations
        if (counter % 3 == 0) {
            MqttClient_PublishMessage(NODE_ID "/temp", snap.temp);
        }
        
        // Publish motion every 4 iterations
        if (counter % 4 == 0) {
            MqttClient_PublishMessage(NODE_ID "/motion", snap.motion);
        }
        
        // Publish fall detection every iteration when true
        if (snap.fall) {
            MqttClient_PublishMessage(NODE_ID "/fall", snap.fall);
        }
        
        // Publish AI data every 5 iterations
        if (counter % 5 == 0) {
            MqttClient_PublishMessage(NODE_ID "/ai/score", snap.aiScore);
            MqttClient_PublishMessage(NODE_ID "/ai/state", snap.aiState);
            MqttClient_PublishMessage(NODE_ID "/ai/alert", snap.aiAlert);
        }
        
        // Publish GPS every 10 iterations
        if (counter % 10 == 0 && (snap.gpsLat != 0 || snap.gpsLon != 0)) {
            MqttClient_PublishMessage(NODE_ID "/gps/lat", snap.gpsLat);
            MqttClient_PublishMessage(NODE_ID "/gps/lon", snap.gpsLon);
        }
        
        // Reset counter to prevent overflow (every 60 iterations)
        if (counter >= 60) {
            counter = 0;
        }
        
        // Simple dashboard (optional - keep your existing Serial prints)
        if (counter % 5 == 0) {
            // Your existing Serial print code...
        }

        // Process MQTT commands
        while (uxQueueMessagesWaiting(mqttCmdQueue) > 0) {
            if (xQueueReceive(mqttCmdQueue, &cmdBuffer, pdMS_TO_TICKS(10)) == pdTRUE) {
                App_HandleMQTTCommand(cmdBuffer);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ================= MQTT COMMANDS ================= 
void App_HandleMQTTCommand(const char *payload) {
    Serial.printf("📨 MQTT COMMAND: '%s'\n", payload);

    if (strcmp(payload, "led on") == 0) {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("💡 LED turned ON");
    } 
    else if (strcmp(payload, "led off") == 0) {
        digitalWrite(LED_PIN, LOW);
        Serial.println("💡 LED turned OFF");
    }
    else if (strcmp(payload, "bpm") == 0) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            Serial.printf("❤️ Current BPM: %.1f\n", gData.bpmAvg);
            xSemaphoreGive(dataMutex);
        }
    }
    else if (strcmp(payload, "temp") == 0) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            Serial.printf("🌡 Current Temperature: %.1f°C\n", gData.temp);
            xSemaphoreGive(dataMutex);
        }
    }
    else if (strcmp(payload, "ai") == 0) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            Serial.printf("🧠 AI Score: %.1f/100 | State: %s | Alert: %s\n", 
                         gData.aiScore, gData.aiState, gData.aiAlert ? "YES" : "NO");
            xSemaphoreGive(dataMutex);
        }
    }
    // TEST COMMANDS
    else if (strcmp(payload, "test1") == 0) {
        testModeActive = true;
        testModeType = 1;
        Serial.println(">>> TEST MODE 1: Stable Resting");
    }
    else if (strcmp(payload, "test2") == 0) {
        testModeActive = true;
        testModeType = 2;
        Serial.println(">>> TEST MODE 2: Normal Activity");
    }
    else if (strcmp(payload, "test3") == 0) {
        testModeActive = true;
        testModeType = 3;
        Serial.println(">>> TEST MODE 3: Fever Condition");
    }
    else if (strcmp(payload, "test4") == 0) {
        testModeActive = true;
        testModeType = 4;
        Serial.println(">>> TEST MODE 4: Seizure Pattern");
    }
    else if (strcmp(payload, "test5") == 0) {
        testModeActive = true;
        testModeType = 5;
        Serial.println(">>> TEST MODE 5: Fall Detection");
    }
    else if (strcmp(payload, "test_off") == 0) {
        testModeActive = false;
        testModeType = 0;
        Serial.println(">>> TEST MODE OFF - Real sensors");
    }
    else if (strcmp(payload, "status") == 0) {
        Serial.println("\n>>> SYSTEM STATUS:");
        Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        Serial.printf("Test Mode: %s (%d)\n", testModeActive ? "ACTIVE" : "INACTIVE", testModeType);
        Serial.printf("AI Model: Ready\n");
        Serial.println(">>>");
    }
    else if (strcmp(payload, "mqtt_test") == 0) {
        Serial.println(">>> Testing MQTT publish...");
        bool success = MqttClient_PublishMessage(NODE_ID "/test", "MQTT test OK");
        Serial.printf(">>> MQTT Test: %s\n", success ? "SUCCESS" : "FAILED");
    }
    else {
        Serial.printf(">>> Unknown command: %s\n", payload);
        Serial.println(">>> Available commands:");
        Serial.println(">>>   led on/off, bpm, temp, ai, status, mqtt_test");
        Serial.println(">>>   test1, test2, test3, test4, test5, test_off");
    }
}

void App_handleNodeMQTTCommandRecived(const char *payload) {
    char payloadCopy[64];
    strncpy(payloadCopy, payload, sizeof(payloadCopy) - 1);
    payloadCopy[sizeof(payloadCopy) - 1] = '\0';
    
    if (xQueueSend(mqttCmdQueue, payloadCopy, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("MQTT Command Queue Full!");
    }
}

/* ================= INIT ================= 
void App_Init(void) {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════════════════╗");
    Serial.println("║         HEALTH MONITOR AI SYSTEM               ║");
    Serial.println("║         TensorFlow Lite 1D-CNN Model           ║");
    Serial.println("╚══════════════════════════════════════════════════╝\n");
    
    Serial.println("🚀 SYSTEM INITIALIZATION");
    Serial.println("──────────────────────────────────────────────────");
    
    i2cMutex  = xSemaphoreCreateMutex();
    dataMutex = xSemaphoreCreateMutex();
    mqttCmdQueue = xQueueCreate(MQTT_CMD_QUEUE_SIZE, sizeof(char[64]));
    
    if (i2cMutex == NULL || dataMutex == NULL || mqttCmdQueue == NULL) {
        Serial.println("❌ Failed to create RTOS objects!");
        while(1);
    }
    
    Serial.println("✅ RTOS objects created");
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    Serial.println("\n🔧 SENSOR INITIALIZATION");
    Serial.println("──────────────────────────────────────────────────");
    
    Serial.println("Initializing Temperature Sensor...");
    BodyTempSensorInit();
    delay(500);
    
    Serial.println("Initializing Heart Rate Sensor...");
    HeartRateSensor_Init();
    delay(500);
    
    Serial.println("Initializing Motion Sensor...");
    MPU6050_Init();
    delay(500);
    
    Serial.println("Initializing GPS...");
    GPS_Init();
    delay(500);
    
    Serial.println("\n✅ All sensors initialized");
    
    Serial.println("\n📡 NETWORK INITIALIZATION");
    Serial.println("──────────────────────────────────────────────────");
    Serial.println("Connecting to WiFi...");
    WiFi_Init();
    delay(1000);
    
    Serial.println("Initializing MQTT...");
    MqttClient_Init();
    delay(1000);
    
    // Initialize global data
    memset(&gData, 0, sizeof(gData));
    gData.temp = 36.5;
    gData.bpmAvg = 75.0;
    gData.aiScore = 0.0;
    strcpy(gData.aiState, "WAITING");
    gData.aiAlert = false;
    
    Serial.println("\n🎯 TASK CREATION");
    Serial.println("──────────────────────────────────────────────────");
    
    xTaskCreate(Task_Motion,    "Motion",   4096, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ Motion task created");
    
    xTaskCreate(Task_HeartRate, "HeartRate", 4096, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ Heart rate task created");
    
    xTaskCreate(Task_BodyTemp,  "Temperature", 4096, NULL, 1, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ Temperature task created");
    
    xTaskCreate(Task_GPS,       "GPS",      4096, NULL, 1, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ GPS task created");
    
    xTaskCreate(Task_HealthAI,  "AI",       8192, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ AI task created");
    
    xTaskCreate(Task_MQTT,      "MQTT",     8192, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ MQTT task created");
    
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("\n╔══════════════════════════════════════════════════╗");
    Serial.println("║           SYSTEM READY! 🎉                      ║");
    Serial.println("║                                                 ║");
    Serial.println("║  AI Model Testing System Active                ║");
    Serial.println("║  Use MQTT commands to test different scenarios ║");
    Serial.println("║  Commands: test1, test2, test3, test4, test5   ║");
    Serial.println("║  Each test runs once then returns to sensors   ║");
    Serial.println("╚══════════════════════════════════════════════════╝\n");
    
    delay(2000);
}

#endif

*/




#include "../CFG.h"
#if APP_MODULE

#include "App.hpp"
#include <Arduino.h>
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../AI/HealthModel/HealthModel.h"
#include "../MqttClient/MqttClient.hpp"
/* ================= GLOBAL ================= */
//#define NODE_ID "HealthMonitor_01"
/*test* */
#define NODE_ID           "sensors/data"
// Debug mode
#define DEBUG_MODE 1
#define DEBUG_PRINT(msg) if(DEBUG_MODE) Serial.println(msg)
#define DEBUG_PRINTF(...) if(DEBUG_MODE) Serial.printf(__VA_ARGS__)

// Temperature calibration constants
#define TEMP_OFFSET 10.0f   // Adjust this based on your sensor calibration
#define TEMP_GAIN 1.0f      // Adjust this if sensor has scaling issues

SemaphoreHandle_t i2cMutex;
SemaphoreHandle_t dataMutex;
QueueHandle_t mqttCmdQueue;
#define MQTT_CMD_QUEUE_SIZE 10

/* ===== Shared Data ===== */
typedef struct {
    float bpmAvg;
    float temp;
    float tempRaw;      // Raw temperature value before calibration
    float motion;
    bool  fall;
    float aiScore;
    char  aiState[32];
    bool  aiAlert;

    float accX, accY, accZ;
    float gyroX, gyroY, gyroZ;

    double gpsLat, gpsLon;
} HealthData_t;

static HealthData_t gData;

/* ================= WIFI ================= */
static void WiFi_Init(void) {
    Serial.print("🌐 Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retries = 0;
    
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("📡 IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ WiFi Connection Failed!");
    }
}

/* ================= TEST MODES ================= */
static bool testModeActive = false;
static int testModeType = 0;

/* ================= TEMPERATURE CALIBRATION ================= */
/* ================= TEMPERATURE CALIBRATION ================= */
static float calibrateTemperature(float rawTemp) {
    // If raw temperature is too low, no finger detected
    if (rawTemp < 20.0f) {
        return 0.0f;  // No finger detected
    }
    
    // Apply offset calibration (add 10°C to match body temperature)
    float calibrated = rawTemp + TEMP_OFFSET;
    
    // Apply gain if needed
    calibrated = calibrated * TEMP_GAIN;
    
    // Clamp to reasonable body temperature range
    if (calibrated > 52.0f) calibrated = 42.0f;    // Maximum reasonable fever
    
    return calibrated;
}

/* ========================================================= */
/* ======================= TASKS =========================== */
/* ========================================================= */

// -------- HEART RATE --------
void Task_HeartRate(void *pv) {
    DEBUG_PRINT("❤️ Heart Rate Task Started");
    
    TickType_t lastWake = xTaskGetTickCount();
    static float filteredBPM = 0.0f;
    
    while (1) {
        if (!testModeActive) {
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(20))) {
                HeartRateSensor_Update();
                long irValue = HeartRateSensor_ReadIR();
                xSemaphoreGive(i2cMutex);
                
                float currentBPM = HeartRateSensor_ReadAvgBPM();
                int contact = HeartRateSensor_ReadContactQuality();
                
                // SIMPLE FILTER: Remove unrealistic values
                if (currentBPM > 0) {
                    if (currentBPM < 40.0f || currentBPM > 180.0f) {
                        currentBPM = 0;  // Discard unrealistic value
                    } else if (currentBPM > 100.0f) {
                        // If BPM > 100, apply extra validation
                        // Wait for multiple high readings before accepting
                        static int highBPMCounter = 0;
                        highBPMCounter++;
                        if (highBPMCounter < 3) {
                            currentBPM = filteredBPM;  // Use previous value
                        } else {
                            filteredBPM = currentBPM;
                            highBPMCounter = 0;
                        }
                    } else {
                        filteredBPM = currentBPM;  // Accept reasonable value
                    }
                }
                
                // Store in global data
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
                    // RESET to 0 if no finger detected
                    if (irValue < 50000 || contact <= 50) {
                        gData.bpmAvg = 0.0f;  // No finger on sensor
                    } else if (currentBPM > 0) {
                        gData.bpmAvg = currentBPM;  // Valid reading
                    }
                    xSemaphoreGive(dataMutex);
                }

                // Debug info
                if (irValue < 50000) {
                    Serial.println("   👉 Put finger on sensor!");
                } else if (currentBPM > 0) {
                    Serial.printf("   ❤️ BPM: %.1f (Filtered)\n", currentBPM);
                }
            }
        }
        
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(250));
    }
}

// -------- BODY TEMP --------
// -------- BODY TEMP --------
void Task_BodyTemp(void *pv) {
    DEBUG_PRINT("🌡 Temperature Task Started");
    
    unsigned long lastPrint = 0;
    
    while (1) {
        if (!testModeActive) {
            float rawTemp = 0;
            
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {
                BodyTempSensor_Update();
                rawTemp = BodyTempSensor_ReadC();
                xSemaphoreGive(i2cMutex);
            }
            
            // Apply calibration (function now handles finger detection)
            float calibratedTemp = calibrateTemperature(rawTemp);
            
            // Determine if finger is detected based on calibrated value
            bool fingerDetected = (calibratedTemp > 0);
            
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
                gData.tempRaw = rawTemp;          // Store raw value
                gData.temp = calibratedTemp;      // Store calibrated value
                xSemaphoreGive(dataMutex);
            }
            
            if (millis() - lastPrint > 3000) {
                if (fingerDetected) {
                    Serial.printf("🌡 Raw: %.1f°C | Calibrated: %.1f°C\n", 
                                 rawTemp, calibratedTemp);
                } else {
                    Serial.println("🌡 No finger detected on temperature sensor");
                    Serial.printf("🌡 Raw: %.1f°C | Calibrated: %.1f°C\n", 
                                 rawTemp, calibratedTemp);
                }
                lastPrint = millis();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// -------- MOTION / MPU6050 --------
void Task_Motion(void *pv) {
    DEBUG_PRINT("🤖 Motion Task Started");
    
    while (1) {
        if (!testModeActive) {
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(20))) {
                MPU6050_Update();
                xSemaphoreGive(i2cMutex);
            }

            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
                gData.accX = MPU6050_GetAccX();
                gData.accY = MPU6050_GetAccY();
                gData.accZ = MPU6050_GetAccZ();

                gData.motion = MPU6050_GetMotionMagnitude();
                gData.fall   = MPU6050_DetectFall();
                xSemaphoreGive(dataMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// -------- HEALTH AI (TENSORFLOW 1D-CNN) --------
void Task_HealthAI(void *pv) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    HealthModel_Init();
    
    Serial.println("\n🧠 AI MODEL STATUS: READY");
    Serial.println("   Model Type: TensorFlow Lite 1D-CNN");
    Serial.println("   Input: 128 samples × [HR, Temp, AccX, AccY, AccZ]");
    Serial.println("   Expected Output: Health Score 0.0-1.0");
    Serial.println("────────────────────────────────────────\n");
    
    TickType_t lastSampleTime = xTaskGetTickCount();
    HealthModelOutput_t ai_output;
    static int inference_count = 0;
    
    while (1) {
        HealthData_t local;
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            local = gData;
            xSemaphoreGive(dataMutex);
        }
        
        // Apply test data if in test mode
        if (testModeActive) {
            // Override with test data
            switch(testModeType) {
                case 1: // Stable
                    local.bpmAvg = 72.0;
                    local.temp = 36.7;
                    local.accX = 0.01;
                    local.accY = 0.02;
                    local.accZ = 1.0;
                    break;
                case 2: // Active
                    local.bpmAvg = 110.0;
                    local.temp = 37.0;
                    local.accX = 0.3;
                    local.accY = 0.2;
                    local.accZ = 1.15;
                    break;
                case 3: // Fever
                    local.bpmAvg = 125.0;
                    local.temp = 39.2;
                    local.accX = 0.25;
                    local.accY = 0.15;
                    local.accZ = 1.1;
                    break;
                case 4: // Seizure
                    local.bpmAvg = 150.0;
                    local.temp = 37.8;
                    local.accX = 2.5;
                    local.accY = 2.0;
                    local.accZ = 2.5;
                    break;
                case 5: // Fall
                    local.bpmAvg = 120.0;
                    local.temp = 36.8;
                    local.accX = -2.8;
                    local.accY = 3.2;
                    local.accZ = 0.4;
                    break;
            }
            
            // Update global data
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10))) {
                gData.bpmAvg = local.bpmAvg;
                gData.temp = local.temp;
                gData.accX = local.accX;
                gData.accY = local.accY;
                gData.accZ = local.accZ;
                xSemaphoreGive(dataMutex);
            }
        }
        
        // Add sample every 20ms (2.56 seconds for 128 samples)
        if (xTaskGetTickCount() - lastSampleTime >= pdMS_TO_TICKS(20)) {
            // Send calibrated temperature to AI
            HealthModel_AddSample(local.bpmAvg, local.temp, 
                                 local.accX, local.accY, local.accZ);
            lastSampleTime = xTaskGetTickCount();
        }
        
        // Run inference when buffer is ready
        if (HealthModel_IsBufferReady()) {
            inference_count++;
            
            // 🔥 Run TensorFlow Model
            HealthModel_Run(&ai_output);
            
            // Update global AI data
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10))) {
                gData.aiScore = ai_output.score * 100.0f;
                strncpy(gData.aiState, ai_output.state, sizeof(gData.aiState)-1);
                gData.aiState[sizeof(gData.aiState)-1] = '\0';
                gData.aiAlert = ai_output.alert;
                xSemaphoreGive(dataMutex);
            }
            
            // 🎨 ATTRACTIVE SERIAL OUTPUT
            Serial.println("\n");
            Serial.println("╔══════════════════════════════════════════════════╗");
            Serial.println("║              HEALTH AI ANALYSIS                  ║");
            Serial.println("╠══════════════════════════════════════════════════╣");
            
            // Sensor Data
            Serial.println("║  📊 SENSOR DATA                                ║");
            Serial.println("╠──────────────────────────────────────────────────╣");
            Serial.printf("║  ❤️  Heart Rate:    %7.1f BPM                  ║\n", local.bpmAvg);
            Serial.printf("║  🌡  Temperature:   %7.1f°C (Raw: %.1f°C)      ║\n", 
                         local.temp, local.tempRaw);
            Serial.printf("║  🏃  Motion Level:  %7.2f g                     ║\n", local.motion);
            Serial.printf("║  📍  Accelerometer: X:%.2f Y:%.2f Z:%.2f      ║\n", 
                         local.accX, local.accY, local.accZ);
            Serial.println("╠══════════════════════════════════════════════════╣");
            
            // AI Results
            Serial.println("║  🧠 TENSORFLOW AI RESULTS                      ║");
            Serial.println("╠──────────────────────────────────────────────────╣");
            
            float score_percent = ai_output.score * 100.0f;
            
            // Progress bar visualization
            Serial.printf("║  Score: [");
            int bars = (int)(score_percent / 5);
            for (int i = 0; i < 20; i++) {
                if (i < bars) {
                    if (score_percent < 30) Serial.print("█");  // Green
                    else if (score_percent < 70) Serial.print("▓");  // Yellow
                    else Serial.print("▒");  // Red
                } else {
                    Serial.print("░");
                }
            }
            Serial.printf("] %.1f%%", score_percent);
            for (int i = 0; i < 7 - (int)log10(fabs(score_percent)+1); i++) Serial.print(" ");
            Serial.println(" ║");
            
            // State with emoji
            const char* emoji = "";
            if (strstr(ai_output.state, "RESTING")) emoji = "😴";
            else if (strstr(ai_output.state, "NORMAL")) emoji = "😊";
            else if (strstr(ai_output.state, "FEVER")) emoji = "🤒";
            else if (strstr(ai_output.state, "SEIZURE")) emoji = "⚠️";
            else if (strstr(ai_output.state, "CRITICAL")) emoji = "🚨";
            else emoji = "❓";
            
            Serial.printf("║  State:  %s %-30s ║\n", emoji, ai_output.state);
            Serial.printf("║  Alert:  %s                              ║\n", 
                         ai_output.alert ? "⚠️ YES - Attention Needed" : "✅ NO - All Good");
            
            // Test mode indicator
            if (testModeActive) {
                const char* test_names[] = {"", "Stable", "Active", "Fever", "Seizure", "Fall"};
                Serial.printf("║  Mode:   🧪 TEST MODE - %-25s ║\n", test_names[testModeType]);
            } else {
                Serial.printf("║  Mode:   📡 REAL SENSORS                       ║\n");
            }
            
            // Risk Assessment
            Serial.println("╠══════════════════════════════════════════════════╣");
            Serial.println("║  📈 RISK ASSESSMENT                              ║");
            Serial.println("╠──────────────────────────────────────────────────╣");
            
            if (score_percent < 15) {
                Serial.println("║  🟢 LOW RISK: Healthy resting state              ║");
            } else if (score_percent < 40) {
                Serial.println("║  🟡 MODERATE: Normal activity level             ║");
            } else if (score_percent < 70) {
                Serial.println("║  🟠 ELEVATED: Medical attention suggested       ║");
            } else if (score_percent < 90) {
                Serial.println("║  🔴 HIGH: Immediate medical check recommended  ║");
            } else {
                Serial.println("║  🚨 CRITICAL: Emergency medical attention needed║");
            }
            
            Serial.printf("║  Inference #%d | Samples: 128                   ║\n", inference_count);
            
            // Alert section
            if (ai_output.alert) {
                Serial.println("╠══════════════════════════════════════════════════╣");
                Serial.println("║  🚨 MEDICAL ALERT DETECTED!                     ║");
                Serial.println("╠──────────────────────────────────────────────────╣");
                static bool alert_blink = false;
                if (alert_blink) {
                    Serial.println("║  🔴 🚑 🏥  URGENT: Check patient immediately!  ║");
                } else {
                    Serial.println("║  ⚠️  ⚠️  ⚠️   ALERT: Abnormal health detected!   ║");
                }
                alert_blink = !alert_blink;
            }
            
            Serial.println("╚══════════════════════════════════════════════════╝");
            Serial.println();
            
            // Reset buffer for next collection
            HealthModel_ResetBuffer();
            
            // Auto-exit test mode after showing results
            if (testModeActive) {
                testModeActive = false;
                testModeType = 0;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// -------- GPS --------
void Task_GPS(void *pv) {
    DEBUG_PRINT("📍 GPS Task Started");
    
    while (1) {
        GPS_Update();

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            if (GPS_IsLocationUpdated()) {
                gData.gpsLat = GPS_GetLatitude();
                gData.gpsLon = GPS_GetLongitude();
            }
            xSemaphoreGive(dataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
/*work*/
/*work*/

void Task_MQTT(void *pv) {
    HealthData_t snap;
    char cmdBuffer[64];
    static unsigned long lastHeartPublish = 0;
    static unsigned long lastTempPublish = 0;
    static unsigned long lastAIPublish = 0;
    static unsigned long lastMotionPublish = 0;
    static bool lastFallState = false;
    static float lastBPM = 0;
    static float lastTemp = 0;
    static float lastMotion = 0;
    static float lastAIScore = 0;
    static char lastAIState[32] = {0};
    static bool lastAIAlert = false;
    
    DEBUG_PRINT("📡 MQTT Task Started");
    
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        // Process MQTT messages
        MqttClient_Task();
        
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            snap = gData;
            xSemaphoreGive(dataMutex);
        }
        
        unsigned long now = millis();
        
        // PUBLISH HEART RATE - Every 1 second or when changed significantly
        if ((now - lastHeartPublish > 1000) || (fabs(snap.bpmAvg - lastBPM) > 5.0)) {
            MqttClient_PublishMessage(NODE_ID "/heart/bpm", snap.bpmAvg);
            lastHeartPublish = now;
            lastBPM = snap.bpmAvg;
        }
        
        // PUBLISH TEMPERATURE - Every 1 second or when changed
        if ((now - lastTempPublish > 1000) || (fabs(snap.temp - lastTemp) > 0.5)) {
            if (snap.temp > 0) {  // Only publish when finger detected
                MqttClient_PublishMessage(NODE_ID "/temp", snap.temp);
            } else {
                // Publish 0 when no finger detected
                MqttClient_PublishMessage(NODE_ID "/temp", 0.0f);
            }
            lastTempPublish = now;
            lastTemp = snap.temp;
        }
        
        // PUBLISH MOTION - Every 2 seconds or when changed
        if ((now - lastMotionPublish > 2000) || (fabs(snap.motion - lastMotion) > 0.1)) {
            MqttClient_PublishMessage(NODE_ID "/motion", snap.motion);
            lastMotionPublish = now;
            lastMotion = snap.motion;
        }
        
        // PUBLISH FALL DETECTION - Immediately when state changes
        if (snap.fall != lastFallState) {
            MqttClient_PublishMessage(NODE_ID "/fall", snap.fall);
            lastFallState = snap.fall;
        }
        
        // PUBLISH AI DATA - Every 3 seconds or when changed significantly
        if ((now - lastAIPublish > 3000) || 
            (fabs(snap.aiScore - lastAIScore) > 5.0) ||
            (strcmp(snap.aiState, lastAIState) != 0) ||
            (snap.aiAlert != lastAIAlert)) {
            
            MqttClient_PublishMessage(NODE_ID "/ai/score", snap.aiScore);
            MqttClient_PublishMessage(NODE_ID "/ai/state", snap.aiState);
            MqttClient_PublishMessage(NODE_ID "/ai/alert", snap.aiAlert);
            
            lastAIPublish = now;
            lastAIScore = snap.aiScore;
            strncpy(lastAIState, snap.aiState, sizeof(lastAIState)-1);
            lastAIState[sizeof(lastAIState)-1] = '\0';
            lastAIAlert = snap.aiAlert;
        }
        
        // PUBLISH GPS - Every 10 seconds
        if (now % 10000 == 0 && (snap.gpsLat != 0 || snap.gpsLon != 0)) {
            MqttClient_PublishMessage(NODE_ID "/gps/lat", snap.gpsLat);
            MqttClient_PublishMessage(NODE_ID "/gps/lon", snap.gpsLon);
        }
        
        // Simple dashboard every 5 seconds
        static unsigned long lastDashboard = 0;
        if (now - lastDashboard > 5000) {
            Serial.println("\n──────────────────────────────────────────────────");
            Serial.println("📊 HEALTH MONITOR - LIVE DATA");
            Serial.println("──────────────────────────────────────────────────");
            Serial.printf("❤️  Heart Rate:    %6.1f BPM\n", snap.bpmAvg);
            Serial.printf("🌡  Temperature:   %6.1f°C (Raw: %.1f°C)\n", snap.temp, snap.tempRaw);
            Serial.printf("🏃  Motion Level:  %6.2f g\n", snap.motion);
            Serial.printf("⚠️  Fall Detected: %s\n", snap.fall ? "YES" : "NO");
            Serial.printf("🧠  AI Score:      %6.1f/100\n", snap.aiScore);
            Serial.printf("📊  AI State:      %s\n", snap.aiState);
            Serial.printf("🚨  AI Alert:      %s\n", snap.aiAlert ? "YES" : "NO");
            
            if (snap.gpsLat != 0 || snap.gpsLon != 0) {
                Serial.printf("📍  GPS Location:  %.6f, %.6f\n", snap.gpsLat, snap.gpsLon);
            }
            
            Serial.printf("📡  MQTT Last Publish: HR:%lu, Temp:%lu ms ago\n", 
                         now - lastHeartPublish, now - lastTempPublish);
            Serial.println("──────────────────────────────────────────────────\n");
            lastDashboard = now;
        }

        // Process MQTT commands
        while (uxQueueMessagesWaiting(mqttCmdQueue) > 0) {
            if (xQueueReceive(mqttCmdQueue, &cmdBuffer, pdMS_TO_TICKS(10)) == pdTRUE) {
                App_HandleMQTTCommand(cmdBuffer);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Run faster - every 100ms
    }
}
    
/* ================= MQTT COMMANDS ================= */
void App_HandleMQTTCommand(const char *payload) 
{
    Serial.printf("📨 MQTT COMMAND: '%s'\n", payload);

    if (strcmp(payload, "led on") == 0) {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("💡 LED turned ON");
    } 
    else if (strcmp(payload, "led off") == 0) {
        digitalWrite(LED_PIN, LOW);
        Serial.println("💡 LED turned OFF");
    }
    else if (strcmp(payload, "bpm") == 0) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            Serial.printf("❤️ Current BPM: %.1f\n", gData.bpmAvg);
            xSemaphoreGive(dataMutex);
        }
    }
    else if (strcmp(payload, "temp") == 0) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            Serial.printf("🌡 Current Temperature: %.1f°C (Raw: %.1f°C)\n", gData.temp, gData.tempRaw);
            xSemaphoreGive(dataMutex);
        }
    }
    else if (strcmp(payload, "ai") == 0) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20))) {
            Serial.printf("🧠 AI Score: %.1f/100 | State: %s | Alert: %s\n", 
                         gData.aiScore, gData.aiState, gData.aiAlert ? "YES" : "NO");
            xSemaphoreGive(dataMutex);
        }
    }
    // TEST COMMANDS
    else if (strcmp(payload, "test1") == 0) {
        testModeActive = true;
        testModeType = 1;
        Serial.println(">>> TEST MODE 1: Stable Resting");
    }
    else if (strcmp(payload, "test2") == 0) {
        testModeActive = true;
        testModeType = 2;
        Serial.println(">>> TEST MODE 2: Normal Activity");
    }
    else if (strcmp(payload, "test3") == 0) {
        testModeActive = true;
        testModeType = 3;
        Serial.println(">>> TEST MODE 3: Fever Condition");
    }
    else if (strcmp(payload, "test4") == 0) {
        testModeActive = true;
        testModeType = 4;
        Serial.println(">>> TEST MODE 4: Seizure Pattern");
    }
    else if (strcmp(payload, "test5") == 0) {
        testModeActive = true;
        testModeType = 5;
        Serial.println(">>> TEST MODE 5: Fall Detection");
    }
    else if (strcmp(payload, "test_off") == 0) {
        testModeActive = false;
        testModeType = 0;
        Serial.println(">>> TEST MODE OFF - Real sensors");
    }
    else if (strcmp(payload, "status") == 0) {
        Serial.println("\n>>> SYSTEM STATUS:");
        Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        Serial.printf("Test Mode: %s (%d)\n", testModeActive ? "ACTIVE" : "INACTIVE", testModeType);
        Serial.printf("Temperature Calibration: Offset=%.1f°C, Gain=%.2f\n", TEMP_OFFSET, TEMP_GAIN);
        Serial.printf("AI Model: Ready\n");
        Serial.println(">>>");
    }
    else if (strcmp(payload, "calibrate") == 0) {
        Serial.println(">>> Temperature Calibration Info:");
        Serial.printf(">>> Current: Raw=%.1f°C, Calibrated=%.1f°C\n", gData.tempRaw, gData.temp);
        Serial.println(">>> To adjust calibration, modify TEMP_OFFSET and TEMP_GAIN in App.cpp");
        Serial.println(">>> TEMP_OFFSET = measured_body_temp - raw_sensor_reading");
    }
    else if (strcmp(payload, "mqtt_test") == 0) {
        Serial.println(">>> Testing MQTT publish...");
        bool success = MqttClient_PublishMessage(NODE_ID "/test", "MQTT test OK");
        Serial.printf(">>> MQTT Test: %s\n", success ? "SUCCESS" : "FAILED");
    }
    else {
        Serial.printf(">>> Unknown command: %s\n", payload);
        Serial.println(">>> Available commands:");
        Serial.println(">>>   led on/off, bpm, temp, ai, status, mqtt_test, calibrate");
        Serial.println(">>>   test1, test2, test3, test4, test5, test_off");
    }
}

void App_handleNodeMQTTCommandRecived(const char *payload) {
    char payloadCopy[64];
    strncpy(payloadCopy, payload, sizeof(payloadCopy) - 1);
    payloadCopy[sizeof(payloadCopy) - 1] = '\0';
    
    if (xQueueSend(mqttCmdQueue, payloadCopy, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("MQTT Command Queue Full!");
    }
}

/* ================= INIT ================= */
void App_Init(void) {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════════════════╗");
    Serial.println("║         HEALTH MONITOR AI SYSTEM               ║");
    Serial.println("║         TensorFlow Lite 1D-CNN Model           ║");
    Serial.println("╚══════════════════════════════════════════════════╝\n");
    
    Serial.println("🚀 SYSTEM INITIALIZATION");
    Serial.println("──────────────────────────────────────────────────");
    
    i2cMutex  = xSemaphoreCreateMutex();
    dataMutex = xSemaphoreCreateMutex();
    mqttCmdQueue = xQueueCreate(MQTT_CMD_QUEUE_SIZE, sizeof(char[64]));
    
    if (i2cMutex == NULL || dataMutex == NULL || mqttCmdQueue == NULL) {
        Serial.println("❌ Failed to create RTOS objects!");
        while(1);
    }
    
    Serial.println("✅ RTOS objects created");
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    Serial.println("\n🔧 SENSOR INITIALIZATION");
    Serial.println("──────────────────────────────────────────────────");
    
    Serial.println("Initializing Temperature Sensor...");
    BodyTempSensorInit();
    delay(500);
    
    Serial.println("Initializing Heart Rate Sensor...");
    HeartRateSensor_Init();
    delay(500);
    
    Serial.println("Initializing Motion Sensor...");
    MPU6050_Init();
    delay(500);
    
    Serial.println("Initializing GPS...");
    GPS_Init();
    delay(500);
    
    Serial.println("\n✅ All sensors initialized");
    
    Serial.println("\n📡 NETWORK INITIALIZATION");
    Serial.println("──────────────────────────────────────────────────");
    Serial.println("Connecting to WiFi...");
    WiFi_Init();
    delay(1000);
    
    Serial.println("Initializing MQTT...");
    MqttClient_Init();
    delay(1000);
    
    // Initialize global data
    memset(&gData, 0, sizeof(gData));
    gData.temp = 0.0f;      // Start with 0 (no finger)
    gData.tempRaw = 0.0f;   // Start with 0
    gData.bpmAvg = 0.0f;    // Start with 0 (no finger)
    gData.aiScore = 0.0f;
    strcpy(gData.aiState, "WAITING");
    gData.aiAlert = false;
    
    Serial.println("\n🎯 TASK CREATION");
    Serial.println("──────────────────────────────────────────────────");
    
    xTaskCreate(Task_Motion,    "Motion",   4096, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ Motion task created");
    
    xTaskCreate(Task_HeartRate, "HeartRate", 4096, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ Heart rate task created");
    
    xTaskCreate(Task_BodyTemp,  "Temperature", 4096, NULL, 1, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ Temperature task created");
    
    xTaskCreate(Task_GPS,       "GPS",      4096, NULL, 1, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ GPS task created");
    
    xTaskCreate(Task_HealthAI,  "AI",       8192, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ AI task created");
    
    xTaskCreate(Task_MQTT,      "MQTT",     8192, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✅ MQTT task created");
    
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("\n╔══════════════════════════════════════════════════╗");
    Serial.println("║           SYSTEM READY! 🎉                      ║");
    Serial.println("║                                                 ║");
    Serial.println("║  AI Model Testing System Active                ║");
    Serial.println("║  Use MQTT commands to test different scenarios ║");
    Serial.println("║  Commands: test1, test2, test3, test4, test5   ║");
    Serial.println("║  Each test runs once then returns to sensors   ║");
    Serial.println("╚══════════════════════════════════════════════════╝\n");
    
    delay(2000);
}

#endif


























/*test ai model*/
/*
#include "../CFG.h"
#if APP_MODULE

#include "App.hpp"
#include <Arduino.h>
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../AI/HealthModel/HealthModel.h"

/* ================= GLOBAL ================= 
#define NODE_ID "HealthMonitor_01"

// Debug mode
#define DEBUG_MODE 1
#define DEBUG_PRINT(msg) if(DEBUG_MODE) Serial.println(msg)
#define DEBUG_PRINTF(...) if(DEBUG_MODE) Serial.printf(__VA_ARGS__)

SemaphoreHandle_t i2cMutex;
SemaphoreHandle_t dataMutex;

/* ===== Shared Data ===== 
typedef struct
{
    float bpmAvg;
    float temp;
    float motion;
    bool  fall;
    float aiScore;

    float accX, accY, accZ;
    float gyroX, gyroY, gyroZ;

    double gpsLat, gpsLon;
} HealthData_t;

static HealthData_t gData;

/* ================= CALIBRATION ================= 
#define TEMP_CALIBRATION_OFFSET  12.5f

/* ================= TEST MODES ================= 
typedef enum {
    TEST_MODE_NONE = 0,
    TEST_MODE_STABLE = 1,
    TEST_MODE_ACTIVE = 2,
    TEST_MODE_FEVER = 3,
    TEST_MODE_SEIZURE = 4,
    TEST_MODE_FALL = 5,
    TEST_MODE_REAL_SENSORS = 6
} TestMode_t;

static TestMode_t currentTestMode = TEST_MODE_NONE;
static bool testModeActive = false;
static unsigned long testModeStartTime = 0;

/* ================= TEST DATA SETS ================= 
typedef struct {
    const char* name;
    float bpm;
    float temp;
    float accX, accY, accZ;
    float expectedScoreMin;
    float expectedScoreMax;
    const char* description;
} TestDataset_t;

static const TestDataset_t testDatasets[] = {
    // Stable resting - healthy person at rest
    {
        "Stable Resting",
        72.0f,      // bpm
        36.7f,      // temp
        0.01f,      // accX
        0.02f,      // accY
        1.00f,      // accZ (gravity)
        0.0f,       // min score
        15.0f,      // max score
        "Normal healthy resting state"
    },
    
    // Normal activity - walking/mild exercise
    {
        "Normal Activity",
        110.0f,     // bpm
        37.0f,      // temp
        0.30f,      // accX
        0.20f,      // accY
        1.15f,      // accZ
        15.0f,      // min score
        30.0f,      // max score
        "Normal physical activity"
    },
    
    // Fever condition - elevated temperature
    {
        "Fever Condition",
        125.0f,     // bpm
        39.2f,      // temp
        0.25f,      // accX
        0.15f,      // accY
        1.10f,      // accZ
        40.0f,      // min score
        70.0f,      // max score
        "Fever with elevated heart rate"
    },
    
    // Seizure pattern - violent shaking
    {
        "Seizure Pattern",
        150.0f,     // bpm
        37.8f,      // temp
        2.50f,      // accX
        2.00f,      // accY
        2.50f,      // accZ
        85.0f,      // min score
        100.0f,     // max score
        "Seizure-like movement pattern"
    },
    
    // Fall detection - sudden impact
    {
        "Fall Detection",
        120.0f,     // bpm
        36.8f,      // temp
        -2.80f,     // accX
        3.20f,      // accY
        0.40f,      // accZ
        70.0f,      // min score
        100.0f,     // max score
        "Simulated fall with impact"
    },
    
    // Stress test - borderline abnormal
    {
        "Stress Test",
        95.0f,      // bpm
        37.5f,      // temp
        0.80f,      // accX
        0.60f,      // accY
        1.30f,      // accZ
        25.0f,      // min score
        50.0f,      // max score
        "Mild stress/agitation"
    },
    
    // End of array marker
    {NULL, 0, 0, 0, 0, 0, 0, 0, NULL}
};

/* ================= WIFI ================= 
static void WiFi_Init(void)
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retries = 0;
    
    while (WiFi.status() != WL_CONNECTED && retries < 20)
    {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nWiFi Connection Failed!");
    }
}

/* ========================================================= */
/* ======================= TASKS =========================== */
/* ========================================================= 

// -------- HEART RATE --------
void Task_HeartRate(void *pv)
{
    DEBUG_PRINT("Heart Rate Task Started");

    static float beatAvg = 0;
    const byte RATE_SIZE = 4;
    byte rates[RATE_SIZE] = {0};
    byte rateSpot = 0;

    while (1)
    {
        if (currentTestMode == TEST_MODE_REAL_SENSORS) {
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(20)))
            {
                HeartRateSensor_Update();
                long irValue = HeartRateSensor_ReadIR();
                xSemaphoreGive(i2cMutex);
                
                float currentBPM = HeartRateSensor_ReadAvgBPM();
                int contact = HeartRateSensor_ReadContactQuality();
                
                if (currentBPM > 40.0f && currentBPM < 180.0f) {
                    rates[rateSpot++] = (byte)currentBPM;
                    rateSpot %= RATE_SIZE;
                    
                    beatAvg = 0;
                    for (byte x = 0; x < RATE_SIZE; x++)
                        beatAvg += rates[x];
                    beatAvg /= RATE_SIZE;
                }
                
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)))
                {
                    gData.bpmAvg = beatAvg;
                    xSemaphoreGive(dataMutex);
                }

                if (irValue < 50000) {
                    Serial.println("Put finger on sensor!");
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// -------- BODY TEMP --------
void Task_BodyTemp(void *pv)
{
    DEBUG_PRINT("Temperature Task Started");
    
    unsigned long lastPrint = 0;
    
    while (1)
    {
        if (currentTestMode == TEST_MODE_REAL_SENSORS) {
            float rawTemp = 0;
            
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)))
            {
                BodyTempSensor_Update();
                rawTemp = BodyTempSensor_ReadC();
                xSemaphoreGive(i2cMutex);
            }
            
            // Apply calibration
            float bodyTemp = rawTemp + TEMP_CALIBRATION_OFFSET;
            
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)))
            {
                gData.temp = bodyTemp;
                xSemaphoreGive(dataMutex);
            }
            
            if (millis() - lastPrint > 3000)
            {
                Serial.printf("Temperature: %.1fC\n", bodyTemp);
                lastPrint = millis();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// -------- MOTION / MPU6050 --------
void Task_Motion(void *pv)
{
    DEBUG_PRINT("Motion Task Started");
    
    while (1)
    {
        if (currentTestMode == TEST_MODE_REAL_SENSORS) {
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(20)))
            {
                MPU6050_Update();
                xSemaphoreGive(i2cMutex);
            }

            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)))
            {
                gData.accX = MPU6050_GetAccX();
                gData.accY = MPU6050_GetAccY();
                gData.accZ = MPU6050_GetAccZ();

                gData.motion = MPU6050_GetMotionMagnitude();
                gData.fall   = MPU6050_DetectFall();
                xSemaphoreGive(dataMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// -------- HEALTH AI (TENSORFLOW 1D-CNN) --------
void Task_HealthAI(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(3000));
    HealthModel_Init();
    
    DEBUG_PRINT("AI Task Started");
    
    TickType_t lastSampleTime = xTaskGetTickCount();
    HealthModelOutput_t ai_output;
    
    while (1)
    {
        HealthData_t local;
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)))
        {
            local = gData;
            xSemaphoreGive(dataMutex);
        }
        
        // Apply test data if in test mode
        if (currentTestMode >= TEST_MODE_STABLE && currentTestMode <= TEST_MODE_FALL) {
            int testIndex = currentTestMode - 1;
            if (testIndex >= 0 && testIndex < 5) {
                const TestDataset_t* testData = &testDatasets[testIndex];
                
                // Add slight random variation to simulate real data
                float bpmVariation = random(-5, 6) * 0.1f;
                float tempVariation = random(-10, 11) * 0.01f;
                float accVariation = random(-50, 51) * 0.001f;
                
                local.bpmAvg = testData->bpm + bpmVariation;
                local.temp = testData->temp + tempVariation;
                local.accX = testData->accX + accVariation;
                local.accY = testData->accY + accVariation;
                local.accZ = testData->accZ + accVariation;
                
                // Update global data with test values
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10))) {
                    gData.bpmAvg = local.bpmAvg;
                    gData.temp = local.temp;
                    gData.accX = local.accX;
                    gData.accY = local.accY;
                    gData.accZ = local.accZ;
                    xSemaphoreGive(dataMutex);
                }
            }
        }
        
        // Add sample every 20ms
        if (xTaskGetTickCount() - lastSampleTime >= pdMS_TO_TICKS(20))
        {
            HealthModel_AddSample(local.bpmAvg, local.temp, 
                                 local.accX, local.accY, local.accZ);
            lastSampleTime = xTaskGetTickCount();
        }
        
        // Run inference when buffer is ready
        if (HealthModel_IsBufferReady())
        {
            HealthModel_Run(&ai_output);
            
            // Update global AI score
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)))
            {
                gData.aiScore = ai_output.score * 100.0f;
                xSemaphoreGive(dataMutex);
            }
            
            // Display comprehensive results
            Serial.println("\n╔════════════════════════════════════════╗");
            Serial.println("║            AI TEST RESULTS             ║");
            Serial.println("╚════════════════════════════════════════╝");
            
            if (currentTestMode == TEST_MODE_NONE) {
                Serial.println("Mode: WAITING FOR TEST SELECTION");
            } else if (currentTestMode == TEST_MODE_REAL_SENSORS) {
                Serial.println("Mode: REAL SENSOR DATA");
                Serial.printf("  Heart Rate:    %.1f BPM\n", local.bpmAvg);
                Serial.printf("  Temperature:   %.1f°C\n", local.temp);
                Serial.printf("  Acceleration:  X:%.3f Y:%.3f Z:%.3f\n", 
                             local.accX, local.accY, local.accZ);
            } else {
                int testIndex = currentTestMode - 1;
                if (testIndex >= 0 && testIndex < 5) {
                    const TestDataset_t* testData = &testDatasets[testIndex];
                    Serial.printf("Mode: TEST - %s\n", testData->name);
                    Serial.printf("  Description:  %s\n", testData->description);
                    Serial.printf("  Heart Rate:   %.1f BPM\n", local.bpmAvg);
                    Serial.printf("  Temperature:  %.1f°C\n", local.temp);
                    Serial.printf("  Expected:     %.0f-%.0f/100\n", 
                                 testData->expectedScoreMin, testData->expectedScoreMax);
                }
            }
            
            Serial.println("╔════════════════════════════════════════╗");
            Serial.println("║           AI INFERENCE RESULT          ║");
            Serial.println("╠════════════════════════════════════════╣");
            Serial.printf("║  Score:         %7.1f/100          ║\n", ai_output.score * 100.0f);
            Serial.printf("║  State:         %-20s║\n", ai_output.state);
            Serial.printf("║  Alert:         %-20s║\n", ai_output.alert ? "YES" : "NO");
            
            // Performance evaluation
            if (currentTestMode >= TEST_MODE_STABLE && currentTestMode <= TEST_MODE_FALL) {
                int testIndex = currentTestMode - 1;
                if (testIndex >= 0 && testIndex < 5) {
                    const TestDataset_t* testData = &testDatasets[testIndex];
                    float actualScore = ai_output.score * 100.0f;
                    
                    Serial.println("╠════════════════════════════════════════╣");
                    Serial.println("║         TEST PERFORMANCE EVAL          ║");
                    Serial.println("╠════════════════════════════════════════╣");
                    
                    if (actualScore >= testData->expectedScoreMin && 
                        actualScore <= testData->expectedScoreMax) {
                        Serial.println("║  Result:        ✓ WITHIN EXPECTED RANGE ║");
                    } else if (actualScore < testData->expectedScoreMin) {
                        Serial.println("║  Result:        ⚠ BELOW EXPECTED RANGE  ║");
                    } else {
                        Serial.println("║  Result:        ⚠ ABOVE EXPECTED RANGE  ║");
                    }
                    
                    Serial.printf("║  Deviation:     %+7.1f points       ║\n", 
                                 actualScore - ((testData->expectedScoreMin + testData->expectedScoreMax) / 2));
                }
            }
            
            if (ai_output.alert) {
                Serial.println("╠════════════════════════════════════════╣");
                Serial.println("║           ⚠ MEDICAL ALERT ⚠            ║");
                Serial.println("║  Medical attention may be needed!      ║");
            }
            
            Serial.println("╚════════════════════════════════════════╝\n");
            
            // Reset buffer
            HealthModel_ResetBuffer();
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// -------- TEST CONTROL TASK --------
void Task_TestControl(void *pv)
{
    DEBUG_PRINT("Test Control Task Started");
    
    unsigned long lastMenuDisplay = 0;
    bool testRunning = false;
    unsigned long testDuration = 10000; // 10 seconds per test
    unsigned long testEndTime = 0;
    
    while (1)
    {
        unsigned long currentTime = millis();
        
        // Display menu every 10 seconds when not in test
        if (!testRunning && (currentTime - lastMenuDisplay > 10000)) {
            Serial.println("\n╔════════════════════════════════════════╗");
            Serial.println("║       AI MODEL TESTING MENU            ║");
            Serial.println("╠════════════════════════════════════════╣");
            Serial.println("║  Select test mode:                     ║");
            Serial.println("║                                        ║");
            Serial.println("║  1. Stable Resting                     ║");
            Serial.println("║  2. Normal Activity                    ║");
            Serial.println("║  3. Fever Condition                    ║");
            Serial.println("║  4. Seizure Pattern                    ║");
            Serial.println("║  5. Fall Detection                     ║");
            Serial.println("║                                        ║");
            Serial.println("║  6. Real Sensors (if connected)        ║");
            Serial.println("║  0. Stop All Tests                     ║");
            Serial.println("║                                        ║");
            Serial.println("║  Enter number (0-6):                   ║");
            Serial.println("╚════════════════════════════════════════╝");
            lastMenuDisplay = currentTime;
        }
        
        // Check for serial input
        if (Serial.available() > 0) {
            char input = Serial.read();
            
            // Clear the input buffer
            while (Serial.available() > 0) {
                Serial.read();
            }
            
            switch(input) {
                case '0':
                    currentTestMode = TEST_MODE_NONE;
                    testRunning = false;
                    Serial.println("\n>>> All tests stopped.");
                    Serial.println(">>> Waiting for new test selection.");
                    break;
                    
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                    currentTestMode = static_cast<TestMode_t>(input - '0');
                    testRunning = true;
                    testEndTime = currentTime + testDuration;
                    testModeStartTime = currentTime;
                    
                    Serial.println("\n>>> Test started!");
                    break;
                    
                case '6':
                    currentTestMode = TEST_MODE_REAL_SENSORS;
                    testRunning = true;
                    testEndTime = currentTime + testDuration;
                    testModeStartTime = currentTime;
                    
                    Serial.println("\n>>> Using real sensor data for 10 seconds.");
                    Serial.println(">>> Make sure sensors are connected properly.");
                    break;
                    
                default:
                    Serial.println("\n>>> Invalid input! Please enter 0-6.");
                    break;
            }
        }
        
        // Check if test duration has elapsed
        if (testRunning && currentTime >= testEndTime) {
            if (currentTestMode == TEST_MODE_REAL_SENSORS) {
                Serial.println("\n>>> Real sensor test completed.");
                Serial.println(">>> Returning to manual mode.");
            } else if (currentTestMode != TEST_MODE_NONE) {
                Serial.println("\n>>> Test completed automatically.");
                Serial.println(">>> Ready for next test.");
            }
            
            currentTestMode = TEST_MODE_NONE;
            testRunning = false;
            lastMenuDisplay = 0; // Force menu display
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// -------- GPS --------
void Task_GPS(void *pv)
{
    DEBUG_PRINT("GPS Task Started");
    
    while (1)
    {
        GPS_Update();

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)))
        {
            if (GPS_IsLocationUpdated())
            {
                gData.gpsLat = GPS_GetLatitude();
                gData.gpsLon = GPS_GetLongitude();
            }
            xSemaphoreGive(dataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ================= INIT ================= 
void App_Init(void)
{
    Serial.begin(115200);
    delay(1000);
    
    // ASCII art banner
    Serial.println("\n╔══════════════════════════════════════════════════════╗");
    Serial.println("║                                                      ║");
    Serial.println("║       HEALTH MONITOR AI MODEL TESTING SYSTEM         ║");
    Serial.println("║                                                      ║");
    Serial.println("║       TensorFlow Lite for Microcontrollers           ║");
    Serial.println("║               1D-CNN Health Model                    ║");
    Serial.println("║                                                      ║");
    Serial.println("╚══════════════════════════════════════════════════════╝\n");
    
    Serial.println("SYSTEM INITIALIZATION");
    Serial.println("=====================");
    
    i2cMutex  = xSemaphoreCreateMutex();
    dataMutex = xSemaphoreCreateMutex();
    
    if (i2cMutex == NULL || dataMutex == NULL) {
        Serial.println("ERROR: Failed to create RTOS objects!");
        while(1);
    }
    
    Serial.println("✓ RTOS objects created");
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    Serial.println("\nSENSOR INITIALIZATION");
    Serial.println("=====================");
    
    Serial.println("Initializing temperature sensor...");
    BodyTempSensorInit();
    delay(500);
    
    Serial.println("Initializing heart rate sensor...");
    HeartRateSensor_Init();
    delay(500);
    
    Serial.println("Initializing motion sensor...");
    MPU6050_Init();
    delay(500);
    
    Serial.println("Initializing GPS...");
    GPS_Init();
    delay(500);
    
    Serial.println("\n✓ All sensors initialized");
    
    Serial.println("\nNETWORK INITIALIZATION");
    Serial.println("======================");
    Serial.println("Connecting to WiFi...");
    WiFi_Init();
    delay(1000);
    
    memset(&gData, 0, sizeof(gData));
    gData.temp = 36.5;
    gData.bpmAvg = 75.0;
    gData.aiScore = 0.0;
    
    Serial.println("\nTASK CREATION");
    Serial.println("=============");
    
    xTaskCreate(Task_Motion,       "Motion",       4096, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✓ Motion task created");
    
    xTaskCreate(Task_HeartRate,    "HeartRate",    4096, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✓ Heart rate task created");
    
    xTaskCreate(Task_BodyTemp,     "Temperature",  4096, NULL, 1, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✓ Temperature task created");
    
    xTaskCreate(Task_GPS,          "GPS",          4096, NULL, 1, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✓ GPS task created");
    
    xTaskCreate(Task_HealthAI,     "AI",           8192, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✓ AI task created");
    
    xTaskCreate(Task_TestControl,  "TestControl",  4096, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("✓ Test control task created");
    
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║           SYSTEM READY!                ║");
    Serial.println("║                                        ║");
    Serial.println("║  AI Model Testing System Active        ║");
    Serial.println("║  Use Serial Monitor to select tests    ║");
    Serial.println("║  Tests run for 10 seconds each         ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    delay(2000);
}

#endif
*/
