from flask import Flask
import cv2
import os
from datetime import datetime

app = Flask(__name__)

# Create folder if not exists
os.makedirs("attendance_photos", exist_ok=True)

@app.route('/helmet_removed')
def helmet_removed():
    print("[INFO] Helmet removed! Taking photo...")

    # Initialize webcam
    cap = cv2.VideoCapture(0)  # 0 = default camera

    if not cap.isOpened():
        return "ERROR: Could not open camera", 500

    ret, frame = cap.read()
    if ret:
        # Generate filename with timestamp
        filename = f"attendance_photos/worker_{datetime.now().strftime('%Y%m%d_%H%M%S')}.jpg"
        cv2.imwrite(filename, frame)
        print(f"[INFO] Photo saved: {filename}")
        cap.release()
        return f"Photo taken and saved: {filename}", 200
    else:
        cap.release()
        return "ERROR: Failed to capture image", 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)