# Medical Wearable Health Monitoring System: End-to-End Development

## Overview
This project details the development of a smart wearable health monitoring system capable of detecting critical medical events: normal activity, falls, seizures, and general medical emergencies. The entire pipeline, from data acquisition and preprocessing to model training, evaluation, and deployment, is unified within a single Colab notebook. The ultimate goal is to deploy this intelligent system on resource-constrained edge devices like the ESP32 using TensorFlow Lite Micro, providing real-time health insights and proactive alerting.

## Project Journey: From Data to Deployment

### 1. Data Acquisition and Exploration

#### Data Sources & Configuration:
Our system utilizes a blend of real-world and clinically validated synthetic data to ensure robust and medically accurate event detection:

*   **PAMAP2 Physical Activity Monitoring Dataset (Real Data)**:
    *   **Source**: [UCI Machine Learning Repository](https://archive.ics.uci.edu/ml/datasets/PAMAP2+Physical+Activity+Monitoring)
    *   **Description**: Contains heart rate, temperature, and 3D accelerometer data from 9 subjects performing 18 physical activities.
    *   **Usage**: Primarily used for `Normal` activity patterns, providing realistic baselines for heart rate, temperature, and accelerometer data. This helps the model understand typical human movement and physiological states.

*   **Clinically Validated Medical Patterns (Synthetic Data)**:
    *   **Source**: Medical literature, clinical guidelines (e.g., SisFall research for falls, epilepsy literature for seizures, AHA/WHO guidelines for vital signs).
    *   **Description**: Since real-world medical emergency data is rare and privacy-sensitive, we generated synthetic data for `Fall Detection`, `Seizure`, and `Medical Emergency` classes. These patterns strictly adhere to published clinical ranges and characteristics for each event type.
    *   **Usage**: Crucial for training the model to recognize critical events with high sensitivity, as they are based on established medical knowledge.

*   **Sensor Specifications**: Data ranges are designed to mimic commercial medical-grade wearables (e.g., accelerometer ±16g, HR 40-200 bpm, Temp 35-42°C).

#### Key Discoveries from Data Exploration (Graphs & Analysis):
Through extensive visualization and statistical analysis, we discovered distinct patterns separating the different health states:

*   **Class Imbalance**: The dataset was intentionally imbalanced (70% Normal, 10% each for Fall, Seizure, Emergency) to reflect real-world scenarios where critical events are rarer.
*   **Fall Events**: Characterized by sudden, high-magnitude accelerations (typically >15 m/s²) with relatively normal heart rates and temperatures. The box plots and scatter plots clearly showed these high acceleration spikes.
*   **Seizure Events**: Exhibited high heart rates (tachycardia, >120 bpm) coupled with significant and erratic acceleration magnitudes (violent movements). The correlation heatmap showed strong positive correlations between heart rate and acceleration for this class.
*   **Medical Emergencies (e.g., Fever/Sepsis)**: Distinguished by elevated temperatures (>38.5°C) and compensatory tachycardia (>110 bpm) but with minimal movement (low acceleration magnitude). Visualizations confirmed high temperature and HR values in this class, contrasting with low acceleration.
*   **Normal Activity**: Showed heart rates within typical ranges (60-100 bpm), stable temperatures (36-37.5°C), and moderate, non-violent acceleration patterns.
*   **Clinical Validation**: Our statistical summary and per-class analysis confirmed that the simulated data for each class aligned remarkably well with established clinical ranges and definitions for each event, enhancing the dataset's credibility.

### 2. Data Preprocessing & Cleaning

We ensured data quality and prepared it for model training:
*   **Missing Values & Duplicates**: Checked and confirmed no missing values or duplicate rows in the final dataset.
*   **Invalid Values**: Ensured all sensor readings (HR, Temp, Accel) were within plausible physiological ranges. 
*   **Outlier Analysis**: Identified outliers but chose to retain them as they often represent legitimate critical events (e.g., high acceleration during a fall is an outlier in normal activity but crucial for detection).
*   **Standard Scaling**: Applied `StandardScaler` to normalize all raw sensor features (accel_x, accel_y, accel_z, accel_magnitude, heart_rate, temperature) to a mean of 0 and standard deviation of 1. This is critical for neural network convergence.
*   **Train/Validation/Test Split**: Split the data into 60% training, 20% validation, and 20% testing sets using `stratify=y` to maintain the original class distribution in all subsets, which is vital for imbalanced datasets.

### 3. Feature Engineering

This was a crucial step to enhance the model's ability to discriminate between complex medical events:
*   **Creation of 15 Advanced Medical Features**: Derived new features based on medical intuition and sensor physics, including:
    *   `accel_energy`, `accel_vertical_ratio`, `accel_deviation_gravity`
    *   `hr_zone`, `hr_severity`, `temp_deviation`, `temp_fever`
    *   **Indicator Features**: `fall_indicator`, `seizure_indicator`, `emergency_indicator`, `normal_indicator` (binary flags based on clinical thresholds).
    *   **Interaction Features**: `hr_temp_product`, `movement_hr_ratio`, `temp_movement_ratio`.
    *   **Physiological Stress Metric**: `physiological_stress` (a composite score combining deviations in HR, Temp, and Accel from normal baselines).
*   **Feature Importance Analysis (Graphs)**: Used Random Forest importance and Mutual Information scores to rank the features. `physiological_stress`, `heart_rate`, `hr_temp_product`, `accel_energy`, and `accel_vertical_ratio` consistently ranked highest, confirming their relevance. The visualizations of feature importance clearly highlighted the most discriminative features.
*   **Feature Selection**: Selected the top 15 most impactful features based on a combined score of Random Forest and Mutual Information, reducing dimensionality while retaining predictive power.
*   **Final Scaling**: Applied a second `StandardScaler` to the *combined* set of original (scaled) and engineered features to ensure consistency before feeding them into the neural network.
*   **Class Separability Analysis (Graphs)**: Histograms and statistics by class showed that the engineered features, particularly `physiological_stress`, `heart_rate`, and `accel_energy`, significantly improved the separation between the different health classes, making the model's task easier.

### 4. Model Training

We designed and trained a Deep Neural Network (DNN) optimized for this multi-class medical classification problem:
*   **DNN Architecture**: A sequential Keras model with multiple dense layers (128, 64, 32, 16 neurons), `ReLU` activation, `BatchNormalization` for stable training, and `Dropout` (0.2-0.3) for regularization to prevent overfitting.
*   **Class Imbalance Handling (Performance Improvement)**: Employed **class weighting** during training, significantly boosting the weights for minority (Fall, Seizure, Medical Emergency) classes. We further **amplified these weights for critical classes (Fall, Seizure, Emergency by 1.5x to 2.0x)** to prioritize recall for these life-threatening events. This was crucial for achieving high detection rates for rare but important events.
*   **Custom Metrics**: Implemented custom Keras `Recall` metrics for each critical class (`recall_class_1`, `recall_class_2`, `recall_class_3`) to monitor their performance specifically during training.
*   **Training Strategy**: Trained for 100 epochs with `Adam` optimizer (initial learning rate 0.001), `EarlyStopping` (patience=15) to prevent overfitting, `ReduceLROnPlateau` (patience=5) to adaptively adjust the learning rate, and `ModelCheckpoint` to save the best model based on validation recall.

### 5. Model Evaluation and Deployment

#### Model Performance (Metrics & Graphs):
The trained DNN achieved excellent performance on the unseen test set, demonstrating the effectiveness of the data generation, preprocessing, and feature engineering steps:
*   **Overall Performance**:
    *   Accuracy: **99.80%**
    *   Macro-Avg Recall: **99.61%**
    *   Weighted-Avg Recall: **99.80%**
*   **Critical Class Performance (Performance Improvement)**:
    *   **Fall Detection**: Recall: **99.50%**, Precision: **99.00%**, F1-Score: **99.25%**, AUC: **1.0000**
    *   **Seizure**: Recall: **99.00%**, Precision: **99.00%**, F1-Score: **99.00%**, AUC: **1.0000**
    *   **Medical Emergency**: Recall: **100.00%**, Precision: **100.00%**, F1-Score: **100.00%**, AUC: **1.0000**
    *   The boosted class weights successfully led to very high recall for critical events, ensuring minimal false negatives for these crucial detections.
*   **Confusion Matrix (Graphs)**: The confusion matrices (absolute counts and normalized percentages) showed minimal misclassifications, particularly for critical events, with almost perfect separation between classes. Analysis of misclassifications highlighted very few instances where critical events were missed or falsely identified.
*   **ROC Curves & AUC Scores (Graphs)**: All classes exhibited AUC scores very close to 1.0000, indicating excellent discriminative power of the model for each class, as visually confirmed by the ROC curves tightly hugging the top-left corner.
*   **Prediction Confidence Analysis (Graphs)**: The model showed high confidence for correct predictions (mean ~99.94%) and notably lower confidence for the few incorrect predictions (mean ~82.12%), suggesting that confidence scores could be used for further alert refinement.

#### Deployment Artifacts & Real-time Prediction:

*   **TFLite Model**: The Keras model was successfully converted to `medical_wearable_model.tflite` for efficient edge deployment.
*   **Deployment Function (`predict_health_status`)**: A Python function was created and saved (`predict_function.pkl`) to encapsulate the entire prediction pipeline (loading artifacts, preprocessing, inference) for easy integration into high-level applications.
*   **C++ Preprocessing & Scaling Logic**: Detailed C++ implementations for `create_medical_features_cpp` and `apply_final_scaling_and_selection_cpp` were provided, ensuring that the critical preprocessing steps can be executed natively on the ESP32.
*   **Arduino Sketch for ESP32**: A complete Arduino sketch was generated, integrating:
    *   TensorFlow Lite Micro library.
    *   The TFLite model (`medical_wearable_model.tflite` embedded via `model_data.h`).
    *   The C++ feature engineering and scaling functions.
    *   Simulated sensor data (for testing purposes, to be replaced by actual sensor inputs).
    *   Real-time inference and serial output of predicted health status, confidence, and alert levels.

## Deployment Instructions (ESP32)

### Prerequisites
*   **Arduino IDE** or **PlatformIO** installed.
*   **ESP32 board package** installed.
*   **Arduino_TensorFlowLite library** installed.
*   **`xxd` utility** (Linux/macOS, or Git Bash on Windows).

### Step 1: Generate `model_data.h`
1.  **Download `medical_wearable_model.tflite`** from the Colab environment.
2.  **Open terminal/command prompt** in the directory containing the `.tflite` file.
3.  **Execute**: `xxd -i medical_wearable_model.tflite > model_data.h`
4.  **Move `model_data.h`** into your Arduino sketch folder.

### Step 2: Prepare Arduino Sketch
1.  **Copy the provided Arduino sketch code** into your `.ino` file (already detailed in the notebook).
2.  **Adjust `kTensorArenaSize`** in the sketch if needed (e.g., `10 * 1024` bytes) to prevent memory errors, monitoring `interpreter->arena_used_bytes()` for optimal sizing.

### Step 3: Compile and Upload
1.  **Select your ESP32 board** and COM port in your IDE.
2.  **Compile and Upload** the sketch.

### Step 4: Monitor Output
1.  **Open Serial Monitor** (115200 baud).
2.  Observe simulated sensor data, preprocessing, and real-time model predictions.

## Testing the Python Model with Custom Inputs

```python
import pickle

# Load the predict_health_status function
with open('predict_function.pkl', 'rb') as f:
    predict_health_status = pickle.load(f)

# === CHANGE THESE VALUES TO TEST DIFFERENT SCENARIOS ===
accel_x_input = 0.5   # X-axis acceleration (e.g., from an accelerometer sensor)
accel_y_input = 0.3   # Y-axis acceleration
accel_z_input = 9.8   # Z-axis acceleration (around 9.8 m/s^2 for gravity at rest)
heart_rate_input = 70 # Heart rate (beats per minute)
temperature_input = 36.6 # Body temperature (degrees Celsius)
# ========================================================

print("\n" + "="*70)
print("TESTING MODEL WITH CUSTOM INPUTS")
print("="*70)

print("Inputs:")
print(f"  Acceleration: ({accel_x_input:.1f}, {accel_y_input:.1f}, {accel_z_input:.1f}) m/s²")
print(f"  Heart Rate: {heart_rate_input} bpm")
print(f"  Temperature: {temperature_input:.1f}°C")

# Get prediction from the loaded function
result = predict_health_status(
    accel_x_input, accel_y_input, accel_z_input,
    heart_rate_input, temperature_input
)

print("\nPrediction:")
print(f"  Class: {result['class_name']}")
print(f"  Confidence: {result['confidence']:.2%}")
print(f"  Alert Level: {result['alert_level']}")

print("\nAll Probabilities:")
for class_name, prob in result['probabilities'].items():
    print(f"  {class_name}: {prob:.2%}")
print("\n" + "="*70)
```

## Future Enhancements
*   Integrate actual sensor hardware (accelerometer, heart rate, temperature sensors) with the ESP32.
*   Implement wireless communication (e.g., Bluetooth, Wi-Fi) to send alerts or data to a smartphone/cloud service.
*   Develop a mobile application or dashboard to visualize real-time health data and alerts.
*   Explore sleep tracking, stress detection, and other physiological insights.
*   Implement over-the-air (OTA) updates for the ESP32 firmware.
