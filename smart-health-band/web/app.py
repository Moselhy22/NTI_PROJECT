# /home/moselhy/Desktop/projects/smart-health-band/web/app.py

from flask import Flask, render_template, request, redirect, session, jsonify
import pandas as pd
import os
import secrets
from datetime import datetime

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

# Helper: Load products
def load_products():
    if os.path.exists(PRODUCTS_PATH):
        return pd.read_csv(PRODUCTS_PATH).to_dict('records')
    return []

# Routes
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

        # Check username uniqueness
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

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)