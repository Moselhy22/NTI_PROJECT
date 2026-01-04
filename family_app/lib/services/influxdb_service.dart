import 'package:http/http.dart' as http;

class InfluxDBService {
  final String serverIp;
  final String token;

  InfluxDBService({required this.serverIp, required this.token});

  Future<String?> fetchRawData() async {
    final url = Uri.parse('http://$serverIp:8086/api/v2/query?org=HealthOrg');
    try {
      final response = await http.post(
        url,
        headers: {
          'Authorization': 'Token $token',
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
      if (response.statusCode == 200) return response.body;
    } catch (e) {
      print("Error fetching data: $e");
    }
    return null;
  }
}