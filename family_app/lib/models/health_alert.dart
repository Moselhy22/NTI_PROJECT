class HealthAlert {
  final String title;
  final String message;
  final DateTime time;
  final bool isCritical;

  HealthAlert({
    required this.title,
    required this.message,
    required this.time,
    this.isCritical = false,
  });
}