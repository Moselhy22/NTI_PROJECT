# /home/moselhy/Desktop/projects/smart-health-band/web/app.py

from flask import Flask, render_template, request, redirect, session, jsonify
import pandas as pd
import os
import secrets
from datetime import datetime, timedelta
import paho.mqtt.client as mqtt
from threading import Lock
import json
import threading
import ssl

app = Flask(__name__)
app.secret_key = secrets.token_hex(16)

# Paths
CSV_PATH = '/home/moselhy/Desktop/projects/smart-health-band/data/registrations.csv'
APPOINTMENTS_PATH = '/home/moselhy/Desktop/projects/smart-health-band/data/appointments.csv'
PRODUCTS_PATH = '/home/moselhy/Desktop/projects/smart-health-band/products/items.csv'

# Ensure directories exist
os.makedirs(os.path.dirname(CSV_PATH), exist_ok=True)
os.makedirs(os.path.dirname(APPOINTMENTS_PATH), exist_ok=True)
os.makedirs(os.path.dirname(PRODUCTS_PATH), exist_ok=True)

# Ensure appointments.csv has correct headers
if not os.path.exists(APPOINTMENTS_PATH):
    pd.DataFrame(columns=[
        'patient_email', 'doctor_email', 'doctor_name', 'visit_date', 'visit_time', 'status', 'created_at'
    ]).to_csv(APPOINTMENTS_PATH, index=False)

# ===== MQTT CONFIGURATION =====
MQTT_BROKER = "c414ec541f134896a3b7cf85ae317db4.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
MQTT_TOPIC = "vitals/#"
MQTT_USERNAME = "Moselhy"
MQTT_PASSWORD = "@Thunderx22"

# Global storage for vitals
vitals_data = {}
vitals_lock = Lock()

# Global storage for historical vitals
vitals_history = {}
MAX_HISTORY_HOURS = 72

# MQTT Callbacks
def on_connect(client, userdata, flags, rc):
    print("✅ Connected to MQTT Broker")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    try:
        username = msg.topic.split('/')[-1]
        data = json.loads(msg.payload.decode())
        
        # Add timestamp
        data_with_time = data.copy()
        data_with_time['timestamp'] = datetime.now().isoformat()
        
        # Initialize user history if needed
        if username not in vitals_history:
            vitals_history[username] = []
        
        # Add new reading
        vitals_history[username].append(data_with_time)
        
        # Clean old data (keep only last MAX_HISTORY_HOURS)
        cutoff_time = datetime.now() - timedelta(hours=MAX_HISTORY_HOURS)
        # Use safe timestamp parsing
        filtered_history = []
        for reading in vitals_history[username]:
            try:
                # Handle both ISO formats
                if '.' in reading['timestamp']:
                    # Has microseconds: 2024-01-01T10:30:00.123456
                    ts = datetime.strptime(reading['timestamp'][:19], '%Y-%m-%dT%H:%M:%S')
                else:
                    # No microseconds: 2024-01-01T10:30:00
                    ts = datetime.strptime(reading['timestamp'], '%Y-%m-%dT%H:%M:%S')
                if ts > cutoff_time:
                    filtered_history.append(reading)
            except (ValueError, KeyError):
                # Skip invalid entries
                continue
        
        vitals_history[username] = filtered_history
        
        # Also update current vitals for real-time display
        with vitals_lock:
            vitals_data[username] = data
            
        print(f"📡 MQTT: {username} → {data}")
    except Exception as e:
        print(f"❌ MQTT Error: {e}")

# Initialize MQTT Client
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

def start_mqtt():
    try:
        mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        mqtt_client.tls_set_context(ssl.create_default_context())
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        print(f"🔌 Connected to HiveMQ: {MQTT_BROKER}")
    except Exception as e:
        print(f"⚠️ MQTT Connection Failed: {e}")
        
# Helper: Load products
def load_products():
    if os.path.exists(PRODUCTS_PATH):
        return pd.read_csv(PRODUCTS_PATH).to_dict('records')
    return []

