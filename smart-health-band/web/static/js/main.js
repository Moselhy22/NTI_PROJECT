document.addEventListener('DOMContentLoaded', function() {
    // ===== SCROLL ANIMATIONS =====
    const elements = document.querySelectorAll('main > *:not(script):not(style)');
    elements.forEach(el => {
        el.classList.add('animate-on-scroll');
    });

    function checkAnimations() {
        const animatedElements = document.querySelectorAll('.animate-on-scroll');
        animatedElements.forEach(el => {
            const elementTop = el.getBoundingClientRect().top;
            const elementVisible = 150;
            if (elementTop < window.innerHeight - elementVisible) {
                el.classList.add('visible');
            }
        });
    }

    window.addEventListener('scroll', checkAnimations);
    checkAnimations();

    // ===== LANGUAGE DETECTION =====
    const isArabic = document.body.classList.contains('arabic');
    const currentLang = isArabic ? 'ar' : 'en';

    // ===== MULTILINGUAL MESSAGES =====
    const messages = {
        en: {
            welcome: "Hello! How can I help you today?",
            heart_rate: "Your current heart rate is 72 bpm. Normal range is 60-100 bpm. If it's consistently high or low, consult your doctor.",
            spo2: "Your SpO₂ is 98%. Normal is 95-100%. If it drops below 90%, seek medical attention.",
            temperature: "Your body temperature is 36.6°C. Normal is 36.1-37.2°C. If above 38°C, you may have a fever.",
            instructions: "To use your Smart Health Band: 1) Wear it snugly on your wrist. 2) Sync daily via app. 3) Ensure it's charged. 4) View vitals in your dashboard.",
            fall: "If you've fallen, please press the emergency button on your band or call 911 immediately. Your location will be shared with your emergency contacts.",
            appointment: "You can schedule a doctor appointment from your Profile → Calendar tab.",
            not_logged_in: "We provide AI-powered health monitoring with real-time vitals tracking. Register to access personalized health insights!",
            learning: "I'm still learning! In a real system, I'd analyze your vitals and give personalized advice."
        },
        ar: {
            welcome: "مرحباً! كيف يمكنني مساعدتك اليوم؟",
            heart_rate: "معدل ضربات قلبك الحالي هو 72 نبضة/دقيقة. المعدل الطبيعي يتراوح بين 60-100. إذا كان مرتفعاً أو منخفضاً باستمرار، استشر طبيبك.",
            spo2: "تشبع الأكسجين في دمك (SpO₂) هو 98%. الطبيعي يتراوح بين 95-100%. إذا انخفض تحت 90%، اطلب الرعاية الطبية فوراً.",
            temperature: "درجة حرارة جسمك هي 36.6°م. الطبيعي يتراوح بين 36.1-37.2°م. إذا كانت فوق 38°م، فقد تعاني من حمى.",
            instructions: "ل استخدام سوارك الصحي: 1) ارتدِه بإحكام على معصمك. 2) زامنه يومياً عبر التطبيق. 3) تأكد من شحنه. 4) اعرض المؤشرات الحيوية في لوحة تحكمك.",
            fall: "إذا سقطت، اضغط على زر الطوارئ في سوارك أو اتصل بـ 911 فوراً. سيتم مشاركة موقعك مع جهات الاتصال الطارئة.",
            appointment: "يمكنك تحديد موعد مع طبيب من ملفك الشخصي → تبويب التقويم.",
            not_logged_in: "نقدم مراقبة صحية مدعومة بالذكاء الاصطناعي مع تتبع حي للمؤشرات الحيوية. سجّل للحصول على رؤى صحية شخصية!",
            learning: "ما زلت أتعلّم! في النظام الحقيقي، سأحلّل مؤشراتك الحيوية وأقدم نصائح شخصية."
        }
    };

    // ===== VOICE RECOGNITION =====
    let recognition;
    const speechLang = isArabic ? 'ar-EG' : 'en-US';
    
    if ('webkitSpeechRecognition' in window || 'SpeechRecognition' in window) {
        const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
        recognition = new SpeechRecognition();
        recognition.lang = speechLang;
        recognition.interimResults = false;
        recognition.maxAlternatives = 1;
    }

    // ===== CHATBOT FUNCTIONALITY =====
    const chatToggle = document.getElementById('chatbot-toggle');
    const chatWindow = document.getElementById('chatbot-window');
    const chatClose = document.getElementById('chatbot-close');
    const chatInput = document.getElementById('chat-input');
    const chatSend = document.getElementById('chat-send');
    const chatMessages = document.getElementById('chat-messages');
    const micBtn = document.getElementById('mic-btn');

    // Initialize with welcome message
    if (chatMessages) {
        const welcome = document.createElement('div');
        welcome.style.alignSelf = 'flex-start';
        welcome.style.background = '#e2e8f0';
        welcome.style.padding = '10px 14px';
        welcome.style.borderRadius = '8px';
        welcome.style.maxWidth = '85%';
        welcome.style.fontFamily = isArabic ? "'Tajawal', sans-serif" : "'Inter', sans-serif";
        welcome.style.fontSize = '14px';
        welcome.style.lineHeight = '1.4';
        welcome.style.textAlign = isArabic ? 'right' : 'left';
        welcome.innerHTML = `<strong>${isArabic ? 'المساعد:' : 'Assistant:'}</strong> ${messages[currentLang].welcome}`;
        chatMessages.appendChild(welcome);
    }

    if (chatToggle && chatWindow && chatClose && chatInput && chatSend) {
        chatToggle.addEventListener('click', function() {
            chatWindow.style.display = 'flex';
            chatMessages.scrollTop = chatMessages.scrollHeight;
        });

        chatClose.addEventListener('click', function() {
            chatWindow.style.display = 'none';
        });

        // Mic button functionality
        if (micBtn && recognition) {
            micBtn.onclick = () => {
                // Visual feedback
                micBtn.innerHTML = '🎙️';
                micBtn.style.background = '#3498db';
                micBtn.disabled = true;
                
                recognition.start();
                
                recognition.onend = () => {
                    // Reset mic button
                    setTimeout(() => {
                        micBtn.innerHTML = '🎤';
                        micBtn.style.background = '#94a3b8';
                        micBtn.disabled = false;
                    }, 500);
                };
                
                recognition.onresult = (event) => {
                    const transcript = event.results[0][0].transcript;
                    chatInput.value = transcript;
                };
                
                recognition.onerror = (event) => {
                    console.error('Speech recognition error:', event.error);
                    micBtn.innerHTML = '🎤';
                    micBtn.style.background = '#94a3b8';
                    micBtn.disabled = false;
                };
            };
        }

        // ===== ENHANCED CHATBOT WITH REAL-TIME VITALS FROM DASHBOARD =====
        function sendMessage() {
            const message = chatInput.value.trim();
            if (message) {
                // Add user message
                const userMsg = document.createElement('div');
                userMsg.style.alignSelf = 'flex-end';
                userMsg.style.background = '#dbeafe';
                userMsg.style.color = '#1e40af';
                userMsg.style.padding = '10px 14px';
                userMsg.style.borderRadius = '8px';
                userMsg.style.maxWidth = '85%';
                userMsg.style.fontFamily = isArabic ? "'Tajawal', sans-serif" : "'Inter', sans-serif";
                userMsg.style.fontSize = '14px';
                userMsg.style.lineHeight = '1.4';
                userMsg.style.textAlign = isArabic ? 'right' : 'left';
                userMsg.innerHTML = `<strong>${isArabic ? 'أنت:' : 'You:'}</strong> ${message}`;
                chatMessages.appendChild(userMsg);

                chatInput.value = '';
                chatMessages.scrollTop = chatMessages.scrollHeight;

                // Check if user is logged in
                const isLoggedIn = document.querySelector('[data-logged-in="true"]') !== null;

                // For guests, use original static response
                if (!isLoggedIn) {
                    setTimeout(() => {
                        const response = messages[currentLang].not_logged_in;
                        const assistantMsg = document.createElement('div');
                        assistantMsg.style.alignSelf = 'flex-start';
                        assistantMsg.style.background = '#e2e8f0';
                        assistantMsg.style.padding = '10px 14px';
                        assistantMsg.style.borderRadius = '8px';
                        assistantMsg.style.maxWidth = '85%';
                        assistantMsg.style.fontFamily = isArabic ? "'Tajawal', sans-serif" : "'Inter', sans-serif";
                        assistantMsg.style.fontSize = '14px';
                        assistantMsg.style.lineHeight = '1.4';
                        assistantMsg.style.textAlign = isArabic ? 'right' : 'left';
                        assistantMsg.innerHTML = `<strong>${isArabic ? 'المساعد:' : 'Assistant:'}</strong> ${response}`;
                        chatMessages.appendChild(assistantMsg);
                        chatMessages.scrollTop = chatMessages.scrollHeight;
                    }, 800);
                    return;
                }

                // ===== GET REAL-TIME VITALS FROM DASHBOARD =====
                // Read directly from DOM elements with class names
                const heartRateElement = document.querySelector('.heart-rate-value');
                const spo2Element = document.querySelector('.spo2-value');
                const tempElement = document.querySelector('.temp-value');
                const fallElement = document.querySelector('.fall-status');

                let heartRate = 72;
                let spo2 = 98;
                let temp = 36.6;
                let fall = false;

                if (heartRateElement) {
                    const hrText = heartRateElement.textContent.replace(/\D/g, '');
                    heartRate = parseInt(hrText) || 72;
                }

                if (spo2Element) {
                    const spo2Text = spo2Element.textContent.replace(/\D/g, '');
                    spo2 = parseInt(spo2Text) || 98;
                }

                if (tempElement) {
                    const tempText = tempElement.textContent.replace(/\D/g, '');
                    temp = parseFloat(tempText) || 36.6;
                }

                if (fallElement) {
                    fall = !fallElement.textContent.includes('Normal');
                }

                // Generate DYNAMIC response based on actual dashboard values
                let response = messages[currentLang].learning;
                const lowerMsg = message.toLowerCase();

                if (lowerMsg.includes('heart rate') || lowerMsg.includes('pulse') || lowerMsg.includes('نبض') || lowerMsg.includes('قلب')) {
                    if (heartRate < 60) {
                        response = `Your heart rate is ${heartRate} bpm (bradycardia - below normal). If you feel dizzy, contact your doctor.`;
                        if (isArabic) response = `معدل ضربات قلبك هو ${heartRate} نبضة/دقيقة (بطء القلب - أقل من الطبيعي). إذا شعرت بالدوار، اتصل بطبيبك.`;
                    } else if (heartRate > 100) {
                        response = `Your heart rate is ${heartRate} bpm (tachycardia - above normal). Rest and monitor.`;
                        if (isArabic) response = `معدل ضربات قلبك هو ${heartRate} نبضة/دقيقة (تسارع القلب - أعلى من الطبيعي). استرح وراقب.`;
                    } else {
                        response = `Your heart rate is ${heartRate} bpm (normal range: 60-100 bpm).`;
                        if (isArabic) response = `معدل ضربات قلبك هو ${heartRate} نبضة/دقيقة (المعدل الطبيعي: 60-100 نبضة/دقيقة).`;
                    }
                } else if (lowerMsg.includes('spo2') || lowerMsg.includes('oxygen') || lowerMsg.includes('أكسجين') || lowerMsg.includes('تشبع')) {
                    if (spo2 < 90) {
                        response = `⚠️ CRITICAL: Your SpO₂ is ${spo2}% (normal: 95-100%). Seek emergency help!`;
                        if (isArabic) response = `⚠️ حالة حرجة: تشبع الأكسجين لديك ${spo2}% (الطبيعي: 95-100%). اطلب المساعدة الطارئة!`;
                    } else if (spo2 < 95) {
                        response = `Your SpO₂ is ${spo2}% (slightly low). Monitor closely.`;
                        if (isArabic) response = `تشبع الأكسجين لديك ${spo2}% (منخفض قليلاً). راقب باستمرار.`;
                    } else {
                        response = `Your SpO₂ is ${spo2}% (normal range: 95-100%).`;
                        if (isArabic) response = `تشبع الأكسجين لديك ${spo2}% (المعدل الطبيعي: 95-100%).`;
                    }
                } else if (lowerMsg.includes('temperature') || lowerMsg.includes('fever') || lowerMsg.includes('حرارة') || lowerMsg.includes('حمى')) {
                    if (temp > 38) {
                        response = `🌡️ FEVER ALERT: Your temperature is ${temp}°C. Seek medical help if above 39°C.`;
                        if (isArabic) response = `🌡️ تنبيه حمى: درجة حرارتك ${temp}°م. اطلب المساعدة الطبية إذا تجاوزت 39°م.`;
                    } else if (temp > 37.5) {
                        response = `Your temperature is ${temp}°C (slightly elevated).`;
                        if (isArabic) response = `درجة حرارتك ${temp}°م (مرتفعة قليلاً).`;
                    } else {
                        response = `Your temperature is ${temp}°C (normal range: 36.1-37.2°C).`;
                        if (isArabic) response = `درجة حرارتك ${temp}°م (المعدل الطبيعي: 36.1-37.2°م).`;
                    }
                } else if (lowerMsg.includes('fall') || lowerMsg.includes('accident') || lowerMsg.includes('سقوط') || lowerMsg.includes('حادث')) {
                    if (fall) {
                        response = "🚨 EMERGENCY: Fall detected! Emergency contacts notified with GPS location.";
                        if (isArabic) response = "🚨 طوارئ: تم اكتشاف سقوط! تم إبلاغ جهات الاتصال الطارئة بموقع GPS.";
                    } else {
                        response = messages[currentLang].fall;
                    }
                } else if (lowerMsg.includes('how to use') || lowerMsg.includes('instructions') || lowerMsg.includes('كيف') || lowerMsg.includes('استخدام')) {
                    response = messages[currentLang].instructions;
                } else if (lowerMsg.includes('appointment') || lowerMsg.includes('doctor') || lowerMsg.includes('موعد') || lowerMsg.includes('طبيب')) {
                    response = messages[currentLang].appointment;
                }

                // Add assistant message with DYNAMIC response
                setTimeout(() => {
                    const assistantMsg = document.createElement('div');
                    assistantMsg.style.alignSelf = 'flex-start';
                    assistantMsg.style.background = '#e2e8f0';
                    assistantMsg.style.padding = '10px 14px';
                    assistantMsg.style.borderRadius = '8px';
                    assistantMsg.style.maxWidth = '85%';
                    assistantMsg.style.fontFamily = isArabic ? "'Tajawal', sans-serif" : "'Inter', sans-serif";
                    assistantMsg.style.fontSize = '14px';
                    assistantMsg.style.lineHeight = '1.4';
                    assistantMsg.style.textAlign = isArabic ? 'right' : 'left';
                    assistantMsg.innerHTML = `<strong>${isArabic ? 'المساعد:' : 'Assistant:'}</strong> ${response}`;
                    chatMessages.appendChild(assistantMsg);
                    chatMessages.scrollTop = chatMessages.scrollHeight;
                }, 800);
            }
        }

        chatSend.addEventListener('click', sendMessage);
        chatInput.addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                sendMessage();
            }
        });
    }
});