/*
 * ESP32 WiFi Manager + Admin Portal - Chicken Feeder v2.0
 * 
 * Production-ready system với web-based configuration
 * 
 * Features:
 * - WiFi Manager: Setup WiFi qua web interface (AP mode)
 * - Web Admin Panel: Quản lý thiết bị từ xa
 * - Authentication: Secure login với session management
 * - Persistent Storage: Lưu cấu hình vào NVS (Non-Volatile Storage)
 * - Statistics Tracking: Theo dõi số lần xả thóc
 * - Mobile-Friendly UI: Responsive design
 * - OTA Ready: Sẵn sàng cho firmware update
 * 
 * Hardware:
 * - ESP32 Development Board
 * - 3x Relay modules (active LOW, 5V)
 * - 3x DC motors for feeders
 * - Status LED (GPIO 2)
 * 
 * Author: Hoang Hai
 * Version: 2.0.0
 * Date: 2024-11-10
 */

// ==================== Includes ====================
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ==================== Project Headers ====================
#include "config.h"
#include "storage.h"            // Task 2 - Completed
#include "wifi_manager.h"       // Task 3 - Completed
// #include "auth.h"            // Task 5 - Not implemented yet
#include "web_setup.h"          // Task 4 - Completed
// #include "web_admin.h"       // Task 6 - Not implemented yet
#include "motor_control.h"      // Task 1 - Completed

// ==================== Global Objects ====================
WebServer server(HTTP_PORT);
Preferences preferences;
DNSServer dnsServer;            // For captive portal

// ==================== Global Variables ====================
// System info
unsigned long bootTime = 0;
String deviceMAC = "";

// ==================== Setup Function ====================
void setup() {
  // Initialize Serial for debugging
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);
  
  // Print header
  printHeader();
  
  // Initialize hardware
  initializeHardware();
  
  // Task 2 - Initialize storage (Preferences)
  initStorage();
  
  // Task 3 - Initialize WiFi Manager
  // This will either connect to saved WiFi or start AP mode
  initWiFiManager();
  
  // Task 4 - Setup HTTP server routes
  setupServerRoutes();
  
  // If in AP mode, start captive portal (DNS server)
  if (getWiFiState() == WIFI_AP_MODE) {
    dnsServer.start(53, "*", WiFi.softAPIP());
    DEBUG_PRINTLN("DNS Server started for captive portal");
  }
  
  // TEMPORARY: Basic routes for testing
  //setupBasicRoutes();
  
  // Start HTTP server
  server.begin();
  DEBUG_PRINTLN("HTTP server started");
  DEBUG_PRINTF("Ready at: http://%s\n", WiFi.localIP().toString().c_str());
  
  // Print footer
  printFooter();
  
  bootTime = millis();
}

// ==================== Main Loop ====================
void loop() {
  // Task 3 - WiFi connection monitoring and auto-reconnect
  checkWiFiConnection();
  
  // Task 4 - Handle DNS requests for captive portal
  if (getWiFiState() == WIFI_AP_MODE) {
    dnsServer.processNextRequest();
  }
  
  // Handle HTTP requests
  server.handleClient();
  
  // Update motor states (non-blocking)
  updateMotors();
  
  // TODO: Task 5 - Cleanup expired sessions
  // cleanupExpiredSessions();
  
  // Small delay to prevent watchdog issues
  delay(LOOP_DELAY);
}

// ==================== Initialization Functions ====================

/**
 * Initialize all hardware components
 */
void initializeHardware() {
  DEBUG_PRINTLN("Initializing hardware...");
  
  // Initialize LED pin
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);
  
  // Initialize motor control
  initializeMotors();
  
  // Initialize WiFi to get MAC address
  WiFi.mode(WIFI_STA);
  delay(100);
  
  // Get MAC address
  deviceMAC = WiFi.macAddress();
  DEBUG_PRINTF("MAC Address: %s\n", deviceMAC.c_str());
  
  DEBUG_PRINTLN("Hardware initialized");
}

/**
 * Print startup header
 */
void printHeader() {
  Serial.println("\n\n========================================");
  Serial.println("ESP32 WiFi Manager + Admin Portal");
  Serial.println("Chicken Feeder v2.0");
  Serial.println("========================================");
  Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
  Serial.printf("Manufacturer: %s\n", MANUFACTURER);
  Serial.println("========================================\n");
}

/**
 * Print startup footer
 */
void printFooter() {
  Serial.println("\n========================================");
  Serial.println("System Ready!");
  Serial.println("========================================\n");
}

// ==================== HTTP Routes Setup ====================

/**
 * Setup all HTTP server routes
 * Routes differ based on WiFi state (AP mode vs Connected mode)
 */
void setupServerRoutes() {
  DEBUG_PRINTLN("Setting up HTTP routes...");
  
  // Common routes (available in all modes)
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/feed", HTTP_POST, handleFeed);
  server.on("/stop", HTTP_POST, handleStop);
  
  // Setup portal routes (for AP mode, but accessible anytime)
  setupSetupPortalRoutes();
  
  // Root route - different behavior based on WiFi state
  server.on("/", HTTP_GET, []() {
    if (getWiFiState() == WIFI_AP_MODE) {
      // In AP mode: show setup portal
      handleSetupRoot();
    } else {
      // In connected mode: show normal home page
      handleRoot();
    }
  });
  
  // 404 handler
  server.onNotFound(handle404);
  
  DEBUG_PRINTLN("HTTP routes configured");
}

// ==================== HTTP Route Handlers ====================