# ===== AI HEALTH ASSISTANT =====
def ai_health_assistant(message, vitals=None, is_logged_in=False, lang='en'):
    # Language-specific responses
    responses = {
        'en': {
            'emergency_fall': "🚨 EMERGENCY: Fall detected! Emergency contacts have been notified with your GPS location. Press the emergency button on your band for immediate help.",
            'emergency_seizure': "⚠️ SEIZURE ALERT: Your vitals indicate possible seizure activity. Emergency contacts notified. Stay calm and safe.",
            'heart_low': "Your heart rate is {} bpm (bradycardia - below normal 60-100). If you feel dizzy or weak, contact your doctor immediately.",
            'heart_high': "Your heart rate is {} bpm (tachycardia - above normal 60-100). Rest and monitor. If it stays high, seek medical attention.",
            'heart_normal': "Your heart rate is {} bpm (normal range: 60-100 bpm).",
            'spo2_critical': "⚠️ CRITICAL: Your SpO₂ is {}% (normal: 95-100%). This indicates severe oxygen deficiency. Seek emergency medical help immediately!",
            'spo2_low': "Your SpO₂ is {}% (slightly low - normal is 95-100%). Monitor closely and consult your doctor if it drops further.",
            'spo2_normal': "Your SpO₂ is {}% (normal range: 95-100%).",
            'temp_high': "🌡️ FEVER ALERT: Your temperature is {}°C (normal: 36.1-37.2°C). Rest, hydrate, and monitor. If above 39°C, seek medical help.",
            'temp_elevated': "Your temperature is {}°C (slightly elevated). Monitor for fever symptoms.",
            'temp_normal': "Your temperature is {}°C (normal range: 36.1-37.2°C).",
            'seizure_stable': "✅ Current status: STABLE. No seizure indicators detected in your vitals.",
            'seizure_warning': "⚠️ WARNING: Vitals show pre-seizure patterns. Please sit down, stay calm, and avoid triggers.",
            'general_health': "I can analyze your real-time health data. Ask me about your heart rate, oxygen levels, temperature, or emergency status.",
            'not_logged_in': "Register to access personalized health monitoring! I can analyze your real-time vitals and provide AI-powered health insights.",
            'fallback': "I'm your AI health assistant. Ask about your vitals, health status, or emergency procedures."
        },
        'ar': {
            'emergency_fall': "🚨 طوارئ: تم اكتشاف سقوط! تم إبلاغ جهات الاتصال الطارئة بموقعك عبر GPS. اضغط على زر الطوارئ في سوارك للحصول على مساعدة فورية.",
            'emergency_seizure': "⚠️ تنبيه نوبة: تشير مؤشراتك الحيوية إلى نشاط نوبات محتمل. تم إبلاغ جهات الاتصال الطارئة. ابقَ هادئًا وآمنًا.",
            'heart_low': "معدل ضربات قلبك هو {} نبضة/دقيقة (بطء القلب - أقل من المعدل الطبيعي 60-100). إذا شعرت بالدوار أو الضعف، اتصل بطبيبك فورًا.",
            'heart_high': "معدل ضربات قلبك هو {} نبضة/دقيقة (تسارع القلب - أعلى من المعدل الطبيعي  60-100). استرح وراقب. إذا بقي مرتفعًا، اطلب الرعاية الطبية.",
            'heart_normal': "معدل ضربات قلبك هو {} نبضة/دقيقة (المعدل الطبيعي: 60-100 نبضة/دقيقة).",
            'spo2_critical': "⚠️ حالة حرجة: تشبع الأكسجين لديك {}% (الطبيعي: 95-100%). هذا يشير إلى نقص حاد في الأكسجين. اطلب المساعدة الطبية الطارئة فورًا!",
            'spo2_low': "تشبع الأكسجين لديك {}% (منخفض قليلاً - الطبيعي 95-100%). راقب باستمرار واستشر طبيبك إذا انخفض أكثر.",
            'spo2_normal': "تشبع الأكسجين لديك {}% (المعدل الطبيعي: 95-100%).",
            'temp_high': "🌡️ تنبيه حمى: درجة حرارتك {}°م (الطبيعي: 36.1-37.2°م). استرح، ترطب، وراقب. إذا تجاوزت 39°م، اطلب المساعدة الطبية.",
            'temp_elevated': "درجة حرارتك {}°م (مرتفعة قليلاً). راقب لأعراض الحمى.",
            'temp_normal': "درجة حرارتك {}°م (المعدل الطبيعي: 36.1-37.2°م).",
            'seizure_stable': "✅ الحالة الحالية: مستقرة. لا توجد مؤشرات نوبة في مؤشراتك الحيوية.",
            'seizure_warning': "⚠️ تحذير: تُظهر المؤشرات الحيوية أنماطًا قبل النوبة. اجلس، ابقَ هادئًا، وتجنب المحفزات.",
            'general_health': "يمكنني تحليل بياناتك الصحية الحية. اسألني عن معدل ضربات القلب، مستويات الأكسجين، درجة الحرارة، أو حالة الطوارئ.",
            'not_logged_in': "سجّل للحصول على مراقبة صحية شخصية! يمكنني تحليل مؤشراتك الحيوية الحية وتقديم رؤى صحية مدعومة بالذكاء الاصطناعي.",
            'fallback': "أنا مساعدك الصحي الذكي. اسأل عن مؤشراتك الحيوية، حالتك الصحية، أو إجراءات الطوارئ."
        }
    }
    
    r = responses[lang]
    message_lower = message.lower()
    
    # Guest users only get basic info
    if not is_logged_in:
        return r['not_logged_in']
    
    # Default vitals if none provided
    hr = vitals.get('heart_rate', 72) if vitals else 72
    spo2 = vitals.get('spo2', 98) if vitals else 98
    temp = vitals.get('temp', 36.6) if vitals else 36.6
    fall = vitals.get('fall', False) if vitals else False
    
    # Emergency: Fall detected
    if fall or any(word in message_lower for word in ['fall', 'سقوط', 'accident', 'حادث', ' emergency', 'طوارئ']):
        return r['emergency_fall']
    
    # Heart rate analysis
    if any(word in message_lower for word in ['heart', 'قلب', 'pulse', 'نبض', 'rate', 'معدل']):
        if hr < 60:
            return r['heart_low'].format(hr)
        elif hr > 100:
            return r['heart_high'].format(hr)
        else:
            return r['heart_normal'].format(hr)
    
    # SpO2 analysis
    if any(word in message_lower for word in ['oxygen', 'أكسجين', 'spo2', 'تشبع', 'saturation']):
        if spo2 < 90:
            return r['spo2_critical'].format(spo2)
        elif spo2 < 95:
            return r['spo2_low'].format(spo2)
        else:
            return r['spo2_normal'].format(spo2)
    
    # Temperature analysis
    if any(word in message_lower for word in ['temp', 'حرار', 'fever', 'حمى', 'temperature', 'درجة']):
        if temp > 38:
            return r['temp_high'].format(temp)
        elif temp > 37.5:
            return r['temp_elevated'].format(temp)
        else:
            return r['temp_normal'].format(temp)
    
    # Seizure analysis (you can integrate your AI model here later)
    if any(word in message_lower for word in ['seizure', 'صرع', 'epilepsy', 'نوبة', 'convulsion']):
        # For now, use simple logic. Replace with your AI model later!
        if hr > 110 and spo2 < 95:
            return r['emergency_seizure']
        elif hr > 95 and spo2 < 97:
            return r['seizure_warning']
        else:
            return r['seizure_stable']
    
    # General health query
    if any(word in message_lower for word in ['health', 'صحة', 'status', 'حالة', 'vitals', 'مؤشرات', 'monitoring', 'مراقبة']):
        return r['general_health']
    
    # Default response
    return r['fallback']

