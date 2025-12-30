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

  final String influxUrl = 'https://us-east-1-1.aws.cloud2.influxdata.com';
  final String token = 'KwYhUgRBf7CnSs8Q06jD_iAEAuYrX14WzjCtaJql-P9k56b0gZbwBnBH8edDkeAIMcjK8Tm-ubCbvS-POY49Sg==';
  final String org = 'c23e135daf014fdd';
  final String bucket = 'patient_data';

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
          |> filter(fn: (r) => r["_measurement"] == "location")
          |> last()
          |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
        ''',
      );

      if (response.statusCode == 200) {
        List<String> lines = response.body.trim().split('\n');
        
        for (var line in lines) {
          if (line.startsWith(',_result')) { 
            var dataParts = line.split(',');

            try {
              double lat = double.parse(dataParts[8].trim()); 
              double lng = double.parse(dataParts[9].trim());

              setState(() {
                patientLocation = LatLng(lat, lng);
              });
              
              _mapController?.animateCamera(
                CameraUpdate.newLatLng(patientLocation),
              );
              
              debugPrint("Cloud Update Success: $lat, $lng");
            } catch (e) {
              debugPrint("Parsing Error: $e");
            }
          }
        }
      } else {
        debugPrint("Cloud Error: ${response.statusCode} - ${response.body}");
      }
    } catch (e) {
      debugPrint("Sync Error: $e");
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