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
      backgroundColor: const Color(0xFFF5F7FA), // Light professional background
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
            
            // Grid for Heart Rate, Temperature, and SpO2
            GridView.count(
              shrinkWrap: true, // Required to use GridView inside SingleChildScrollView
              physics: const NeverScrollableScrollPhysics(),
              crossAxisCount: 2,
              crossAxisSpacing: 12,
              mainAxisSpacing: 12,
              childAspectRatio: 1.1,
              children: const [
                VitalCard(title: "Heart Rate", value: "82", unit: " bpm", icon: Icons.favorite, color: Colors.red),
                VitalCard(title: "Temperature", value: "36.8", unit: " °C", icon: Icons.thermostat, color: Colors.orange),
                VitalCard(title: "SpO2", value: "99", unit: " %", icon: Icons.water_drop, color: Colors.blue),
                VitalCard(title: "System Status", value: "Active", unit: "", icon: Icons.wifi, color: Colors.teal),
              ],
            ),
            
            const SizedBox(height: 25),
            const Text("Movement (MPU6050)", style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
            const SizedBox(height: 15),

            // Wide Card for MPU Data (Accelerometer/Gyroscope)
            _buildMPUCard(x: "0.02", y: "1.10", z: "-0.98"),
            
            const SizedBox(height: 20),
            
            // Patient Status Card (Summary)
            _buildStatusSummaryCard(status: "Stable", lastUpdate: "2 mins ago"),
          ],
        ),
      ),
    );
  }

  // Specialized UI for MPU Sensor data
  Widget _buildMPUCard({required String x, required String y, required String z}) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [BoxShadow(color: Colors.black.withOpacity(0.05), blurRadius: 10)],
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              _axisIndicator("X-Axis", x, Colors.purple),
              _axisIndicator("Y-Axis", y, Colors.indigo),
              _axisIndicator("Z-Axis", z, Colors.blueGrey),
            ],
          ),
          const Divider(height: 30),
          const Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.directions_walk, color: Colors.grey),
              SizedBox(width: 8),
              Text("Current Activity: Standing", style: TextStyle(fontWeight: FontWeight.w500)),
            ],
          )
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