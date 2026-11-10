
/*
 * ESP32 WiFi Chicken Feeder Controller
 * 
 * Features:
 * - WiFi connectivity with auto-reconnect
 * - HTTP server for remote control
 * - 3 relay control for different feed types (cám, ngô, thóc)
 * - Simultaneous motor control support
 * - Status LED indicator
 * - Safety timeout protection
 * 
 * Hardware:
 * - ESP32 Development Board
 * - 3x Relay modules (active LOW)
 * - 3x DC motors for feeders
 * 
 * Author: Auto-generated for TikTok Live Connector
 * Date: 2024
 */
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ==================== WiFi Configuration ====================
const char* ssid = "The Mett Free";        // WiFi SSID
const char* password = "";             // WiFi Password

// ==================== Pin Configuration ====================
#define RELAY_CAM   25  // GPIO 25 - Relay for Cám (corn feed)
#define RELAY_NGO   26  // GPIO 26 - Relay for Ngô (maize)
#define RELAY_THOC  27  // GPIO 27 - Relay for Thóc (rice)
#define LED_STATUS  2   // GPIO 2  - Built-in LED for status

// ==================== Constants ====================
#define HTTP_PORT 80
#define MAX_DURATION 30000  // Maximum motor run time: 30 seconds
#define MIN_DURATION 100    // Minimum motor run time: 100ms
#define WIFI_RETRY_DELAY 5000
#define LED_BLINK_FAST 100
#define LED_BLINK_SLOW 500

// ==================== Global Objects ====================
WebServer server(HTTP_PORT);

// ==================== Motor Control Structure ====================
struct MotorControl {
  int pin;
  unsigned long startTime;
  unsigned long duration;
  bool isRunning;
  String name;
};

MotorControl motors[3] = {
  {RELAY_CAM, 0, 0, false, "cam"},
  {RELAY_NGO, 0, 0, false, "ngo"},
  {RELAY_THOC, 0, 0, false, "thoc"}
};

// ==================== Setup Function ====================
void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n========================================");
  Serial.println("ESP32 WiFi Chicken Feeder Controller");
  Serial.println("========================================\n");

  // Initialize pins
  initializePins();

  // Connect to WiFi
  connectToWiFi();

  // Setup HTTP server routes
  setupServerRoutes();

  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.printf("Ready to receive commands at http://%s/feed\n", WiFi.localIP().toString().c_str());
  Serial.println("========================================\n");
}

// ==================== Main Loop ====================
void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost! Attempting to reconnect...");
    connectToWiFi();
  }

  // Handle HTTP requests
  server.handleClient();

  // Update motor states (non-blocking)
  updateMotors();

  // Small delay to prevent watchdog issues
  delay(1);
}

// ==================== Pin Initialization ====================
void initializePins() {
  Serial.println("Initializing pins...");
  
  // Configure relay pins as outputs (relays are active LOW)
  pinMode(RELAY_CAM, OUTPUT);
  pinMode(RELAY_NGO, OUTPUT);
  pinMode(RELAY_THOC, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);

  // Turn off all relays (HIGH = OFF for active LOW relays)
  digitalWrite(RELAY_CAM, HIGH);
  digitalWrite(RELAY_NGO, HIGH);
  digitalWrite(RELAY_THOC, HIGH);
  digitalWrite(LED_STATUS, LOW);

  Serial.println("Pins initialized - All relays OFF");
}

// ==================== WiFi Connection ====================
void connectToWiFi() {
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    blinkLED(LED_BLINK_FAST);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    
    // Celebrate with LED blinks
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_STATUS, HIGH);
      delay(100);
      digitalWrite(LED_STATUS, LOW);
      delay(100);
    }
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    Serial.println("Please check your credentials and try again.");
  }
}

// ==================== HTTP Server Routes ====================
void setupServerRoutes() {
  // POST /feed - Main endpoint for feeding control
  server.on("/feed", HTTP_POST, handleFeedRequest);

  // GET / - Root endpoint for status check
  server.on("/", HTTP_GET, handleRoot);

  // GET /status - Get current motor status
  server.on("/status", HTTP_GET, handleStatus);

  // POST /stop - Emergency stop all motors
  server.on("/stop", HTTP_POST, handleStop);

  // 404 handler
  server.onNotFound(handleNotFound);
}

// ==================== HTTP Handlers ====================

