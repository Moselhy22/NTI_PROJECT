import 'package:flutter/material.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:http/http.dart' as http;
import 'dart:async';

class TrackingScreen extends StatefulWidget {
  final String serverIp;
  final String token;

  TrackingScreen({required this.serverIp, required this.token});

  @override
  _TrackingScreenState createState() => _TrackingScreenState();
}

class _TrackingScreenState extends State<TrackingScreen> {
  GoogleMapController? _mapController;
  LatLng _patientLocation = LatLng(30.0444, 31.2357); 
  Set<Marker> _markers = {};
  Timer? _timer;
  List<Map<String, String>> emergencyHistory = []; 

  @override
  void initState() {
    super.initState();
    // Auto-fetch every 5 seconds
    _timer = Timer.periodic(Duration(seconds: 5), (timer) => fetchPatientLocation());
  }
  void _checkHealthAlerts(double hr, double temp, int mpu) {
  String message = "";
  
  if (hr > 100) message += "High Heart Rate Detected! ";
  if (temp > 38) message += "High Body Temperature! ";
  if (mpu == 1) message += "Fall Detected! "; // MPU Sensor

  if (message.isNotEmpty) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text("⚠️ EMERGENCY ALERT"),
        content: Text(message),
        actions: [TextButton(onPressed: () => Navigator.pop(context), child: Text("OK"))],
      ),
    );

    _saveEmergencyToHistory(message);
  }
}
void _saveEmergencyToHistory(String alertMessage) {
  setState(() {
    emergencyHistory.add({
      'time': DateTime.now().toString().substring(0, 16), 
      'event': alertMessage,                            
      'location': "${_patientLocation.latitude}, ${_patientLocation.longitude}" 
    });
  });
  print("Saved to history: $alertMessage"); 
}
  Future<void> fetchPatientLocation() async {
    final url = Uri.parse('http://${widget.serverIp}:8086/api/v2/query?org=HealthOrg');
    
    try {
      final response = await http.post(
        url,
        headers: {
          'Authorization': 'Token ${widget.token}',
          'Content-Type': 'application/vnd.flux',
          'Accept': 'application/csv',
        },
        body: '''
          from(bucket: "GPS_Bucket")
            |> range(start: -1h)
            |> filter(fn: (r) => r["_measurement"] == "location")
            |> last()
        ''',
      );

     if (response.statusCode == 200) {
  List<String> lines = response.body.split('\n');
  if (lines.length > 1 && lines[1].isNotEmpty) {
    var dataParts = lines[1].split(',');

    double newLat = double.parse(dataParts[5]); 
    double newLng = double.parse(dataParts[6]);
    
    double heartRate = double.parse(dataParts[7]);
    double temperature = double.parse(dataParts[8]);
    int mpuStatus = int.parse(dataParts[9]);

    setState(() {
      _patientLocation = LatLng(newLat, newLng);
      
      _checkHealthAlerts(heartRate, temperature, mpuStatus);

      _markers = {
        Marker(
          markerId: MarkerId('patient_marker'),
          position: _patientLocation,
          icon: BitmapDescriptor.defaultMarkerWithHue(
            (heartRate > 100 || mpuStatus == 1) ? BitmapDescriptor.hueRed : BitmapDescriptor.hueGreen
          ),
          infoWindow: InfoWindow(title: "Heart Rate: $heartRate BPM"),
        )
      };
    });
    
    _mapController?.animateCamera(CameraUpdate.newLatLng(_patientLocation));
  }
}
    } catch (e) {
      debugPrint("Connection Error: $e");
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("Live Tracking"),
        backgroundColor: Colors.blueAccent,
      ),
      body: GoogleMap(
        initialCameraPosition: CameraPosition(target: _patientLocation, zoom: 15),
        onMapCreated: (controller) => _mapController = controller,
        markers: _markers,
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: fetchPatientLocation,
        child: Icon(Icons.refresh),
      ),
    );
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }
}
