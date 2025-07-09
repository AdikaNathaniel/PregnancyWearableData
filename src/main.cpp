#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Replace with your actual WiFi credentials
const char* ssid = "hid50911532-HUAWEI nova Y72S";
const char* password = "nathaniel";

// ✅ Replace with your laptop's local IP (NOT localhost)
const char* postUrl = "http://192.168.43.64:3100/api/v1/vitals";

// Local web server
WebServer server(80);

// Vitals data
float heartRate, systolicBP, diastolicBP, temperature, bloodGlucose, oxygenSaturation;
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 5000; // 5 seconds

// Pretty-print helper
String repeatChar(char c, int count) {
  String result = "";
  for (int i = 0; i < count; i++) result += c;
  return result;
}

// Generate dummy vitals
void generateDummyData() {
  heartRate = random(60, 100);
  systolicBP = random(110, 140);
  diastolicBP = random(70, 90);
  temperature = random(360, 380) / 10.0;
  bloodGlucose = random(80, 120);
  oxygenSaturation = random(95, 100);

  Serial.println("\n" + repeatChar('=', 50));
  Serial.println("📤 VITALS UPDATED LOCALLY");
  Serial.printf("💓 Heart Rate:        %.0f bpm\n", heartRate);
  Serial.printf("🩸 Blood Pressure:    %.0f/%.0f mmHg\n", systolicBP, diastolicBP);
  Serial.printf("🌡️ Temperature:       %.1f °C\n", temperature);
  Serial.printf("🍬 Blood Glucose:     %.0f mg/dL\n", bloodGlucose);
  Serial.printf("💨 Oxygen Saturation: %.0f %%\n", oxygenSaturation);
  Serial.println(repeatChar('=', 50));
}

// POST to NestJS backend
void postVitals() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(postUrl);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<512> doc;
    doc["patientId"] = "patient-id-001";
    doc["systolic"] = systolicBP;
    doc["diastolic"] = diastolicBP;
    doc["temperature"] = temperature;
    doc["heartRate"] = heartRate;
    doc["glucose"] = bloodGlucose;
    doc["spo2"] = oxygenSaturation;
    doc["proteinuria"] = 1;  // Dummy value
    doc["severity"] = "moderate";
    doc["rationale"] = "Routine dummy reading";

    String jsonBody;
    serializeJson(doc, jsonBody);

    Serial.println("🌐 Sending POST to backend...");
    Serial.println("📦 JSON Payload:");
    Serial.println(jsonBody);

    int httpResponseCode = http.POST(jsonBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.printf("✅ POST Success! HTTP %d\n", httpResponseCode);
      Serial.println("📥 Response from backend:");
      Serial.println(response);
      Serial.println("🎉 Vitals successfully sent to backend!");
    } else {
      Serial.printf("❌ POST Failed. HTTP Code: %d\n", httpResponseCode);
      Serial.println("🚫 Vitals NOT sent.");
    }

    http.end();
  } else {
    Serial.println("⚠️ WiFi not connected. Skipping POST.");
  }
}

// Serve /vitals for Flutter frontend
void handleVitalsRequest() {
  StaticJsonDocument<300> doc;
  doc["heartRate"] = heartRate;
  doc["systolicBP"] = systolicBP;
  doc["diastolicBP"] = diastolicBP;
  doc["temperature"] = temperature;
  doc["bloodGlucose"] = bloodGlucose;
  doc["oxygenSaturation"] = oxygenSaturation;

  String response;
  serializeJson(doc, response);

  Serial.println("\n🔁 Flutter app requested /vitals");
  Serial.println("📦 Sending JSON data to Flutter:");
  Serial.println(response);

  server.send(200, "application/json", response);
}

// Connect to Wi-Fi
void connectToWiFi() {
  Serial.println("🔌 Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.println("📡 IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi connection failed.");
    Serial.println("💡 Check SSID/password.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n" + repeatChar('=', 50));
  Serial.println("🏥 XIAO S3 PREGNANCY MONITOR - HTTP SERVER");
  Serial.println(repeatChar('=', 50));

  connectToWiFi();

  server.on("/vitals", HTTP_GET, handleVitalsRequest);
  server.begin();
  Serial.println("🚀 Server running. Use /vitals to get dummy vitals from Flutter.");

  randomSeed(analogRead(0));
  generateDummyData();  // Initial vitals
  postVitals();         // First POST
  lastUpdateTime = millis();
}

void loop() {
  server.handleClient();

  if (millis() - lastUpdateTime >= updateInterval) {
    generateDummyData();
    postVitals();
    lastUpdateTime = millis();
  }
}
