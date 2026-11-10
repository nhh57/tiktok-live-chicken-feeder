/*
 * WiFi Manager Module
 * 
 * Quản lý kết nối WiFi và Access Point mode
 * 
 * Features:
 * - Auto-connect to saved WiFi
 * - Fallback to AP mode if connection fails
 * - Auto-reconnect on disconnection
 * - WiFi network scanning
 * - LED status indicators
 * - Generated AP SSID based on MAC address
 * 
 * State Machine:
 * DISCONNECTED → CONNECTING → CONNECTED
 *       ↓                         ↓
 *   AP_MODE ←──────────────────────
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "storage.h"

// ==================== WiFi State Enum ====================
enum WiFiState {
  WIFI_DISCONNECTED,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_AP_MODE
};

// ==================== Global Variables ====================
WiFiState currentWiFiState = WIFI_DISCONNECTED;
String apSSID = "";
unsigned long lastConnectionAttempt = 0;
int connectionAttempts = 0;
unsigned long lastLEDToggle = 0;
bool ledState = false;

// ==================== Function Prototypes ====================
void initWiFiManager();
bool connectToWiFi(String ssid, String password);
bool connectToSavedWiFi();
void startAPMode();
void stopAPMode();
String scanNetworksJSON();
void checkWiFiConnection();
void updateLEDIndicator();
String getAPSSID();
WiFiState getWiFiState();
String getWiFiStateString();

// ==================== Function Implementations ====================

/**
 * Initialize WiFi Manager
 * Decides whether to connect to saved WiFi or start AP mode
 */
void initWiFiManager() {
  DEBUG_PRINTLN("Initializing WiFi Manager...");
  
  // Generate AP SSID from MAC address
  apSSID = getAPSSID();
  DEBUG_PRINTF("AP SSID will be: %s\n", apSSID.c_str());
  
  // Check if WiFi credentials are saved
  String ssid, password;
  if (loadWiFiCredentials(ssid, password) && ssid.length() > 0) {
    DEBUG_PRINTLN("Found saved WiFi credentials");
    
    // Try to connect
    if (connectToWiFi(ssid, password)) {
      DEBUG_PRINTLN("WiFi Manager initialized - CONNECTED mode");
      currentWiFiState = WIFI_CONNECTED;
      digitalWrite(LED_STATUS, HIGH); // Solid ON
    } else {
      DEBUG_PRINTLN("Failed to connect - starting AP mode");
      startAPMode();
    }
  } else {
    DEBUG_PRINTLN("No saved WiFi credentials - starting AP mode");
    startAPMode();
  }
}

/**
 * Connect to WiFi network
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return true if connected successfully
 */
bool connectToWiFi(String ssid, String password) {
  DEBUG_PRINTF("Connecting to WiFi: %s\n", ssid.c_str());
  
  currentWiFiState = WIFI_CONNECTING;
  connectionAttempts = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && 
         (millis() - startTime) < (WIFI_CONNECT_TIMEOUT * 1000)) {
    delay(500);
    Serial.print(".");
    
    // Blink LED while connecting
    digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    DEBUG_PRINTLN("✓ WiFi connected successfully!");
    DEBUG_PRINTF("  IP Address: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("  Signal Strength: %d dBm\n", WiFi.RSSI());
    DEBUG_PRINTF("  MAC Address: %s\n", WiFi.macAddress().c_str());
    
    currentWiFiState = WIFI_CONNECTED;
    connectionAttempts = 0;
    digitalWrite(LED_STATUS, HIGH); // Solid ON
    
    return true;
  } else {
    Serial.println();
    DEBUG_PRINTLN("✗ Failed to connect to WiFi");
    DEBUG_PRINTF("  WiFi status: %d\n", WiFi.status());
    
    currentWiFiState = WIFI_DISCONNECTED;
    connectionAttempts++;
    
    return false;
  }
}

/**
 * Connect to saved WiFi credentials
 * @return true if connected successfully
 */
bool connectToSavedWiFi() {
  String ssid, password;
  if (!loadWiFiCredentials(ssid, password)) {
    DEBUG_PRINTLN("No saved WiFi credentials");
    return false;
  }
  
  return connectToWiFi(ssid, password);
}

/**
 * Start Access Point mode
 * Creates AP with generated SSID and saved password
 */
void startAPMode() {
  DEBUG_PRINTLN("Starting Access Point mode...");
  
  currentWiFiState = WIFI_AP_MODE;
  
  // Stop any existing WiFi connection
  WiFi.disconnect();
  delay(100);
  
  // Get AP password from storage
  String apPassword = loadAPPassword();
  
  // Start AP
  WiFi.mode(WIFI_AP);
  bool success = WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  
  if (success) {
    // Configure AP IP address
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;
    
    local_IP.fromString(AP_IP_ADDRESS);
    gateway.fromString(AP_GATEWAY);
    subnet.fromString(AP_SUBNET);
    
    WiFi.softAPConfig(local_IP, gateway, subnet);
    
    IPAddress apIP = WiFi.softAPIP();
    
    DEBUG_PRINTLN("✓ Access Point started!");
    DEBUG_PRINTF("  SSID: %s\n", apSSID.c_str());
    DEBUG_PRINTF("  Password: %s\n", apPassword.c_str());
    DEBUG_PRINTF("  IP Address: %s\n", apIP.toString().c_str());
    DEBUG_PRINTLN("  Connect to this AP and navigate to http://192.168.4.1");
    
    // Fast blink LED in AP mode
    lastLEDToggle = millis();
    
  } else {
    DEBUG_PRINTLN("✗ Failed to start Access Point!");
    digitalWrite(LED_STATUS, LOW);
  }
}