/**
 * Handle GET /
 * Normal home page (when connected to WiFi)
 */
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>ChickenFeeder v2.0</title>";
  html += "<style>";
  html += "body { font-family: Arial; margin: 40px; background: #f5f5f5; }";
  html += "h1 { color: " + String(COLOR_PRIMARY) + "; }";
  html += ".card { background: white; padding: 20px; border-radius: 8px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += ".status { color: " + String(COLOR_SUCCESS) + "; font-weight: bold; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>🐔 ESP32 WiFi Chicken Feeder v2.0</h1>";
  html += "<div class='card'>";
  html += "<h2>Status</h2>";
  html += "<p><strong>Device:</strong> " + String(DEVICE_NAME) + "</p>";
  html += "<p><strong>Firmware:</strong> " + String(FIRMWARE_VERSION) + "</p>";
  html += "<p><strong>Status:</strong> <span class='status'>Online</span></p>";
  html += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p><strong>Uptime:</strong> " + String((millis() - bootTime) / 1000) + " seconds</p>";
  html += "</div>";
  html += "<div class='card'>";
  html += "<h2>Available Endpoints</h2>";
  html += "<ul>";
  html += "<li>POST /feed - Feed control</li>";
  html += "<li>GET /status - Get motor status</li>";
  html += "<li>POST /stop - Emergency stop</li>";
  html += "</ul>";
  html += "<h3>Example POST /feed:</h3>";
  html += "<pre>{\"motor\": \"cam\", \"duration\": 1500}</pre>";
  html += "<pre>{\"motors\": [\"cam\", \"ngo\", \"thoc\"], \"duration\": 2000}</pre>";
  html += "</div>";
  html += "<div class='card'>";
  html += "<p><em>⚠️ Development Mode - WiFi Manager and Admin Panel coming soon...</em></p>";
  html += "</div>";
  html += "</body></html>";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

/**
 * Handle POST /feed
 */
void handleFeed() {
  DEBUG_PRINTLN("\n--- New Feed Request ---");
  
  // Parse JSON body directly (don't check Content-Type header - ESP32 WebServer doesn't collect it properly)
  if (!server.hasArg("plain") || server.arg("plain").length() == 0) {
    sendJSONResponse(400, false, "Missing request body");
    return;
  }
  
  String body = server.arg("plain");
  DEBUG_PRINTF("Body: %s\n", body.c_str());

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    DEBUG_PRINTF("JSON parse error: %s\n", error.c_str());
    sendJSONResponse(400, false, "Invalid JSON format");
    return;
  }

  // Get duration (default 1500ms)
  int duration = doc["duration"] | 1500;

  // Handle single motor
  if (doc.containsKey("motor")) {
    String motorName = doc["motor"].as<String>();
    
    if (activateMotor(motorName, duration)) {
      // Task 9 - Update statistics
      updateStats(motorName);
      
      sendJSONResponse(200, true, "Motor " + motorName + " activated for " + String(duration) + "ms");
    } else {
      sendJSONResponse(400, false, "Invalid motor name. Use: cam, ngo, or thoc");
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
      
      if (activateMotor(motorName, duration)) {
        activatedCount++;
        if (activatedMotors.length() > 0) activatedMotors += ", ";
        activatedMotors += motorName;
        
        // Task 9 - Update statistics
        updateStats(motorName);
      }
    }

    if (activatedCount > 0) {
      sendJSONResponse(200, true, "Activated " + String(activatedCount) + " motor(s): " + activatedMotors);
    } else {
      sendJSONResponse(400, false, "No valid motors specified");
    }
    return;
  }

  sendJSONResponse(400, false, "Missing 'motor' or 'motors' field");
}

/**
 * Handle GET /status
 */
void handleStatus() {
  StaticJsonDocument<512> doc;
  doc["success"] = true;
  doc["device"] = loadDeviceName();
  doc["firmware"] = FIRMWARE_VERSION;
  doc["wifi_state"] = getWiFiStateString();
  doc["wifi_connected"] = (getWiFiState() == WIFI_CONNECTED);
  
  if (getWiFiState() == WIFI_CONNECTED) {
    doc["ip_address"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["ssid"] = WiFi.SSID();
  } else if (getWiFiState() == WIFI_AP_MODE) {
    doc["ap_mode"] = true;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["ap_ssid"] = getAPSSID();
  }
  
  doc["uptime_seconds"] = (millis() - bootTime) / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  
  // Add statistics
  uint32_t cam, ngo, thoc;
  getStats(cam, ngo, thoc);
  JsonObject stats = doc.createNestedObject("statistics");
  stats["cam"] = cam;
  stats["ngo"] = ngo;
  stats["thoc"] = thoc;
  stats["total"] = cam + ngo + thoc;
  
  // Add motors status
  doc["motors"] = serialized(getMotorsStatusJSON());

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

/**
 * Handle POST /stop
 */
void handleStop() {
  DEBUG_PRINTLN("EMERGENCY STOP requested");
  stopAllMotors();
  sendJSONResponse(200, true, "All motors stopped");
}

/**
 * Handle 404
 */
void handle404() {
  String message = "404 - Endpoint Not Found\n\n";
  message += "URI: " + server.uri() + "\n";
  message += "Method: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";
  server.send(404, "text/plain", message);
}

// ==================== Utility Functions ====================

/**
 * Send JSON response
 */
void sendJSONResponse(int statusCode, bool success, String message) {
  StaticJsonDocument<256> doc;
  doc["success"] = success;
  doc["message"] = message;
  doc["timestamp"] = millis();

  String response;
  serializeJson(doc, response);
  server.send(statusCode, "application/json", response);
  
  DEBUG_PRINTF("Response: %d - %s\n", statusCode, message.c_str());
}
