import 'package:flutter/material.dart';
import 'dart:async';
import 'package:http/http.dart' as http;
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'screens/health_screen.dart';
import 'screens/map_screen.dart';
import 'screens/alerts_screen.dart';

class MainWrapper extends StatefulWidget {
  const MainWrapper({super.key});

  @override
  State<MainWrapper> createState() => _MainWrapperState();
}

class _MainWrapperState extends State<MainWrapper> {
  GoogleMapController? _mapController;
  int _currentIndex = 0;
  Timer? _timer;

  final String influxUrl = 'http://44.202.223.0:8086'; 
  final String token = 'my-super-secret-token';        
  final String org = 'nti_org';                       
  final String bucket = 'iot_bucket';                  

  double heartRate = 0.0;
  double temperature = 0.0;
  int mpuStatus = 0;
  LatLng patientLocation = const LatLng(30.0444, 31.2357);

  @override
  void initState() {
    super.initState();
    _timer = Timer.periodic(const Duration(seconds: 5), (timer) => fetchFromInflux());
  }

 Future<void> fetchFromInflux() async {
  await _fetchLocation();
  
  await _fetchVitals();
}

Future<void> _fetchLocation() async {
  final url = Uri.parse('$influxUrl/api/v2/query?org=$org');
  try {
    final response = await http.post(
      url,
      headers: {
        'Authorization': 'Token $token',
        'Content-Type': 'application/vnd.flux',
        'Accept': 'application/csv',
      },
      body: '''
        from(bucket: "$bucket")
          |> range(start: -1h)
          |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
          |> filter(fn: (r) => r["_field"] == "lat" or r["_field"] == "lon")
          |> last()
          |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
      ''',
    );

    if (response.statusCode == 200) {
      List<String> lines = response.body.trim().split('\n');
      for (var line in lines) {
        if (line.contains(',_result') && !line.contains('table')) {
          var dataParts = line.split(',');
          
        
          try {
            double? lat;
            double? lon;

            lat = double.tryParse(dataParts[dataParts.length - 2]); 
            lon = double.tryParse(dataParts[dataParts.length - 1]);

            if (lat != null && lon != null) {
              setState(() {
                patientLocation = LatLng(lat!, lon!);
              });
              _mapController?.animateCamera(CameraUpdate.newLatLng(patientLocation));
            }
          } catch (e) {
            debugPrint("Parsing Error: $e");
          }
        }
      }
    }
  } catch (e) { 
    debugPrint("Connection Error: $e"); 
  }
}

Future<void> _fetchVitals() async {
  final url = Uri.parse('$influxUrl/api/v2/query?org=$org');
  
  const String fluxQuery = '''
    from(bucket: "iot_bucket")
      |> range(start: -1h)
      |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
      |> filter(fn: (r) => r["_field"] == "value")
      |> last()
  ''';

  try {
    final response = await http.post(
      url,
      headers: {
        'Authorization': 'Token $token',
        'Content-Type': 'application/vnd.flux',
        'Accept': 'application/csv',
      },
      body: fluxQuery,
    );

    if (response.statusCode == 200) {
      List<String> lines = response.body.trim().split('\n');
      
      for (var line in lines) {
        if (!line.startsWith(',_result')) continue;
        
        List<String> parts = line.split(',');
        
       
        String topic = parts[5]; 
        double value = double.tryParse(parts[6]) ?? 0.0;

        String lastAlertMessage = "";

        setState(() {
       String? newAlert;

  if (topic.contains("heart/bpm")) {
    heartRate = value;
    if (value > 110) newAlert = "High Heart Rate (${value.toStringAsFixed(1)} bpm)";
  } 
  else if (topic.contains("temp")) {
    temperature = value;
    if (value > 38.5) newAlert = "High Fever (${value.toStringAsFixed(1)} °C)";
  } 
  else if (topic.contains("motion") && value == 1) {
    mpuStatus = 1;
    newAlert = "Fall Detected!";
  }
  else if (topic.contains("ai/score") && value < 0.4) {
    newAlert = "Critical Condition Predicted (Score: ${value.toStringAsFixed(2)})";
  }

  // منع إضافة نفس التنبيه مرتين ورا بعض
  if (newAlert != null && newAlert != lastAlertMessage) {
    AlertsScreen.addAlert(newAlert, patientLocation);
    lastAlertMessage = newAlert; // حفظ آخر تنبيه
  }
});
      }
    }
  } catch (e) {
    debugPrint("Error: $e");
  }
}
  

  void _goToPatientLocation() {
    if (_mapController != null) {
      _mapController!.animateCamera(
        CameraUpdate.newLatLngZoom(patientLocation, 15),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final List<Widget> pages = [
      HealthScreen(heartRate: heartRate, temperature: temperature, mpuStatus: mpuStatus),
      MapScreen(
        location: patientLocation, 
        heartRate: heartRate, 
        mpuStatus: mpuStatus,
        onMapCreated: (controller) => _mapController = controller,
      ),
      const AlertsScreen(),
    ];

    return Scaffold(
      body: IndexedStack(
        index: _currentIndex,
        children: pages,
      ),
      floatingActionButton: _currentIndex == 1 
          ? FloatingActionButton(
              onPressed: _goToPatientLocation,
              backgroundColor: Colors.teal,
              child: const Icon(Icons.my_location),
            )
          : null,
      bottomNavigationBar: BottomNavigationBar(
        currentIndex: _currentIndex,
        onTap: (index) => setState(() => _currentIndex = index),
        selectedItemColor: Colors.teal,
        items: const [
          BottomNavigationBarItem(icon: Icon(Icons.favorite), label: 'Vitals'),
          BottomNavigationBarItem(icon: Icon(Icons.map), label: 'Track'),
          BottomNavigationBarItem(icon: Icon(Icons.history), label: 'Alerts'),
        ],
      ),
    );
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }
}