# Routes (unchanged from your working version)
@app.route('/')
def home():
    return render_template('home.html')

@app.route('/about')
def about():
    return render_template('about.html')

@app.route('/contact')
def contact():
    return render_template('contact.html')

@app.route('/services')
def services():
    return render_template('services.html')

@app.route('/products')
def products():
    products_list = load_products()
    return render_template('products.html', products=products_list)

@app.route('/cart')
def cart():
    return render_template('cart.html')

@app.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == 'POST':
        name = request.form.get('name', '').strip()
        username = request.form.get('username', '').strip()
        email = request.form.get('email', '').strip()
        password = request.form.get('password', '').strip()
        address = request.form.get('address', '').strip()
        country = request.form.get('country', '').strip()
        phone = request.form.get('phone', '').strip()
        emergency_contact = request.form.get('emergency_contact', '').strip()
        medical_conditions = request.form.getlist('medical_conditions')
        other_condition = request.form.get('other_condition', '').strip()
        user_type = request.form.get('user_type', 'patient').strip()

        if not all([name, username, email, password, address, country, phone, emergency_contact]):
            return "<h2 style='color:#e74c3c; text-align:center; margin:2rem;'>❌ All fields are required!</h2><div style='text-align:center;'><a href='/register' style='color:#3498db; text-decoration:underline;'>← Go back</a></div>", 400

        if os.path.exists(CSV_PATH):
            try:
                df = pd.read_csv(CSV_PATH)
                if username in df['username'].values:
                    return "<h2 style='color:#e74c3c; text-align:center; margin:2rem;'>❌ Username already taken!</h2><div style='text-align:center;'><a href='/register' style='color:#3498db; text-decoration:underline;'>← Try another</a></div>", 400
            except:
                pass

        if 'None' in medical_conditions:
            final_medical = 'None'
        else:
            medical_conditions = [mc for mc in medical_conditions if mc != 'None']
            if 'Other' in medical_conditions:
                medical_conditions = [mc for mc in medical_conditions if mc != 'Other']
                if other_condition:
                    medical_conditions.append(f"Other: {other_condition}")
            final_medical = '; '.join(medical_conditions) if medical_conditions else 'None'

        record = {
            'timestamp': datetime.now().isoformat(),
            'name': name,
            'username': username,
            'email': email,
            'password': password,
            'user_type': user_type,
            'address': address,
            'country': country,
            'phone': phone,
            'emergency_contact': emergency_contact,
            'medical_conditions': final_medical
        }

        columns = [
            'timestamp', 'name', 'username', 'email', 'password', 'user_type',
            'address', 'country', 'phone', 'emergency_contact', 'medical_conditions'
        ]
        df = pd.DataFrame([record], columns=columns)

        if os.path.exists(CSV_PATH):
            try:
                existing_df = pd.read_csv(CSV_PATH)
                if list(existing_df.columns) == columns:
                    df.to_csv(CSV_PATH, mode='a', header=False, index=False)
                else:
                    raise ValueError("Header mismatch")
            except:
                df.to_csv(CSV_PATH, index=False)
        else:
            df.to_csv(CSV_PATH, index=False)

        return render_template('thankyou.html')

    return render_template('register.html')

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        login_input = request.form.get('login', '').strip()
        password = request.form.get('password', '').strip()

        if not os.path.exists(CSV_PATH):
            return "<h2>❌ No users registered yet.</h2><a href='/register'>Register first</a>", 401

        try:
            df = pd.read_csv(CSV_PATH, dtype=str, keep_default_na=False, na_values=[])
            for col in df.columns:
                if df[col].dtype == 'object':
                    df[col] = df[col].str.strip()

            matched = df[
                ((df['username'] == login_input) | (df['email'] == login_input)) &
                (df['password'] == password)
            ]

            if not matched.empty:
                user = matched.iloc[0]
                session['logged_in'] = True
                session['name'] = user['name']
                session['username'] = user['username']
                session['email'] = user['email']
                session['user_type'] = user['user_type']
                return redirect('/profile')

            return "<h2 style='color:#ef4444; text-align:center;'>❌ Invalid credentials</h2><div style='text-align:center;'><a href='/login'>← Try again</a></div>", 401

        except Exception as e:
            return f"<h2>💥 Error</h2><p>{e}</p>", 500

    return render_template('login.html')