// Handle POST /feed
void handleFeedRequest() {
  blinkLED(LED_BLINK_FAST);
  
  Serial.println("\n--- New Feed Request ---");
  
  // Check content type
  if (!server.hasHeader("Content-Type") || 
      server.header("Content-Type") != "application/json") {
    sendJsonResponse(400, false, "Content-Type must be application/json");
    return;
  }

  // Parse JSON body
  String body = server.arg("plain");
  Serial.printf("Received: %s\n", body.c_str());

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    sendJsonResponse(400, false, "Invalid JSON format");
    return;
  }

  // Get duration
  int duration = doc["duration"] | 1500; // Default 1500ms
  
  // Validate duration
  if (duration < MIN_DURATION) {
    duration = MIN_DURATION;
    Serial.printf("Duration too short, set to minimum: %d ms\n", MIN_DURATION);
  }
  if (duration > MAX_DURATION) {
    duration = MAX_DURATION;
    Serial.printf("Duration too long, set to maximum: %d ms\n", MAX_DURATION);
  }

  // Handle single motor
  if (doc.containsKey("motor")) {
    String motorName = doc["motor"].as<String>();
    motorName.toLowerCase();
    
    if (activateMotor(motorName, duration)) {
      String msg = "Motor " + motorName + " activated for " + String(duration) + "ms";
      Serial.println(msg);
      sendJsonResponse(200, true, msg);
    } else {
      sendJsonResponse(400, false, "Invalid motor name. Use: cam, ngo, or thoc");
    }
    return;
  }

  // Handle multiple motors
  if (doc.containsKey("motors")) {
    JsonArray motorsArray = doc["motors"].as<JsonArray>();
    int activatedCount = 0;
    String activatedMotors = "";

    for (JsonVariant motorVariant : motorsArray) {
      String motorName = motorVariant.as<String>();
      motorName.toLowerCase();
      
      if (activateMotor(motorName, duration)) {
        activatedCount++;
        if (activatedMotors.length() > 0) activatedMotors += ", ";
        activatedMotors += motorName;
      }
    }

    if (activatedCount > 0) {
      String msg = "Activated " + String(activatedCount) + " motor(s): " + activatedMotors + " for " + String(duration) + "ms";
      Serial.println(msg);
      sendJsonResponse(200, true, msg);
    } else {
      sendJsonResponse(400, false, "No valid motors specified");
    }
    return;
  }

  sendJsonResponse(400, false, "Missing 'motor' or 'motors' field");
}

// Handle GET /
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>ESP32 Chicken Feeder</title></head><body>";
  html += "<h1>🐔 ESP32 WiFi Chicken Feeder</h1>";
  html += "<p><strong>Status:</strong> Online</p>";
  html += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<h2>Available Endpoints:</h2>";
  html += "<ul>";
  html += "<li>POST /feed - Feed control</li>";
  html += "<li>GET /status - Get motor status</li>";
  html += "<li>POST /stop - Emergency stop</li>";
  html += "</ul>";
  html += "<h3>Example POST /feed:</h3>";
  html += "<pre>{\"motor\": \"cam\", \"duration\": 1500}</pre>";
  html += "<pre>{\"motors\": [\"cam\", \"ngo\", \"thoc\"], \"duration\": 2000}</pre>";
  html += "</body></html>";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

// Handle GET /status
void handleStatus() {
  StaticJsonDocument<512> doc;
  doc["success"] = true;
  doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
  doc["ip_address"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();

  JsonArray motorsArray = doc.createNestedArray("motors");
  
  for (int i = 0; i < 3; i++) {
    JsonObject motor = motorsArray.createNestedObject();
    motor["name"] = motors[i].name;
    motor["pin"] = motors[i].pin;
    motor["running"] = motors[i].isRunning;
    if (motors[i].isRunning) {
      unsigned long elapsed = millis() - motors[i].startTime;
      unsigned long remaining = (elapsed < motors[i].duration) ? (motors[i].duration - elapsed) : 0;
      motor["remaining_ms"] = remaining;
    }
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Handle POST /stop
void handleStop() {
  Serial.println("\n--- EMERGENCY STOP ---");
  stopAllMotors();
  sendJsonResponse(200, true, "All motors stopped");
}

// Handle 404
void handleNotFound() {
  String message = "404 - Endpoint Not Found\n\n";
  message += "URI: " + server.uri() + "\n";
  message += "Method: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";
  server.send(404, "text/plain", message);
}

// ==================== Motor Control Functions ====================

// Activate a specific motor
bool activateMotor(String motorName, int duration) {
  for (int i = 0; i < 3; i++) {
    if (motors[i].name == motorName) {
      motors[i].startTime = millis();
      motors[i].duration = duration;
      motors[i].isRunning = true;
      digitalWrite(motors[i].pin, LOW); // Active LOW relay
      Serial.printf("Motor %s ON (Pin %d) for %d ms\n", 
                    motors[i].name.c_str(), motors[i].pin, duration);
      return true;
    }
  }
  return false;
}

// Update motor states (non-blocking)
void updateMotors() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < 3; i++) {
    if (motors[i].isRunning) {
      // Check if duration has elapsed
      if (currentTime - motors[i].startTime >= motors[i].duration) {
        digitalWrite(motors[i].pin, HIGH); // Turn OFF (active LOW)
        motors[i].isRunning = false;
        Serial.printf("Motor %s OFF (completed)\n", motors[i].name.c_str());
      }
    }
  }
}

// Stop all motors immediately
void stopAllMotors() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(motors[i].pin, HIGH); // Turn OFF
    motors[i].isRunning = false;
  }
  Serial.println("All motors stopped");
}

// ==================== Utility Functions ====================

// Send JSON response
void sendJsonResponse(int statusCode, bool success, String message) {
  StaticJsonDocument<256> doc;
  doc["success"] = success;
  doc["message"] = message;
  doc["timestamp"] = millis();

  String response;
  serializeJson(doc, response);
  server.send(statusCode, "application/json", response);
  
  Serial.printf("Response: %d - %s\n", statusCode, message.c_str());
}

// Blink status LED
void blinkLED(int duration) {
  digitalWrite(LED_STATUS, HIGH);
  delay(duration);
  digitalWrite(LED_STATUS, LOW);
}
