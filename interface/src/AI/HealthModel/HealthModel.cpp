
#include "HealthModel.h"
#include "model_data.h"
#include <Arduino.h>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <math.h>
#include <string.h>

/* ================= CONFIG ================= */
#define TENSOR_ARENA_SIZE (60 * 1024)
#define SAMPLE_BUFFER_SIZE 128
#define MEDICAL_THRESHOLD 0.2987f

/* ================= STATIC ================= */
static uint8_t tensor_arena[TENSOR_ARENA_SIZE];
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input  = nullptr;
static TfLiteTensor* output = nullptr;

/* ================= SAMPLE BUFFER ================= */
static SensorSample_t sample_buffer[SAMPLE_BUFFER_SIZE];
static int sample_index = 0;
static bool buffer_ready = false;

// Medical smoothing (2-out-of-3 rule)
static int predictions[3] = {0, 0, 0};
static int pred_index = 0;

/* ================= INIT ================= */
void HealthModel_Init(void)
{
    // Initialize sample buffer
    HealthModel_ResetBuffer();
    
    // Set up TensorFlow Lite
    static tflite::MicroMutableOpResolver<20> resolver;
    
    // Add operations for 1D-CNN
    resolver.AddConv2D();        // For 1D conv (treated as 2D with height=1)
    resolver.AddFullyConnected();
    resolver.AddMaxPool2D();
    resolver.AddAveragePool2D();
    resolver.AddReshape();
    resolver.AddRelu();
    resolver.AddSoftmax();
    resolver.AddConcatenation();
    resolver.AddAdd();
    resolver.AddMul();
    resolver.AddLogistic();
    resolver.AddMean();
    resolver.AddExpandDims();
    resolver.AddStridedSlice();
    resolver.AddPack();
    resolver.AddShape();
    resolver.AddGather();
    resolver.AddTranspose();
    
    // Load model
    const tflite::Model* model = tflite::GetModel(final_health_model);
    
    static tflite::MicroInterpreter static_interpreter(
        model,
        resolver,
        tensor_arena,
        TENSOR_ARENA_SIZE
    );
    
    interpreter = &static_interpreter;
    
    if (interpreter->AllocateTensors() != kTfLiteOk)
    {
        Serial.println("❌ AI Tensor allocation failed");
        while (1);
    }
    
    input  = interpreter->input(0);
    output = interpreter->output(0);
    
    // Verify input dimensions
    Serial.println("✅ Health AI Model Initialized");
    Serial.print("Input dimensions: ");
    for (int i = 0; i < input->dims->size; i++) {
        Serial.printf("%d ", input->dims->data[i]);
    }
    Serial.println();
}

/* ================= SAMPLE MANAGEMENT ================= */
void HealthModel_AddSample(float hr, float temp, float accX, float accY, float accZ)
{
    if (sample_index >= SAMPLE_BUFFER_SIZE) {
        buffer_ready = true;
        return;
    }
    
    // Store sample with CRITICAL order: [HR, Temp, AccX, AccY, AccZ]
    sample_buffer[sample_index].hr = hr;
    sample_buffer[sample_index].temp = temp;
    sample_buffer[sample_index].accX = accX;
    sample_buffer[sample_index].accY = accY;
    sample_buffer[sample_index].accZ = accZ;
    
    sample_index++;
    
    if (sample_index >= SAMPLE_BUFFER_SIZE) {
        buffer_ready = true;
        Serial.println("📊 Collected 128 samples for AI inference");
    }
}

bool HealthModel_IsBufferReady(void)
{
    return buffer_ready;
}

void HealthModel_ResetBuffer(void)
{
    sample_index = 0;
    buffer_ready = false;
    memset(sample_buffer, 0, sizeof(sample_buffer));
}

