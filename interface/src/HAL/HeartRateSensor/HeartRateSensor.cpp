#include "HeartRateSensor.h"
#include "../../MCAL/I2C_Driver/I2C_Driver.h"
#include <Wire.h>

// Global variables
static bool hrInitialized = false;
static long irValue = 0;
static int contactQuality = 0;
static float beatsPerMinute = 0.0f;
static float beatAvg = 0.0f;

// For BPM calculation
static const byte RATE_SIZE = 4;
static byte rates[RATE_SIZE];
static byte rateSpot = 0;
static long lastBeat = 0;

// Add these two variables for better detection
static bool beatDetected = false;
static float lastValidBPM = 0;

void HeartRateSensor_Init(void) {
    I2C_Begin(21, 22, 100000);
    
    Serial.println("Initializing MAX30102...");
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        Serial.println("❌ MAX30102 NOT FOUND");
        hrInitialized = false;
        return;
    }
    
    particleSensor.setup(); // default settings
    Serial.println("✅ MAX30102 FOUND");
    
    // Clear the sensor (important!)
    particleSensor.clearFIFO();
    hrInitialized = true;
}

void HeartRateSensor_Update(void) {
    if (!hrInitialized) return;
    
    // Static variables for heart rate calculation
    static long lastIR = 0;
    static float bpmSamples[10] = {0};
    static int sampleIndex = 0;
    static int validSamples = 0;
    static bool lookingForPeak = true;
    static unsigned long lastPeakTime = 0;
    static long lastDelta = 0;
    static int noFingerCounter = 0;
    
    irValue = particleSensor.getIR();
    
    // Check if finger is present
    if (irValue > 50000) {
        contactQuality = 100;
        noFingerCounter = 0;  // Reset no-finger counter
        
        // Calculate IR signal derivative (rate of change)
        long irDelta = irValue - lastIR;
        lastIR = irValue;
        
        // Detect peak: when slope changes from positive to negative
        if (lastDelta > 0 && irDelta <= 0) {
            // This is a peak!
            unsigned long currentTime = millis();
            
            if (lookingForPeak) {
                // First peak, just store the time
                lastPeakTime = currentTime;
                lookingForPeak = false;
            } else {
                // Calculate time between peaks (in milliseconds)
                unsigned long beatInterval = currentTime - lastPeakTime;
                
                // Check if interval is reasonable (40-180 BPM = 333-1500 ms)
                if (beatInterval >= 333 && beatInterval <= 1500) {
                    // Calculate BPM
                    float currentBPM = 60000.0 / beatInterval;
                    
                    // Filter unrealistic BPM values
                    if (currentBPM >= 40.0 && currentBPM <= 180.0) {
                        // Add to rolling average buffer
                        bpmSamples[sampleIndex] = currentBPM;
                        sampleIndex = (sampleIndex + 1) % 10;
                        if (validSamples < 10) validSamples++;
                        
                        // Calculate average of last N samples
                        float sum = 0;
                        int count = 0;
                        for (int i = 0; i < 10; i++) {
                            if (bpmSamples[i] > 0) {
                                sum += bpmSamples[i];
                                count++;
                            }
                        }
                        
                        if (count >= 3) {  // Need at least 3 samples for accuracy
                            beatAvg = sum / count;
                            beatsPerMinute = currentBPM;
                        }
                    }
                }
                
                // Update for next peak detection
                lastPeakTime = currentTime;
            }
        }
        
        lastDelta = irDelta;
        
    } else {
        // No finger detected
        contactQuality = 0;
        
        // Increment no-finger counter
        noFingerCounter++;
        
        // Reset values after 10 consecutive readings with no finger
        if (noFingerCounter > 10) {
            beatAvg = 0;
            beatsPerMinute = 0;
            validSamples = 0;
            sampleIndex = 0;
            memset(bpmSamples, 0, sizeof(bpmSamples));
            lookingForPeak = true;
            noFingerCounter = 0;
        }
    }
}

// FIX THIS FUNCTION - Make it return beatAvg not 0
float HeartRateSensor_ReadAvgBPM(void) {
    return beatAvg;  // Changed from return 0;
}

// Add this function if it doesn't exist
float HeartRateSensor_ReadBPM(void) {
    return beatsPerMinute;
}

long HeartRateSensor_ReadIR(void) { 
    return irValue; 
}

int HeartRateSensor_ReadContactQuality(void) { 
    // Simple mapping: if IR > 50000, contact is good
    if (irValue > 50000) return 100;
    if (irValue > 10000) return 50;
    return 0;
}

float HeartRateSensor_ReadSpO2(void) { 
    return 0; 
}

bool HeartRateSensor_IsInitialized(void) { 
    return hrInitialized; 
}



