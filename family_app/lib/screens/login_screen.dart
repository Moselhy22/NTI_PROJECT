import 'package:flutter/material.dart';
import 'tracking_screen.dart'; // To navigate to the map

class LoginScreen extends StatefulWidget {
  @override
  _LoginScreenState createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  // Setup the controllers with your specific info
  final TextEditingController _ipController = TextEditingController(text: '10.42.0.141');
  final TextEditingController _tokenController = TextEditingController(text: '9TJ1WTF9VCpY2e9_d9e2c_L9Rl6h-M76tJdo3nmsx7...');

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("Family App - Setup"),
        backgroundColor: Colors.blueAccent,
      ),
      body: Padding(
        padding: EdgeInsets.all(20.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.security, size: 80, color: Colors.blueAccent),
            SizedBox(height: 30),
            
            // Server IP Field
            TextField(
              controller: _ipController,
              decoration: InputDecoration(
                labelText: 'InfluxDB Server IP',
                border: OutlineInputBorder(),
                prefixIcon: Icon(Icons.computer),
              ),
            ),
            SizedBox(height: 20),
            
            // Token Field
            TextField(
              controller: _tokenController,
              decoration: InputDecoration(
                labelText: 'Access Token',
                border: OutlineInputBorder(),
                prefixIcon: Icon(Icons.vpn_key),
              ),
            ),
            SizedBox(height: 30),
            
            // Login Button
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                minimumSize: Size(double.infinity, 50),
                backgroundColor: Colors.blueAccent,
              ),
              onPressed: () {
                // Navigate to TrackingScreen and pass the IP and Token
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => TrackingScreen(
                      serverIp: _ipController.text,
                      token: _tokenController.text,
                    ),
                  ),
                );
              },
              child: Text("Start Tracking", style: TextStyle(color: Colors.white, fontSize: 18)),
            ),
          ],
        ),
      ),
    );
  }
}