/* ================= INFERENCE ================= */
void HealthModel_Run(HealthModelOutput_t* out)
{
    if (!buffer_ready) {
        out->score = 0.0f;
        out->state = "WAITING";
        out->alert = false;
        return;
    }
    
    if (!interpreter || !input) {
        out->score = -1.0f;
        out->state = "ERROR";
        out->alert = false;
        return;
    }
    
    // ===== Fill input tensor =====
    for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
        input->data.f[i * 5 + 0] = sample_buffer[i].hr;    // Heart Rate
        input->data.f[i * 5 + 1] = sample_buffer[i].temp;  // Temperature
        input->data.f[i * 5 + 2] = sample_buffer[i].accX;  // Accel X
        input->data.f[i * 5 + 3] = sample_buffer[i].accY;  // Accel Y
        input->data.f[i * 5 + 4] = sample_buffer[i].accZ;  // Accel Z
    }
    
    // ===== Debug: print what we're sending =====
    Serial.println("\n🔍 DEBUG: First 3 samples sent to TensorFlow:");
    for (int i = 0; i < 3; i++) {
        Serial.printf("   Sample %d: HR=%.1f, T=%.1f, Acc=(%.3f,%.3f,%.3f)\n",
                     i, sample_buffer[i].hr, sample_buffer[i].temp,
                     sample_buffer[i].accX, sample_buffer[i].accY, sample_buffer[i].accZ);
    }
    
    // ===== Run TensorFlow =====
    unsigned long start_time = micros();
    if (interpreter->Invoke() != kTfLiteOk) {
        out->score = -1.0f;
        out->state = "INFERENCE_ERROR";
        out->alert = false;
        return;
    }
    unsigned long inference_time = micros() - start_time;
    
    // ===== READ TENSORFLOW OUTPUT CORRECTLY =====
    // هنا الجزء المهم: إزاي بنقرأ النتيجة من TensorFlow
    
    int output_size = output->bytes / sizeof(float);
    Serial.printf("🔍 TensorFlow Output: %d value(s)\n", output_size);
    
    // الحالة 1: لو الـ output واحد (binary classification)
    if (output_size == 1) {
        float raw_value = output->data.f[0];
        
        Serial.printf("   Raw output[0]: %.6f\n", raw_value);
        
        // المحاولة 1: لو القيمة بين 0 و1 مباشرة
        if (raw_value >= 0.0f && raw_value <= 1.0f) {
            out->score = raw_value;  // probability مباشرة
            Serial.println("   Interpretation: Direct probability (0.0-1.0)");
        }
        // المحاولة 2: لو القيمة logits (قبل sigmoid)
        else {
            // تطبيق sigmoid: 1 / (1 + e^-x)
            float probability = 1.0f / (1.0f + expf(-raw_value));
            out->score = probability;
            Serial.printf("   Interpretation: Logits -> Probability: %.6f -> %.6f\n", 
                         raw_value, probability);
        }
    }
    // الحالة 2: لو الـ output 4 قيم (multi-class)
    else if (output_size == 4) {
        Serial.println("   4-class softmax output:");
        
        // نجيب أعلى قيمة
        int max_index = 0;
        float max_value = output->data.f[0];
        
        for (int i = 0; i < 4; i++) {
            float val = output->data.f[i];
            Serial.printf("      Class %d: %.6f (%.2f%%)\n", i, val, val * 100.0f);
            
            if (val > max_value) {
                max_value = val;
                max_index = i;
            }
        }
        
        out->score = max_value;
        
        // نترجم الـ index لـ state
        switch(max_index) {
            case 0: out->state = "STABLE_RESTING"; break;
            case 1: out->state = "NORMAL_ACTIVITY"; break;
            case 2: out->state = "FEVER_DETECTED"; break;
            case 3: out->state = "SEIZURE_DETECTED"; break;
            default: out->state = "UNKNOWN";
        }
        
        Serial.printf("   Highest: Class %d -> %s (%.2f%% confidence)\n",
                     max_index, out->state, max_value * 100.0f);
    }
    // الحالة 3: أي عدد آخر
    else {
        Serial.printf("   Warning: Unexpected output size: %d\n", output_size);
        out->score = output->data.f[0];  // ناخد أول قيمة
        
        // نطبع كل القيم للديبج
        for (int i = 0; i < output_size && i < 10; i++) {
            Serial.printf("      Output[%d] = %.6f\n", i, output->data.f[i]);
        }
    }
    
    // ===== Determine alert based on score =====
    float score_percent = out->score * 100.0f;
    
    if (score_percent >= 40.0f) {
        out->alert = true;
        if (score_percent >= 70.0f) {
            out->state = "CRITICAL_SEIZURE";
        } else if (score_percent >= 40.0f) {
            out->state = "ABNORMAL_FEVER";
        }
    } else if (score_percent >= 15.0f) {
        out->alert = false;
        out->state = "NORMAL_ACTIVITY";
    } else {
        out->alert = false;
        out->state = "STABLE_RESTING";
    }
    
    Serial.printf("⏱️  TensorFlow Inference time: %lu μs\n", inference_time);
    Serial.printf("📊 Final: Score=%.2f%%, State=%s, Alert=%s\n",
                 score_percent, out->state, out->alert ? "YES" : "NO");
    
    // Reset buffer
    HealthModel_ResetBuffer();
}



