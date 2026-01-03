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

  // Server settings
  final String influxUrl = 'http://44.202.223.0:8086';
  final String token = 'my-super-secret-token';
  final String org = 'nti_org';
  final String bucket = 'iot_bucket';

  // Data variables
  double heartRate = 72.0;
  double temperature = 36.5;
  int mpuStatus = 0;
  LatLng patientLocation = const LatLng(30.0444, 31.2357);
  Set<String> recentAlerts = {};
  
  // For debugging
  String lastUpdateTime = '';

  @override
  void initState() {
    super.initState();
    fetchFromInflux();
    _timer = Timer.periodic(const Duration(seconds: 3), (timer) {
      fetchFromInflux();
    });
  }

  Future<void> fetchFromInflux() async {
    await _fetchLocation();
    await _fetchVitals();
    if (mounted) {
      setState(() {
        lastUpdateTime = TimeOfDay.now().format(context);
      });
    }
  }

  Future<void> _fetchLocation() async {
    final url = Uri.parse('$influxUrl/api/v2/query?org=$org');
    
    // Try multiple queries to find location data
    final queries = [
      // Query 1: Look for lat/lon fields
      '''
      from(bucket: "iot_bucket")
        |> range(start: -2h)
        |> filter(fn: (r) => r["_field"] == "lat" or r["_field"] == "lon")
        |> last()
      ''',
      // Query 2: Look for latitude/longitude fields
      '''
      from(bucket: "iot_bucket")
        |> range(start: -2h)
        |> filter(fn: (r) => r["_field"] == "latitude" or r["_field"] == "longitude")
        |> last()
      ''',
      // Query 3: Look in specific measurement
      '''
      from(bucket: "iot_bucket")
        |> range(start: -2h)
        |> filter(fn: (r) => r["_measurement"] == "location" or r["_measurement"] == "gps")
        |> last()
      ''',
    ];

    for (var query in queries) {
      try {
        final response = await http.post(
          url,
          headers: {
            'Authorization': 'Token $token',
            'Content-Type': 'application/vnd.flux',
            'Accept': 'application/csv',
          },
          body: query,
        ).timeout(const Duration(seconds: 10));

        if (response.statusCode == 200 && response.body.isNotEmpty) {
          debugPrint("\n=== LOCATION RAW RESPONSE ===");
          debugPrint(response.body);
          debugPrint("=========================\n");
          
          Map<String, double> locationData = _parseLocationCSV(response.body);
          
          if (locationData.isNotEmpty && mounted) {
            double? lat = locationData['lat'];
            double? lon = locationData['lon'];
            
            if (lat != null && lon != null) {
              setState(() {
                patientLocation = LatLng(lat, lon);
              });
              debugPrint("✓ Location updated: $patientLocation");
              return; // Success, exit the loop
            }
          }
        }
      } catch (e) {
        debugPrint("Location query error: $e");
      }
    }
    
    debugPrint("⚠ No location data found");
  }

  Map<String, double> _parseLocationCSV(String csv) {
    Map<String, double> result = {};
    final lines = csv.split('\n');
    List<String> headers = [];
    
    for (var line in lines) {
      if (line.trim().isEmpty || line.startsWith('#')) continue;
      
      final parts = line.split(',');
      
      // First line with column names
      if (headers.isEmpty && line.contains('_value')) {
        headers = parts.map((h) => h.trim()).toList();
        debugPrint("Location Headers: $headers");
        continue;
      }
      
      // Skip header line
      if (line.contains('_result') && line.contains('table')) continue;
      
      if (headers.isNotEmpty && parts.length >= headers.length) {
        try {
          int valueIdx = headers.indexOf('_value');
          int fieldIdx = headers.indexOf('_field');
          
          if (valueIdx >= 0 && fieldIdx >= 0 && valueIdx < parts.length && fieldIdx < parts.length) {
            double? value = double.tryParse(parts[valueIdx].trim());
            String field = parts[fieldIdx].trim();
            
            if (value != null && (field == 'lat' || field == 'lon')) {
              result[field] = value;
              debugPrint("Found $field = $value");
            }
          }
        } catch (e) {
          debugPrint("Parse error: $e");
        }
      }
    }
    
    return result;
  }

  Future<void> _fetchVitals() async {
    final url = Uri.parse('$influxUrl/api/v2/query?org=$org');
    const String fluxQuery = '''
      from(bucket: "iot_bucket")
        |> range(start: -2h)
        |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
        |> filter(fn: (r) => r["_field"] == "value")
        |> group(columns: ["topic"])
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
      ).timeout(const Duration(seconds: 10));

      if (response.statusCode == 200 && response.body.isNotEmpty) {
        debugPrint("\n=== VITALS RAW RESPONSE ===");
        debugPrint(response.body);
        debugPrint("========================\n");
        
        Map<String, double> vitalsData = _parseVitalsCSV(response.body);
        
        if (vitalsData.isNotEmpty && mounted) {
          setState(() {
            vitalsData.forEach((topic, value) {
              if (topic.contains('heart')) {
                heartRate = value;
                debugPrint("✓ Heart rate: $heartRate");
              } else if (topic.contains('temp')) {
                temperature = value;
                debugPrint("✓ Temperature: $temperature");
              } else if (topic.contains('motion') || topic.contains('mpu')) {
                mpuStatus = value > 0.5 ? 1 : 0;
                debugPrint("✓ Motion status: $mpuStatus");
              }
              
              _checkAndAddAlert(topic, value);
            });
          });
        }
      } else {
        debugPrint("Vitals response: ${response.statusCode}");
      }
    } catch (e) {
      debugPrint("Vitals Error: $e");
    }
  }

  Map<String, double> _parseVitalsCSV(String csv) {
    Map<String, double> result = {};
    final lines = csv.split('\n');
    List<String> headers = [];
    
    for (var line in lines) {
      if (line.trim().isEmpty || line.startsWith('#')) continue;
      
      final parts = line.split(',');
      
      // First data line is headers
      if (headers.isEmpty && line.contains('_value')) {
        headers = parts.map((h) => h.trim()).toList();
        debugPrint("Vitals Headers: $headers");
        continue;
      }
      
      // Skip if no headers yet or this is the header line itself
      if (headers.isEmpty || (line.contains('_result') && line.contains('table'))) continue;
      
      if (parts.length >= headers.length) {
        try {
          // Find indices
          int valueIdx = headers.indexOf('_value');
          int topicIdx = headers.indexOf('topic');
          
          if (valueIdx >= 0 && topicIdx >= 0 && valueIdx < parts.length && topicIdx < parts.length) {
            double? value = double.tryParse(parts[valueIdx].trim());
            String topic = parts[topicIdx].trim();
            
            if (value != null && topic.isNotEmpty) {
              result[topic] = value;
              debugPrint("Parsed: $topic = $value");
            }
          }
        } catch (e) {
          debugPrint("Parse error on line: $line - Error: $e");
        }
      }
    }
    
    return result;
  }

  void _checkAndAddAlert(String topic, double value) {
    String? alertMessage;
    String alertKey = '';

    if (topic.contains('heart') && value > 110) {
      alertMessage = "⚠️ High Heart Rate (${value.toStringAsFixed(0)} bpm)";
      alertKey = 'heart_${value.toInt()}';
    } else if (topic.contains('temp') && value > 38.5) {
      alertMessage = "🌡️ High Fever (${value.toStringAsFixed(1)} °C)";
      alertKey = 'temp_${value.toInt()}';
    } else if ((topic.contains('motion') || topic.contains('mpu')) && value > 0.5) {
      alertMessage = "🚨 Fall Detected!";
      alertKey = 'fall_${DateTime.now().minute}';
    } else if (topic.contains('ai') && value < 0.4) {
      alertMessage = "⚡ Critical Condition (${value.toStringAsFixed(2)})";
      alertKey = 'ai_${(value * 100).toInt()}';
    }

    if (alertMessage != null && !recentAlerts.contains(alertKey)) {
      AlertsScreen.addAlert(alertMessage, patientLocation);
      recentAlerts.add(alertKey);
      
      Future.delayed(const Duration(seconds: 30), () {
        if (mounted) recentAlerts.remove(alertKey);
      });
    }
  }

  void _moveToPatientLocation() {
    if (_mapController != null) {
      _mapController!.animateCamera(
        CameraUpdate.newLatLngZoom(patientLocation, 17),
      );
      
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Centered on patient location'),
          duration: Duration(seconds: 1),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: IndexedStack(
        index: _currentIndex,
        children: [
          HealthScreen(
            heartRate: heartRate,
            temperature: temperature,
            mpuStatus: mpuStatus,
          ),
          MapScreen(
            location: patientLocation,
            heartRate: heartRate,
            mpuStatus: mpuStatus,
            onMapCreated: (controller) {
              _mapController = controller;
              Future.delayed(const Duration(milliseconds: 1000), () {
                if (mounted) _moveToPatientLocation();
              });
            },
          ),
          const AlertsScreen(),
        ],
      ),
      floatingActionButton: _currentIndex == 1
          ? FloatingActionButton(
              onPressed: _moveToPatientLocation,
              backgroundColor: Colors.teal,
              elevation: 8,
              tooltip: 'Center on Patient',
              child: const Icon(Icons.my_location, color: Colors.white, size: 28),
            )
          : null,
      floatingActionButtonLocation: FloatingActionButtonLocation.endFloat,
      bottomNavigationBar: BottomNavigationBar(
        currentIndex: _currentIndex,
        onTap: (index) => setState(() => _currentIndex = index),
        selectedItemColor: Colors.teal,
        unselectedItemColor: Colors.grey,
        type: BottomNavigationBarType.fixed,
        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.favorite),
            label: 'Vitals',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.map),
            label: 'Track',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.notifications_active),
            label: 'Alerts',
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    _timer?.cancel();
    _mapController?.dispose();
    super.dispose();
  }
}