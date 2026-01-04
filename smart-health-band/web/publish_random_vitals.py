import random
import time
import json
import paho.mqtt.client as mqtt
import ssl

# ===== HIVE MQ CLOUD CONFIGURATION =====
HIVE_MQ_HOST = "c414ec541f134896a3b7cf85ae317db4.s1.eu.hivemq.cloud"  # ← REPLACE WITH YOUR HOST
HIVE_MQ_PORT = 8883
HIVE_MQ_USERNAME = "Moselhy"          # ← REPLACE WITH YOUR USERNAME  
HIVE_MQ_PASSWORD = "@Thunderx22"          # ← REPLACE WITH YOUR PASSWORD

# Your username (must match Flask session.username)
USERNAME = "Omar"  # ← CHANGE TO YOUR USERNAME

# MQTT Client Setup
client = mqtt.Client()
client.username_pw_set(HIVE_MQ_USERNAME, HIVE_MQ_PASSWORD)
client.tls_set_context(ssl.create_default_context())

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to HiveMQ Cloud")
        print(f"🚀 Publishing random vitals to: vitals/{USERNAME}")
    else:
        print(f"❌ Connection failed with code {rc}")

def on_publish(client, userdata, mid):
    print(f"📤 Message published (ID: {mid})")

client.on_connect = on_connect
client.on_publish = on_publish

# Connect to HiveMQ
client.connect(HIVE_MQ_HOST, HIVE_MQ_PORT, 60)
client.loop_start()

# Publish random vitals every 5 seconds
try:
    while True:
        # Generate random (but realistic) vitals
        vitals = {
            "heart_rate": random.randint(55, 110),    # Normal: 60-100
            "temp": round(random.uniform(36.0, 38.5), 1),  # Normal: 36.1-37.2
            "spo2": random.randint(92, 100),         # Normal: 95-100  
            "fall": random.random() < 0.05           # 5% chance of fall
        }
        
        # Publish to topic: vitals/omar.m
        topic = f"vitals/{USERNAME}"
        payload = json.dumps(vitals)
        
        result = client.publish(topic, payload, qos=1)
        print(f"📊 Published: {vitals}")
        
        time.sleep(5)  # Update every 5 seconds
        
except KeyboardInterrupt:
    print("\n🛑 Stopping publisher...")
    client.loop_stop()
    client.disconnect()