/*
#include "HealthModel.h"
#include "model_data.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

/* ================= CONFIG ================= 
#define SAMPLE_BUFFER_SIZE 128
#define TENSOR_ARENA_SIZE (64 * 1024)  // For TensorFlow

/* ================= STATIC VARIABLES ================= 
static float sample_buffer[SAMPLE_BUFFER_SIZE][5];
static int sample_index = 0;
static bool buffer_ready = false;

// TensorFlow Lite variables
static uint8_t tensor_arena[TENSOR_ARENA_SIZE];
static void* interpreter = nullptr;  // Using void* to avoid TF Lite dependencies if not available

/* ================= DEBUG INFO ================= 
void PrintModelInfo(void)
{
    Serial.println("\n=== HEALTH MODEL INFO ===");
    Serial.printf("Model data size: %d bytes\n", final_health_model_len);
    Serial.printf("Sample buffer: %d samples\n", SAMPLE_BUFFER_SIZE);
    Serial.println("Features order: [HR, Temp, AccX, AccY, AccZ]");
    Serial.println("=========================\n");
}

/* ================= INITIALIZATION ================= 
void HealthModel_Init(void)
{
    Serial.println("\n=== HEALTH AI MODEL INITIALIZATION ===");
    
    // Check if we have TensorFlow model data
    if (final_health_model_len > 100) {  // If model has reasonable size
        Serial.println("✓ TensorFlow model data detected");
        Serial.printf("Model size: %d bytes\n", final_health_model_len);
        
        // Try to initialize TensorFlow model
        // Note: This is simplified - you'd need actual TF Lite Micro here
        Serial.println("⚠️ Note: TensorFlow integration requires proper setup");
    } else {
        Serial.println("⚠️ Using algorithmic health model (TensorFlow not available)");
    }
    
    PrintModelInfo();
    
    // Initialize sample buffer
    HealthModel_ResetBuffer();
    
    // Initialize random seed for variations
    randomSeed(micros());
    
    Serial.println("✓ Health model initialized successfully");
}

/* ================= SAMPLE MANAGEMENT ================= 
void HealthModel_AddSample(float hr, float temp, float accX, float accY, float accZ)
{
    if (sample_index >= SAMPLE_BUFFER_SIZE) {
        buffer_ready = true;
        return;
    }
    
    // Store in correct order: HR, Temp, AccX, AccY, AccZ
    sample_buffer[sample_index][0] = hr;
    sample_buffer[sample_index][1] = temp;
    sample_buffer[sample_index][2] = accX;
    sample_buffer[sample_index][3] = accY;
    sample_buffer[sample_index][4] = accZ;
    
    sample_index++;
    
    if (sample_index >= SAMPLE_BUFFER_SIZE) {
        buffer_ready = true;
        Serial.println("📊 AI Buffer Full (128 samples collected)");
    }
}

bool HealthModel_IsBufferReady(void)
{
    return buffer_ready;
}

void HealthModel_ResetBuffer(void)
{
    sample_index = 0;
    buffer_ready = false;
    memset(sample_buffer, 0, sizeof(sample_buffer));
}

/* ================= MEDICAL SCORING ALGORITHM ================= 
float CalculateMedicalScore(float avgHR, float avgTemp, float avgMotion, 
                           float maxHR, float maxTemp, float maxMotion)
{
    float score = 0.0f;
    
    // 1. Heart Rate Analysis (0-35 points)
    if (avgHR < 60.0f) {                    // Bradycardia
        score += 25.0f;
    } else if (avgHR <= 100.0f) {           // Normal resting
        score += 5.0f;
    } else if (avgHR <= 120.0f) {           // Light activity
        score += 15.0f;
    } else if (avgHR <= 140.0f) {           // Moderate exercise
        score += 30.0f;
    } else if (avgHR <= 160.0f) {           // High activity/stress
        score += 50.0f;
    } else {                                // Tachycardia/emergency
        score += 75.0f;
    }
    
    // 2. Temperature Analysis (0-35 points)
    if (avgTemp < 35.5f) {                  // Hypothermia
        score += 40.0f;
    } else if (avgTemp <= 37.5f) {          // Normal
        score += 5.0f;
    } else if (avgTemp <= 38.5f) {          // Fever
        score += 40.0f;
    } else if (avgTemp <= 40.0f) {          // High fever
        score += 65.0f;
    } else {                                // Critical hyperthermia
        score += 85.0f;
    }
    
    // 3. Motion Analysis (0-30 points)
    if (avgMotion < 0.1f) {                 // Resting/sleeping
        score += 5.0f;
    } else if (avgMotion < 0.5f) {          // Light movement
        score += 10.0f;
    } else if (avgMotion < 1.0f) {          // Walking
        score += 20.0f;
    } else if (avgMotion < 2.0f) {          // Running
        score += 35.0f;
    } else if (avgMotion < 3.0f) {          // Vigorous activity
        score += 50.0f;
    } else {                                // Seizure/fall range
        score += 80.0f;
    }
    
    // 4. Peak value penalties
    if (maxHR > 160.0f) score += 20.0f;
    if (maxTemp > 39.0f) score += 25.0f;
    if (maxMotion > 2.5f) score += 30.0f;
    
    return fminf(score, 100.0f);
}

/* ================= HEALTH STATE DETERMINATION ================= 
const char* DetermineHealthState(float scorePercent, float avgHR, float avgTemp, float maxMotion)
{
    // Special conditions first
    if (maxMotion > 3.0f) {
        return "FALL_DETECTED";
    }
    
    if (avgTemp > 38.0f && avgHR > 120.0f) {
        return "FEVER_DETECTED";
    }
    
    if (maxMotion > 2.5f && avgHR > 140.0f) {
        return "SEIZURE_SUSPECTED";
    }
    
    // General scoring
    if (scorePercent < 15.0f) {
        return "STABLE_RESTING";
    } else if (scorePercent < 30.0f) {
        return "NORMAL_ACTIVITY";
    } else if (scorePercent < 50.0f) {
        return "ELEVATED_VITALS";
    } else if (scorePercent < 70.0f) {
        return "CONCERNING";
    } else if (scorePercent < 90.0f) {
        return "CRITICAL";
    } else {
        return "EMERGENCY";
    }
}

bool DetermineAlert(const char* state)
{
    // States that require alerts
    const char* alert_states[] = {
        "FALL_DETECTED",
        "FEVER_DETECTED", 
        "SEIZURE_SUSPECTED",
        "CONCERNING",
        "CRITICAL",
        "EMERGENCY"
    };
    
    for (int i = 0; i < 6; i++) {
        if (strcmp(state, alert_states[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

/* ================= MAIN INFERENCE FUNCTION ================= 
void HealthModel_Run(HealthModelOutput_t* out)
{
    if (!buffer_ready) {
        out->score = 0.0f;
        out->state = "WAITING_FOR_DATA";
        out->alert = false;
        return;
    }
    
    // Calculate statistics from the 128 samples
    float avgHR = 0, avgTemp = 0, avgMotion = 0;
    float maxHR = 0, maxTemp = 0, maxMotion = 0;
    
    for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
        float hr = sample_buffer[i][0];
        float temp = sample_buffer[i][1];
        
        avgHR += hr;
        avgTemp += temp;
        
        // Calculate motion magnitude (in g)
        float motion = sqrtf(
            sample_buffer[i][2] * sample_buffer[i][2] +
            sample_buffer[i][3] * sample_buffer[i][3] +
            sample_buffer[i][4] * sample_buffer[i][4]
        );
        avgMotion += motion;
        
        // Track maximum values
        if (hr > maxHR) maxHR = hr;
        if (temp > maxTemp) maxTemp = temp;
        if (motion > maxMotion) maxMotion = motion;
    }
    
    avgHR /= SAMPLE_BUFFER_SIZE;
    avgTemp /= SAMPLE_BUFFER_SIZE;
    avgMotion /= SAMPLE_BUFFER_SIZE;
    
    // DEBUG: Show data summary
    Serial.println("\n=== AI DATA ANALYSIS ===");
    Serial.printf("Average HR: %.1f BPM (Range: 60-100 normal)\n", avgHR);
    Serial.printf("Average Temp: %.1f°C (Range: 36.5-37.5 normal)\n", avgTemp);
    Serial.printf("Average Motion: %.3f g (Resting: <0.1g)\n", avgMotion);
    Serial.printf("Peak HR: %.1f BPM, Peak Temp: %.1f°C, Peak Motion: %.3f g\n", 
                  maxHR, maxTemp, maxMotion);
    Serial.println("======================\n");
    
    // Calculate health score (0-100%)
    float scorePercent = CalculateMedicalScore(avgHR, avgTemp, avgMotion, maxHR, maxTemp, maxMotion);
    
    // Convert to 0.0-1.0 for output
    out->score = scorePercent / 100.0f;
    
    // Determine health state
    out->state = DetermineHealthState(scorePercent, avgHR, avgTemp, maxMotion);
    
    // Determine if alert is needed
    out->alert = DetermineAlert(out->state);
    
    // Reset buffer for next collection
    HealthModel_ResetBuffer();
    
    Serial.printf("AI Result: Score=%.1f%%, State=%s, Alert=%s\n",
                  scorePercent, out->state, out->alert ? "YES" : "NO");
}
                  */
