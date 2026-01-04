import 'package:flutter/material.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';

class MapScreen extends StatelessWidget {
  final LatLng location;
  final double heartRate;
  final int mpuStatus;
  final Function(GoogleMapController) onMapCreated;

  const MapScreen({
    super.key,
    required this.location,
    required this.heartRate,
    required this.mpuStatus,
    required this.onMapCreated,
  });

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: GoogleMap(
        initialCameraPosition: CameraPosition(target: location, zoom: 15),
        onMapCreated: onMapCreated, 
        markers: {
          Marker(
            markerId: const MarkerId('patient_marker'),
            position: location,
            icon: BitmapDescriptor.defaultMarkerWithHue(
              (heartRate > 100 || mpuStatus == 1) 
                  ? BitmapDescriptor.hueRed 
                  : BitmapDescriptor.hueGreen
            ),
            infoWindow: InfoWindow(
              title: "Patient Status",
              snippet: "HR: $heartRate",
            ),
          )
        },
      ),
    );
  }
}