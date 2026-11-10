/*
 * ESP32 + DRV8833 Motor Driver - TikTok Live Feeder
 * Hardware: ESP32 DevKit V1 + DRV8833 + N20 Motor
 * 
 * Kết nối:
 * - GPIO 18 → IN1 DRV8833
 * - GPIO 19 → IN2 DRV8833
 * - GND ESP32 ↔ GND DRV8833 (Common Ground)
 * - VCC DRV8833 ← 5V External Power (KHÔNG NỐI VÀO ESP32!)
 * 
 * Logic điều khiển motor:
 * - Forward:  IN1=HIGH, IN2=LOW
 * - Backward: IN1=LOW, IN2=HIGH
 * - Stop:     IN1=LOW, IN2=LOW (hoặc cả 2 HIGH)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ==================== Pin Configuration ====================
// Motor 1 (Cám)
#define MOTOR1_IN1   18    // GPIO 18 (D18) → DRV8833 IN1
#define MOTOR1_IN2   19    // GPIO 19 (D19) → DRV8833 IN2

// Motor 2 (Ngô)
#define MOTOR2_IN3   16    // GPIO 16 (D16) → DRV8833 IN3
#define MOTOR2_IN4   17    // GPIO 17 (D17) → DRV8833 IN4

#define LED_STATUS   2     // Built-in LED

// ==================== WiFi Configuration ====================
#define AP_SSID     "ESP32-Feeder"
#define AP_PASSWORD "12345678"

// ==================== Global Variables ====================
WebServer server(80);
Preferences prefs;

String wifiSSID = "";
String wifiPassword = "";
bool wifiConfigured = false;

// Motor states
struct MotorState {
  unsigned long startTime;
  unsigned long duration;
  bool running;
};

MotorState motor1 = {0, 0, false};  // Cám
MotorState motor2 = {0, 0, false};  // Ngô

uint32_t feedCountCam = 0;   // Statistics motor 1
uint32_t feedCountNgo = 0;   // Statistics motor 2

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 + DRV8833 Motor Control");
  Serial.println("TikTok Live Chicken Feeder");
  Serial.println("========================================\n");
  
  // Initialize pins
  pinMode(MOTOR1_IN1, OUTPUT);
  pinMode(MOTOR1_IN2, OUTPUT);
  pinMode(MOTOR2_IN3, OUTPUT);
  pinMode(MOTOR2_IN4, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  
  // Stop all motors initially
  stopAllMotors();
  digitalWrite(LED_STATUS, LOW);
  
  // Load WiFi config
  prefs.begin("feeder", false);
  wifiSSID = prefs.getString("wifi_ssid", "");
  wifiPassword = prefs.getString("wifi_pass", "");
  feedCountCam = prefs.getUInt("feed_count_cam", 0);
  feedCountNgo = prefs.getUInt("feed_count_ngo", 0);
  
  if (wifiSSID.length() > 0) {
    wifiConfigured = true;
    Serial.println("WiFi config found, connecting...");
    connectWiFi();
  } else {
    Serial.println("No WiFi config, starting AP mode...");
    startAPMode();
  }
  
  // Setup HTTP routes
  setupRoutes();
  server.begin();
  
  Serial.println("\n========================================");
  Serial.println("System Ready!");
  Serial.println("========================================\n");
  
  if (wifiConfigured && WiFi.status() == WL_CONNECTED) {
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Admin: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/admin");
  } else {
    Serial.println("AP Mode: Connect to ESP32-Feeder");
    Serial.println("Setup: http://192.168.4.1");
  }
}

// ==================== Main Loop ====================
void loop() {
  server.handleClient();
  
  // Auto-stop motor 1 after duration
  if (motor1.running && (millis() - motor1.startTime >= motor1.duration)) {
    stopMotor1();
    Serial.println("Motor 1 (Cám) auto-stopped");
  }
  
  // Auto-stop motor 2 after duration
  if (motor2.running && (millis() - motor2.startTime >= motor2.duration)) {
    stopMotor2();
    Serial.println("Motor 2 (Ngô) auto-stopped");
  }
  
  // Blink LED when any motor running
  bool anyMotorRunning = motor1.running || motor2.running;
  if (anyMotorRunning) {
    digitalWrite(LED_STATUS, (millis() / 200) % 2);
  } else {
    digitalWrite(LED_STATUS, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
  }
  
  delay(10);
}

// ==================== Motor Control ====================

// Motor 1 (Cám) - IN1/IN2
void runMotor1Forward(unsigned long duration) {
  Serial.printf("Motor 1 (Cám) Forward for %lu ms\n", duration);
  
  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, LOW);
  
  motor1.startTime = millis();
  motor1.duration = duration;
  motor1.running = true;
  
  feedCountCam++;
  prefs.putUInt("feed_count_cam", feedCountCam);
}

void stopMotor1() {
  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, LOW);
  motor1.running = false;
}

// Motor 2 (Ngô) - IN3/IN4
void runMotor2Forward(unsigned long duration) {
  Serial.printf("Motor 2 (Ngô) Forward for %lu ms\n", duration);
  
  digitalWrite(MOTOR2_IN3, HIGH);
  digitalWrite(MOTOR2_IN4, LOW);
  
  motor2.startTime = millis();
  motor2.duration = duration;
  motor2.running = true;
  
  feedCountNgo++;
  prefs.putUInt("feed_count_ngo", feedCountNgo);
}

void stopMotor2() {
  digitalWrite(MOTOR2_IN3, LOW);
  digitalWrite(MOTOR2_IN4, LOW);
  motor2.running = false;
}

void stopAllMotors() {
  stopMotor1();
  stopMotor2();
}

// ==================== WiFi Functions ====================
void connectWiFi() {
  Serial.printf("Connecting to: %s\n", wifiSSID.c_str());
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed, starting AP mode");
    startAPMode();
  }
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("AP Mode started");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

// ==================== HTTP Routes ====================
void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/admin", HTTP_GET, handleAdmin);
  server.on("/feed", HTTP_POST, handleFeed);
  server.on("/feed", HTTP_GET, handleFeed);  // Support GET for TikFinity
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/setup", HTTP_GET, handleSetupPage);
  server.on("/setup", HTTP_POST, handleSetupSave);
  server.onNotFound(handle404);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>ESP32 Feeder</title>";
  html += "<style>body{font-family:Arial;margin:40px;background:#f5f5f5;}";
  html += "h1{color:#667eea;}.card{background:white;padding:20px;border-radius:8px;margin:20px 0;}";
  html += ".btn{background:#667eea;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px;}";
  html += ".btn:hover{opacity:0.9;}</style></head><body>";
  html += "<h1>🐔 ESP32 Motor Feeder</h1>";
  html += "<div class='card'>";
  html += "<h2>Status</h2>";
  html += "<p><strong>Motor 1 (Cám):</strong> " + String(motor1.running ? "Running ⚡" : "Stopped ⏸") + "</p>";
  html += "<p><strong>Motor 2 (Ngô):</strong> " + String(motor2.running ? "Running ⚡" : "Stopped ⏸") + "</p>";
  html += "<p><strong>Feed Count Cám:</strong> " + String(feedCountCam) + "</p>";
  html += "<p><strong>Feed Count Ngô:</strong> " + String(feedCountNgo) + "</p>";
  html += "<p><strong>WiFi:</strong> " + String(WiFi.status() == WL_CONNECTED ? "Connected ✅" : "AP Mode 📡") + "</p>";
  if (WiFi.status() == WL_CONNECTED) {
    html += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
  }
  html += "</div>";
  html += "<div class='card'>";
  html += "<h2>Quick Test</h2>";
  html += "<button class='btn' onclick=\"fetch('/feed?duration=1000')\">Test 1s</button>";
  html += "<button class='btn' onclick=\"fetch('/feed?duration=3000')\">Test 3s</button>";
  html += "<button class='btn' onclick=\"fetch('/stop',{method:'POST'})\">Stop</button>";
  html += "</div>";
  html += "<div class='card'>";
  html += "<a href='/admin' class='btn'>🔧 Admin Panel</a>";
  html += "<a href='/setup' class='btn'>⚙️ WiFi Setup</a>";
  html += "</div>";
  html += "</body></html>";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

void handleAdmin() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>Admin - ESP32 Feeder</title>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;margin:0;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px;}";
  html += ".container{max-width:800px;margin:0 auto;}";
  html += ".header{background:white;padding:20px;border-radius:12px;margin-bottom:20px;box-shadow:0 4px 6px rgba(0,0,0,0.1);}";
  html += "h1{color:#667eea;margin:0;}";
  html += ".card{background:white;padding:20px;border-radius:12px;margin-bottom:15px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += ".stat{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #eee;}";
  html += ".stat:last-child{border-bottom:none;}";
  html += ".stat-label{font-weight:600;color:#666;}";
  html += ".stat-value{color:#333;}";
  html += ".btn{background:#667eea;color:white;padding:12px 24px;border:none;border-radius:8px;cursor:pointer;font-size:16px;margin:5px;width:calc(50% - 10px);display:inline-block;text-align:center;text-decoration:none;}";
  html += ".btn:hover{opacity:0.9;}";
  html += ".btn-danger{background:#ef4444;}";
  html += ".btn-success{background:#10b981;}";
  html += "</style>";
  html += "<script>";
  html += "function testMotor(motor,duration){fetch('/feed?motor='+motor+'&duration='+duration).then(()=>setTimeout(()=>location.reload(),duration+200));}";
  html += "function stopMotor(){fetch('/stop',{method:'POST'}).then(()=>location.reload());}";
  html += "</script>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<div class='header'><h1>🔧 Admin Dashboard</h1></div>";
  
  // Statistics
  html += "<div class='card'>";
  html += "<h2>📊 Statistics</h2>";
  html += "<div class='stat'><span class='stat-label'>Feeds Cám:</span><span class='stat-value'>" + String(feedCountCam) + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Feeds Ngô:</span><span class='stat-value'>" + String(feedCountNgo) + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Motor 1 (Cám):</span><span class='stat-value'>" + String(motor1.running ? "⚡ Running" : "⏸ Stopped") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Motor 2 (Ngô):</span><span class='stat-value'>" + String(motor2.running ? "⚡ Running" : "⏸ Stopped") + "</span></div>";
  html += "<div class='stat'><span class='stat-label'>Uptime:</span><span class='stat-value'>" + String(millis()/1000) + "s</span></div>";
  html += "</div>";
  
  // Motor Control
  html += "<div class='card'>";
  html += "<h2>⚙️ Motor 1 Control (Cám)</h2>";
  html += "<button class='btn btn-success' onclick='testMotor(\"cam\",1000)'>Cám 1s</button>";
  html += "<button class='btn btn-success' onclick='testMotor(\"cam\",3000)'>Cám 3s</button>";
  html += "<button class='btn btn-success' onclick='testMotor(\"cam\",5000)'>Cám 5s</button>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h2>⚙️ Motor 2 Control (Ngô)</h2>";
  html += "<button class='btn btn-success' onclick='testMotor(\"ngo\",1000)'>Ngô 1s</button>";
  html += "<button class='btn btn-success' onclick='testMotor(\"ngo\",3000)'>Ngô 3s</button>";
  html += "<button class='btn btn-success' onclick='testMotor(\"ngo\",5000)'>Ngô 5s</button>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<button class='btn btn-danger' onclick='stopMotor()'>🛑 Emergency Stop All</button>";
  html += "</div>";
  
  // Network Info
  html += "<div class='card'>";
  html += "<h2>🌐 Network Info</h2>";
  if (WiFi.status() == WL_CONNECTED) {
    html += "<div class='stat'><span class='stat-label'>WiFi Status:</span><span class='stat-value'>✅ Connected</span></div>";
    html += "<div class='stat'><span class='stat-label'>SSID:</span><span class='stat-value'>" + WiFi.SSID() + "</span></div>";
    html += "<div class='stat'><span class='stat-label'>IP Address:</span><span class='stat-value'>" + WiFi.localIP().toString() + "</span></div>";
    html += "<div class='stat'><span class='stat-label'>Signal:</span><span class='stat-value'>" + String(WiFi.RSSI()) + " dBm</span></div>";
  } else {
    html += "<div class='stat'><span class='stat-label'>Mode:</span><span class='stat-value'>📡 AP Mode</span></div>";
    html += "<div class='stat'><span class='stat-label'>AP IP:</span><span class='stat-value'>" + WiFi.softAPIP().toString() + "</span></div>";
  }
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<a href='/' class='btn'>← Back Home</a>";
  html += "<a href='/setup' class='btn'>⚙️ WiFi Setup</a>";
  html += "</div>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

void handleFeed() {
  // Support both GET and POST for TikFinity compatibility
  int duration = 1500;  // Default 1.5s
  String motor = "cam";  // Default motor
  
  if (server.hasArg("duration")) {
    duration = server.arg("duration").toInt();
  }
  
  if (server.hasArg("motor")) {
    motor = server.arg("motor");
    motor.toLowerCase();
  }
  
  // Limit duration for safety
  if (duration < 100) duration = 100;
  if (duration > 10000) duration = 10000;
  
  // Select motor
  if (motor == "cam" || motor == "motor1") {
    runMotor1Forward(duration);
  } else if (motor == "ngo" || motor == "motor2") {
    runMotor2Forward(duration);
  } else if (motor == "both" || motor == "all") {
    runMotor1Forward(duration);
    runMotor2Forward(duration);
  } else {
    // Default to motor 1
    runMotor1Forward(duration);
  }
  
  StaticJsonDocument<256> doc;
  doc["success"] = true;
  doc["message"] = "Motor started";
  doc["motor"] = motor;
  doc["duration"] = duration;
  doc["feed_count_cam"] = feedCountCam;
  doc["feed_count_ngo"] = feedCountNgo;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStop() {
  stopAllMotors();
  
  StaticJsonDocument<128> doc;
  doc["success"] = true;
  doc["message"] = "All motors stopped";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStatus() {
  StaticJsonDocument<512> doc;
  doc["success"] = true;
  doc["motor1_running"] = motor1.running;
  doc["motor2_running"] = motor2.running;
  doc["feed_count_cam"] = feedCountCam;
  doc["feed_count_ngo"] = feedCountNgo;
  doc["uptime_seconds"] = millis() / 1000;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  
  if (WiFi.status() == WL_CONNECTED) {
    doc["ip_address"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["ssid"] = WiFi.SSID();
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetupPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>WiFi Setup</title>";
  html += "<style>body{font-family:Arial;max-width:400px;margin:50px auto;padding:20px;}";
  html += "input{width:100%;padding:10px;margin:10px 0;border:1px solid #ddd;border-radius:5px;}";
  html += "button{width:100%;padding:12px;background:#667eea;color:white;border:none;border-radius:5px;cursor:pointer;font-size:16px;}";
  html += "button:hover{opacity:0.9;}</style></head><body>";
  html += "<h2>⚙️ WiFi Setup</h2>";
  html += "<form action='/setup' method='POST'>";
  html += "<input name='ssid' placeholder='WiFi SSID' required>";
  html += "<input name='password' type='password' placeholder='WiFi Password' required>";
  html += "<button type='submit'>Save & Connect</button>";
  html += "</form>";
  html += "<p><a href='/'>← Back</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

void handleSetupSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    wifiSSID = server.arg("ssid");
    wifiPassword = server.arg("password");
    
    prefs.putString("wifi_ssid", wifiSSID);
    prefs.putString("wifi_pass", wifiPassword);
    
    server.send(200, "text/html", "<html><body><h2>Saved! Rebooting...</h2><script>setTimeout(()=>window.location='/',3000)</script></body></html>");
    
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handle404() {
  server.send(404, "text/plain", "404 Not Found");
}
