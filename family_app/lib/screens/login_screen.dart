import 'package:flutter/material.dart';
import '../main_wrapper.dart'; // Import the new navigation hub

class LoginScreen extends StatefulWidget {
  const LoginScreen({super.key});

  @override
  _LoginScreenState createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  // Static configuration (Professional tip: Move these to a config file later)
  final TextEditingController _ipController = TextEditingController(text: 'Username');
  final TextEditingController _tokenController = TextEditingController(text: 'Password');

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(30.0),
        child: Column(
          children: [
            const SizedBox(height: 80),
            const Icon(Icons.health_and_safety, size: 100, color: Colors.teal),
            const SizedBox(height: 20),
            const Text("Family Health Guard", 
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.teal)),
            const SizedBox(height: 10),
            const Text("Setup your connection to start monitoring", 
              style: TextStyle(color: Colors.grey)),
            const SizedBox(height: 40),
            
            // Server IP Field
            TextField(
              controller: _ipController,
              decoration: InputDecoration(
                labelText: 'InfluxDB Server IP',
                border: OutlineInputBorder(borderRadius: BorderRadius.circular(12)),
                prefixIcon: const Icon(Icons.lan),
              ),
            ),
            const SizedBox(height: 20),
            
            // Access Token Field
            TextField(
              controller: _tokenController,
              decoration: InputDecoration(
                labelText: 'Access Token',
                border: OutlineInputBorder(borderRadius: BorderRadius.circular(12)),
                prefixIcon: const Icon(Icons.lock_open),
              ),
            ),
            const SizedBox(height: 40),
            
            // Login Button - Navigates to the professional dashboard
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                minimumSize: const Size(double.infinity, 55),
                backgroundColor: Colors.teal,
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
              ),
              onPressed: () {
                // Use pushReplacement so the user can't go back to Login without logout
                Navigator.pushReplacement(
                  context,
                  MaterialPageRoute(builder: (context) => const MainWrapper()),
                );
              },
              child: const Text("Connect & Start", 
                style: TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold)),
            ),
          ],
        ),
      ),
    );
  }
}