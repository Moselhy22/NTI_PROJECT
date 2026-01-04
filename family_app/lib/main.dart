import 'package:flutter/material.dart';
import 'main_wrapper.dart';
import 'screens/login_screen.dart'; // Import your navigation hub

void main() {
  runApp(const MaterialApp(
    debugShowCheckedModeBanner: false,
    home: LoginScreen(), // Start with Login
  ));
}
class FamilyHealthApp extends StatelessWidget {
  const FamilyHealthApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Family Health Tracker',
      theme: ThemeData(
        primarySwatch: Colors.teal,
        useMaterial3: true, // Gives a modern Android look
      ),
      home: const MainWrapper(), // The app starts with the bottom navigation
    );
  }
}