@app.route('/logout')
def logout():
    session.clear()
    return redirect('/')

@app.route('/profile')
def profile():
    if not session.get('logged_in'):
        return redirect('/login')
    
    appointments = []
    try:
        if os.path.exists(APPOINTMENTS_PATH):
            df = pd.read_csv(APPOINTMENTS_PATH, dtype=str)
            if session['user_type'] == 'patient':
                patient_df = df[df['patient_email'] == session['email']]
                appointments = patient_df.to_dict('records')
            else:
                doctor_df = df[df['doctor_email'] == session['email']]
                appointments = doctor_df.to_dict('records')
    except Exception as e:
        print("Error loading appointments:", e)
    
    return render_template('profile.html', appointments=appointments)

@app.route('/schedule-appointment', methods=['POST'])
def schedule_appointment():
    if not session.get('logged_in') or session.get('user_type') != 'doctor':
        return redirect('/login')
    
    patient_identifier = request.form.get('patient_identifier', '').strip()
    visit_date = request.form.get('visit_date', '').strip()
    visit_time = request.form.get('visit_time', '').strip()

    if not all([patient_identifier, visit_date, visit_time]):
        return "<h2>❌ All fields required</h2><a href='/profile'>← Back</a>", 400

    patient_email = None
    if os.path.exists(CSV_PATH):
        df = pd.read_csv(CSV_PATH)
        patient_row = df[(df['username'] == patient_identifier) | (df['email'] == patient_identifier)]
        if not patient_row.empty:
            patient_email = patient_row.iloc[0]['email']
    
    if not patient_email:
        return "<h2>❌ Patient not found</h2><a href='/profile'>← Back</a>", 404

    record = {
        'patient_email': patient_email,
        'doctor_email': session['email'],
        'doctor_name': session['name'],
        'visit_date': visit_date,
        'visit_time': visit_time,
        'status': 'Confirmed',
        'created_at': datetime.now().isoformat()
    }

    columns = ['patient_email', 'doctor_email', 'doctor_name', 'visit_date', 'visit_time', 'status', 'created_at']
    df = pd.DataFrame([record], columns=columns)
    df.to_csv(APPOINTMENTS_PATH, mode='a', header=False, index=False)

    return "<h2 style='color:#10b981;'>✅ Appointment Scheduled!</h2><div style='margin-top:15px;'><a href='/profile' style='color:#3498db; text-decoration:underline;'>← Back to Dashboard</a></div>"

