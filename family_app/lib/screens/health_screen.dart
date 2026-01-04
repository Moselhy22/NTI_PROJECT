import 'package:flutter/material.dart';
import '../widgets/vital_card.dart';

class HealthScreen extends StatelessWidget {
final double heartRate;
  final double temperature;
  final int mpuStatus;

  const HealthScreen({
    super.key, 
    required this.heartRate, 
    required this.temperature, 
    required this.mpuStatus
  });  

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF5F7FA), 
      appBar: AppBar(
        title: const Text("Patient Health Dashboard", style: TextStyle(color: Colors.black)),
        backgroundColor: Colors.transparent,
        elevation: 0,
        centerTitle: true,
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text("Vital Signs", style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
            const SizedBox(height: 15),
            
          GridView.count(
  shrinkWrap: true,
  physics: const NeverScrollableScrollPhysics(),
  crossAxisCount: 2,
  crossAxisSpacing: 12,
  mainAxisSpacing: 12,
  childAspectRatio: 1.1,
  children: [
    VitalCard(
      title: "Heart Rate",
      value: heartRate.toStringAsFixed(0), 
      unit: " bpm",
      icon: Icons.favorite,
      color: (heartRate > 100 || heartRate < 60) ? Colors.red : Colors.green,
    ),
    VitalCard(
      title: "Temperature",
      value: temperature.toStringAsFixed(1), // عرض رقم عشري واحد
      unit: " °C",
      icon: Icons.thermostat,
      color: (temperature > 37.5 || temperature < 35.0) ? Colors.orange : Colors.blue,
    ),
    // يمكنك استغلال الكروت الباقية لعرض حالة النظام أو سكور الـ AI
    VitalCard(
      title: "Movement",
      value: mpuStatus == 1 ? "Moving" : "Still",
      unit: "",
      icon: Icons.directions_run,
      color: Colors.purple,
    ),
    const VitalCard(title: "System", value: "Online", unit: "", icon: Icons.cloud_done, color: Colors.teal),
  ],
),
            
            const SizedBox(height: 25),
            const Text("Movement (MPU6050)", style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
            const SizedBox(height: 15),

         //   _buildMPUCard(x: "0.02", y: "1.10", z: "-0.98"),
            
            const SizedBox(height: 20),
            
            _buildStatusSummaryCard(status: "Stable", lastUpdate: "2 mins ago"),
          ],
        ),
      ),
    );
  }

 Widget _buildMPUCard() {
  bool isFallen = mpuStatus == 1; // نفترض أن 1 تعني اكتشاف سقوط أو حركة مفاجئة
  return Container(
    padding: const EdgeInsets.all(20),
    decoration: BoxDecoration(
      color: isFallen ? Colors.red.shade50 : Colors.white,
      borderRadius: BorderRadius.circular(20),
      border: isFallen ? Border.all(color: Colors.red, width: 2) : null,
      boxShadow: [BoxShadow(color: Colors.black.withOpacity(0.05), blurRadius: 10)],
    ),
    child: Row(
      children: [
        Icon(
          isFallen ? Icons.warning_rounded : Icons.accessibility_new,
          size: 40,
          color: isFallen ? Colors.red : Colors.green,
        ),
        const SizedBox(width: 20),
        Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              isFallen ? "FALL DETECTED!" : "Patient Status",
              style: TextStyle(
                fontSize: 18,
                fontWeight: FontWeight.bold,
                color: isFallen ? Colors.red : Colors.black87,
              ),
            ),
            Text(
              isFallen ? "Emergency alert sent" : "Normal movement pattern",
              style: const TextStyle(color: Colors.grey),
            ),
          ],
        ),
      ],
    ),
  );
}

  Widget _axisIndicator(String label, String value, Color color) {
    return Column(
      children: [
        Text(label, style: const TextStyle(color: Colors.grey, fontSize: 12)),
        const SizedBox(height: 5),
        Text(value, style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: color)),
      ],
    );
  }

  Widget _buildStatusSummaryCard({required String status, required String lastUpdate}) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        gradient: LinearGradient(colors: [Colors.green.shade400, Colors.green.shade700]),
        borderRadius: BorderRadius.circular(15),
      ),
      child: ListTile(
        leading: const CircleAvatar(backgroundColor: Colors.white, child: Icon(Icons.person, color: Colors.green)),
        title: Text("Overall Status: $status", style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        subtitle: Text("Last data sync: $lastUpdate", style: const TextStyle(color: Colors.white70)),
      ),
    );
  }
}