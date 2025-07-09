#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Replace with your actual WiFi credentials
const char* ssid = "hid50911532-HUAWEI nova Y72S";
const char* password = "nathaniel";

// Kafka REST Proxy configuration
const char* kafkaRestProxy = "http://192.168.43.64:8082";  
const char* kafkaTopic = "model-vitals";

// Timing variables
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000; // Send data every 5 seconds

// Helper function for console formatting
String repeatChar(char c, int count) {
  String result = "";
  for (int i = 0; i < count; i++) {
    result += c;
  }
  return result;
}

// Print details after successful connection
void printWiFiStatus() {
  Serial.println("\n" + repeatChar('=', 40));
  Serial.println("📶 WIFI CONNECTION DETAILS");
  Serial.println(repeatChar('-', 40));
  Serial.printf("✅ Connected to SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("📡 IP Address:       %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("📶 Signal Strength:  %d dBm\n", WiFi.RSSI());
  Serial.println(repeatChar('=', 40) + "\n");
}

// Attempt to connect to WiFi
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
    Serial.println("\n✅ WiFi connected successfully!");
    printWiFiStatus();
  } else {
    Serial.println("\n❌ Failed to connect to WiFi.");
    Serial.println("💡 Please check SSID and password.");
  }
}

// Generate dummy vital signs data
void generateDummyData(float &heartRate, float &systolicBP, float &diastolicBP, 
                      float &temperature, float &bloodGlucose, float &oxygenSaturation) {
  // Generate realistic dummy values with some variation
  heartRate = random(60, 100);
  systolicBP = random(110, 140);
  diastolicBP = random(70, 90);
  temperature = random(360, 380) / 10.0; // 36.0 - 38.0°C
  bloodGlucose = random(80, 120);
  oxygenSaturation = random(95, 100);
}

// Send data to Kafka via REST Proxy
bool sendToKafka(float heartRate, float systolicBP, float diastolicBP, 
                float temperature, float bloodGlucose, float oxygenSaturation) {
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected!");
    return false;
  }
  
  HTTPClient http;
  
  // Create JSON payload
  StaticJsonDocument<300> jsonDoc;
  jsonDoc["heartRate"] = heartRate;
  jsonDoc["systolicBP"] = systolicBP;
  jsonDoc["diastolicBP"] = diastolicBP;
  jsonDoc["temperature"] = temperature;
  jsonDoc["bloodGlucose"] = bloodGlucose;
  jsonDoc["oxygenSaturation"] = oxygenSaturation;
  
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  
  // Create Kafka REST Proxy payload
  StaticJsonDocument<500> kafkaPayload;
  kafkaPayload["records"][0]["value"] = jsonDoc;
  
  String kafkaJsonString;
  serializeJson(kafkaPayload, kafkaJsonString);
  
  // Print data being sent
  Serial.println("\n" + repeatChar('=', 50));
  Serial.println("📤 SENDING VITAL SIGNS DATA TO KAFKA");
  Serial.println(repeatChar('-', 50));
  Serial.printf("💓 Heart Rate:        %.0f bpm\n", heartRate);
  Serial.printf("🩸 Blood Pressure:    %.0f/%.0f mmHg\n", systolicBP, diastolicBP);
  Serial.printf("🌡️ Temperature:       %.1f °C\n", temperature);
  Serial.printf("🍬 Blood Glucose:     %.0f mg/dL\n", bloodGlucose);
  Serial.printf("💨 Oxygen Saturation: %.0f %%\n", oxygenSaturation);
  Serial.println("📡 Topic: " + String(kafkaTopic));
  Serial.println(repeatChar('=', 50));
  
  // Send HTTP POST request to Kafka REST Proxy
  String url = String(kafkaRestProxy) + "/topics/" + String(kafkaTopic);
  http.begin(url);
  http.addHeader("Content-Type", "application/vnd.kafka.json.v2+json");
  
  int httpResponseCode = http.POST(kafkaJsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("✅ Data sent successfully!");
    Serial.println("📨 Response code: " + String(httpResponseCode));
    Serial.println("📄 Response: " + response);
    http.end();
    return true;
  } else {
    Serial.println("❌ Error sending data!");
    Serial.println("🔴 HTTP Error code: " + String(httpResponseCode));
    Serial.println("💡 Make sure Kafka REST Proxy is running on port 8082");
    http.end();
    return false;
  }
}

void setup() {
  delay(1000); // Allow time for serial monitor to start
  Serial.begin(115200);
  
  Serial.println("\n" + repeatChar('=', 50));
  Serial.println("🏥 ESP32-S3 PREGNANCY MONITOR - KAFKA SENDER");
  Serial.println(repeatChar('=', 50));
  Serial.println("📡 Will send dummy vital signs to Kafka topic: " + String(kafkaTopic));
  Serial.println("⏱️ Sending interval: " + String(sendInterval/1000) + " seconds");
  Serial.println(repeatChar('=', 50) + "\n");
  
  connectToWiFi();
  
  // Initialize random seed
  randomSeed(analogRead(0));
}

void loop() {
  // Check if it's time to send data
  if (millis() - lastSendTime >= sendInterval) {
    if (WiFi.status() == WL_CONNECTED) {
      float heartRate, systolicBP, diastolicBP, temperature, bloodGlucose, oxygenSaturation;
      
      // Generate dummy data
      generateDummyData(heartRate, systolicBP, diastolicBP, temperature, bloodGlucose, oxygenSaturation);
      
      // Send to Kafka
      bool success = sendToKafka(heartRate, systolicBP, diastolicBP, temperature, bloodGlucose, oxygenSaturation);
      
      if (success) {
        Serial.println("🟢 Ready for next transmission in " + String(sendInterval/1000) + " seconds...\n");
      } else {
        Serial.println("🔴 Transmission failed. Retrying in " + String(sendInterval/1000) + " seconds...\n");
      }
      
      lastSendTime = millis();
    } else {
      Serial.println("❌ WiFi disconnected! Attempting to reconnect...");
      connectToWiFi();
    }
  }
  
  delay(100); 
}