@app.route('/api/products')
def api_products():
    return jsonify(load_products())

# ===== VITALS API ENDPOINT (REAL-TIME) =====
@app.route('/api/vitals/<username>')
def get_vitals(username):
    with vitals_lock:
        data = vitals_data.get(username, {
            "heart_rate": 72,
            "temp": 36.6,
            "spo2": 98,
            "fall": False
        })
    return jsonify(data)

# ===== VITALS HISTORY API ENDPOINT =====
@app.route('/api/vitals-history/<username>')
def get_vitals_history(username):
    time_range = request.args.get('range', 'day')
    
    if username not in vitals_history or len(vitals_history[username]) == 0:
        return jsonify({
            "timestamps": [],
            "heart_rate": [],
            "spo2": [],
            "temperature": [],
            "count": 0
        })
    
    all_data = vitals_history[username]
    now = datetime.now()
    
    if time_range == 'day':
        cutoff = now - timedelta(hours=24)
    elif time_range == 'week':
        cutoff = now - timedelta(days=7)
    elif time_range == 'month':
        cutoff = now - timedelta(days=30)
    else:
        cutoff = now - timedelta(hours=24)
    
    # Filter data with safe timestamp parsing
    filtered_data = []
    for reading in all_data:
        try:
            if '.' in reading['timestamp']:
                ts = datetime.strptime(reading['timestamp'][:19], '%Y-%m-%dT%H:%M:%S')
            else:
                ts = datetime.strptime(reading['timestamp'], '%Y-%m-%dT%H:%M:%S')
            if ts >= cutoff:
                filtered_data.append(reading)
        except (ValueError, KeyError):
            continue
    
    if len(filtered_data) == 0:
        return jsonify({
            "timestamps": [],
            "heart_rate": [],
            "spo2": [],
            "temperature": [],
            "count": 0
        })
    
    timestamps = [reading['timestamp'] for reading in filtered_data]
    heart_rates = [reading['heart_rate'] for reading in filtered_data]
    spo2_values = [reading['spo2'] for reading in filtered_data]
    temp_values = [reading['temp'] for reading in filtered_data]
    
    return jsonify({
        "timestamps": timestamps,
        "heart_rate": heart_rates,
        "spo2": spo2_values,
        "temperature": temp_values,
        "count": len(filtered_data)
    })

# ===== LANGUAGE SWITCHING =====
@app.route('/set-language', methods=['POST'])
def set_language():
    data = request.get_json()
    lang = data.get('lang', 'en')
    if lang in ['en', 'ar']:
        session['lang'] = lang
    return '', 204

# ===== AI CHAT API ENDPOINT =====
@app.route('/api/chat', methods=['POST'])
def chat():
    data = request.get_json()
    message = data.get('message', '')
    username = data.get('username')
    lang = session.get('lang', 'en')
    
    # Get real-time vitals from MQTT
    vitals = None
    if username:
        with vitals_lock:
            vitals = vitals_data.get(username)
    
    is_logged_in = username is not None
    
    # Generate AI response based on real vitals
    response = ai_health_assistant(message, vitals, is_logged_in, lang)
    
    return jsonify({"response": response})

if __name__ == '__main__':
    start_mqtt()
    app.run(debug=True, host='0.0.0.0', port=5000)