/**
 * Stop Access Point mode
 */
void stopAPMode() {
  DEBUG_PRINTLN("Stopping Access Point mode...");
  WiFi.softAPdisconnect(true);
  currentWiFiState = WIFI_DISCONNECTED;
}

/**
 * Scan WiFi networks and return as JSON
 * @return JSON array of networks: [{"ssid":"...", "rssi":-50, "secure":true}, ...]
 */
String scanNetworksJSON() {
  DEBUG_PRINTLN("Scanning WiFi networks...");
  
  int n = WiFi.scanNetworks();
  
  String json = "[";
  
  if (n == 0) {
    DEBUG_PRINTLN("No networks found");
  } else {
    DEBUG_PRINTF("Found %d networks:\n", n);
    
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      
      json += "{";
      json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"secure\":" + String((WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "true" : "false") + ",";
      json += "\"channel\":" + String(WiFi.channel(i));
      json += "}";
      
      DEBUG_PRINTF("  %d: %s (%d dBm) %s\n", 
                    i + 1, 
                    WiFi.SSID(i).c_str(), 
                    WiFi.RSSI(i),
                    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "[Open]" : "[Secure]");
    }
  }
  
  json += "]";
  
  // Clean up
  WiFi.scanDelete();
  
  return json;
}

/**
 * Check WiFi connection and auto-reconnect if needed
 * Call this in main loop
 */
void checkWiFiConnection() {
  // Only check if we're supposed to be connected (not in AP mode)
  if (currentWiFiState == WIFI_AP_MODE) {
    updateLEDIndicator(); // Update AP mode LED
    return;
  }
  
  // Check connection status
  if (WiFi.status() != WL_CONNECTED && currentWiFiState == WIFI_CONNECTED) {
    DEBUG_PRINTLN("⚠ WiFi connection lost!");
    currentWiFiState = WIFI_DISCONNECTED;
    digitalWrite(LED_STATUS, LOW);
  }
  
  // Auto-reconnect logic
  if (currentWiFiState == WIFI_DISCONNECTED) {
    // Check if enough time has passed since last attempt
    if (millis() - lastConnectionAttempt > WIFI_RECONNECT_INTERVAL) {
      lastConnectionAttempt = millis();
      
      DEBUG_PRINTF("Reconnection attempt %d/%d...\n", 
                    connectionAttempts + 1, WIFI_MAX_ATTEMPTS);
      
      if (connectToSavedWiFi()) {
        DEBUG_PRINTLN("Reconnected successfully!");
        connectionAttempts = 0;
      } else {
        connectionAttempts++;
        
        // If max attempts reached, start AP mode
        if (connectionAttempts >= WIFI_MAX_ATTEMPTS) {
          DEBUG_PRINTLN("Max reconnection attempts reached - starting AP mode");
          startAPMode();
          connectionAttempts = 0;
        }
      }
    }
  }
  
  // Update LED indicator
  updateLEDIndicator();
}

/**
 * Update LED indicator based on WiFi state
 * Call this in main loop
 */
void updateLEDIndicator() {
  unsigned long currentTime = millis();
  
  switch (currentWiFiState) {
    case WIFI_CONNECTED:
      // Solid ON
      digitalWrite(LED_STATUS, HIGH);
      break;
      
    case WIFI_CONNECTING:
      // Slow blink (1 second interval)
      if (currentTime - lastLEDToggle >= LED_BLINK_SLOW) {
        lastLEDToggle = currentTime;
        ledState = !ledState;
        digitalWrite(LED_STATUS, ledState);
      }
      break;
      
    case WIFI_AP_MODE:
      // Fast blink (200ms interval)
      if (currentTime - lastLEDToggle >= LED_BLINK_FAST) {
        lastLEDToggle = currentTime;
        ledState = !ledState;
        digitalWrite(LED_STATUS, ledState);
      }
      break;
      
    case WIFI_DISCONNECTED:
      // OFF
      digitalWrite(LED_STATUS, LOW);
      break;
  }
}

/**
 * Generate AP SSID from MAC address
 * Format: ChickenFeeder-XXXX (last 4 hex digits of MAC)
 * @return Generated SSID
 */
String getAPSSID() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String suffix = mac.substring(mac.length() - 4);
  suffix.toUpperCase();
  
  return String(AP_SSID_PREFIX) + suffix;
}

/**
 * Get current WiFi state
 * @return WiFiState enum value
 */
WiFiState getWiFiState() {
  return currentWiFiState;
}

/**
 * Get WiFi state as human-readable string
 * @return State string
 */
String getWiFiStateString() {
  switch (currentWiFiState) {
    case WIFI_CONNECTED: return "Connected";
    case WIFI_CONNECTING: return "Connecting";
    case WIFI_DISCONNECTED: return "Disconnected";
    case WIFI_AP_MODE: return "AP Mode";
    default: return "Unknown";
  }
}

/**
 * Get WiFi status info as JSON
 * @return JSON string with WiFi information
 */
String getWiFiInfoJSON() {
  String json = "{";
  json += "\"state\":\"" + getWiFiStateString() + "\",";
  
  if (currentWiFiState == WIFI_CONNECTED) {
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\"";
  } else if (currentWiFiState == WIFI_AP_MODE) {
    json += "\"ap_ssid\":\"" + apSSID + "\",";
    json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"ap_clients\":" + String(WiFi.softAPgetStationNum());
  } else {
    json += "\"attempts\":" + String(connectionAttempts);
  }
  
  json += "}";
  return json;
}

#endif // WIFI_MANAGER_H
