import 'package:flutter/material.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';

class AlertsScreen extends StatefulWidget {
  const AlertsScreen({super.key});

  // مصفوفة ثابتة لتخزين التنبيهات لكي لا تضيع عند التنقل بين الشاشات
  static List<Map<String, String>> emergencyHistory = [];

  // الدالة يجب أن تكون static لكي يتم استدعاؤها من أي مكان بدون إنشاء نسخة من الكلاس
  static void addAlert(String message, LatLng location) {
    emergencyHistory.insert(0, {
      'time': DateTime.now().toString().substring(11, 16), // عرض الساعة والدقيقة فقط
      'event': message,
      'location': "${location.latitude.toStringAsFixed(4)}, ${location.longitude.toStringAsFixed(4)}"
    });
  }

  @override
  State<AlertsScreen> createState() => _AlertsScreenState();
}

class _AlertsScreenState extends State<AlertsScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Emergency History"),
        backgroundColor: Colors.red.shade50,
      ),
      body: AlertsScreen.emergencyHistory.isEmpty
          ? const Center(child: Text("No alerts yet. System is stable."))
          : ListView.builder(
              itemCount: AlertsScreen.emergencyHistory.length,
              itemBuilder: (context, index) {
                final alert = AlertsScreen.emergencyHistory[index];
                return Card(
                  margin: const EdgeInsets.symmetric(horizontal: 15, vertical: 8),
                  child: ListTile(
                    leading: const CircleAvatar(
                      backgroundColor: Colors.red,
                      child: Icon(Icons.warning, color: Colors.white),
                    ),
                    title: Text(alert['event']!, style: const TextStyle(fontWeight: FontWeight.bold)),
                    subtitle: Text("Time: ${alert['time']} | Loc: ${alert['location']}"),
                  ),
                );
              },
            ),
    );
  }
}