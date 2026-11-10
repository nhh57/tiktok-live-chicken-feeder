/*
 * Setup Portal Module - WiFi Configuration Web Interface
 * 
 * Provides web interface for WiFi configuration when device is in AP mode
 * 
 * Features:
 * - Scan WiFi networks
 * - Display available networks with signal strength
 * - Form to input WiFi credentials
 * - Save credentials and reconnect
 * - Mobile-responsive design
 * - Real-time connection status
 * 
 * Usage:
 * - Called when ESP32 is in AP mode
 * - User connects to ChickenFeeder-XXXX AP
 * - Navigate to http://192.168.4.1
 * - Select WiFi and enter password
 */

#ifndef WEB_SETUP_H
#define WEB_SETUP_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "storage.h"
#include "wifi_manager.h"

// External WebServer object
extern WebServer server;

// ==================== Function Prototypes ====================
void setupSetupPortalRoutes();
void handleSetupRoot();
void handleSetupScan();
void handleSetupConnect();
void handleSetupStatus();

// ==================== Setup Portal Routes ====================

/**
 * Register all setup portal routes
 * Call this from main setup() when starting HTTP server
 */
void setupSetupPortalRoutes() {
  // Setup portal routes (active in AP mode)
  server.on("/setup", HTTP_GET, handleSetupRoot);
  server.on("/setup/scan", HTTP_GET, handleSetupScan);
  server.on("/setup/connect", HTTP_POST, handleSetupConnect);
  server.on("/setup/status", HTTP_GET, handleSetupStatus);
  
  DEBUG_PRINTLN("Setup portal routes registered");
}

// ==================== HTML Pages ====================

/**
 * Main setup portal page
 * GET /setup or GET / (when in AP mode)
 */
void handleSetupRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ChickenFeeder Setup</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .container {
      background: white;
      border-radius: 16px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 480px;
      width: 100%;
      overflow: hidden;
    }
    .header {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 30px 20px;
      text-align: center;
    }
    .header h1 {
      font-size: 24px;
      margin-bottom: 8px;
    }
    .header p {
      opacity: 0.9;
      font-size: 14px;
    }
    .content {
      padding: 30px 20px;
    }
    .section {
      margin-bottom: 25px;
    }
    .section-title {
      font-size: 16px;
      font-weight: 600;
      color: #333;
      margin-bottom: 12px;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .btn {
      background: #667eea;
      color: white;
      border: none;
      padding: 12px 24px;
      border-radius: 8px;
      font-size: 15px;
      font-weight: 600;
      cursor: pointer;
      width: 100%;
      transition: all 0.3s;
    }
    .btn:hover {
      background: #5568d3;
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
    }
    .btn:disabled {
      background: #ccc;
      cursor: not-allowed;
      transform: none;
    }
    .btn-secondary {
      background: #f3f4f6;
      color: #667eea;
    }
    .btn-secondary:hover {
      background: #e5e7eb;
    }
    .network-list {
      list-style: none;
      max-height: 300px;
      overflow-y: auto;
      border: 1px solid #e5e7eb;
      border-radius: 8px;
    }
    .network-item {
      padding: 12px 16px;
      border-bottom: 1px solid #e5e7eb;
      cursor: pointer;
      transition: background 0.2s;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .network-item:last-child {
      border-bottom: none;
    }
    .network-item:hover {
      background: #f9fafb;
    }
    .network-item.selected {
      background: #ede9fe;
      border-left: 3px solid #667eea;
    }
    .network-name {
      font-weight: 500;
      color: #333;
    }
    .network-signal {
      font-size: 12px;
      color: #6b7280;
    }
    .signal-strong { color: #10b981; }
    .signal-medium { color: #f59e0b; }
    .signal-weak { color: #ef4444; }
    .form-group {
      margin-bottom: 16px;
    }
    .form-label {
      display: block;
      font-size: 14px;
      font-weight: 500;
      color: #374151;
      margin-bottom: 6px;
    }
    .form-input {
      width: 100%;
      padding: 10px 12px;
      border: 1px solid #d1d5db;
      border-radius: 8px;
      font-size: 15px;
      transition: border 0.3s;
    }
    .form-input:focus {
      outline: none;
      border-color: #667eea;
      box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
    }
    .status {
      padding: 12px;
      border-radius: 8px;
      margin-top: 16px;
      font-size: 14px;
      display: none;
    }
    .status.show {
      display: block;
    }
    .status-success {
      background: #d1fae5;
      color: #065f46;
    }
    .status-error {
      background: #fee2e2;
      color: #991b1b;
    }
    .status-info {
      background: #dbeafe;
      color: #1e40af;
    }
    .spinner {
      border: 3px solid #f3f3f3;
      border-top: 3px solid #667eea;
      border-radius: 50%;
      width: 24px;
      height: 24px;
      animation: spin 1s linear infinite;
      display: inline-block;
      margin-right: 8px;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    .hidden {
      display: none;
    }
    .icon {
      width: 20px;
      height: 20px;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🐔 ChickenFeeder Setup</h1>
      <p>Configure WiFi Connection</p>
    </div>
    
    <div class="content">
      <!-- Step 1: Scan Networks -->
      <div class="section" id="scanSection">
        <div class="section-title">
          <span>📡</span>
          <span>Step 1: Scan WiFi Networks</span>
        </div>
        <button class="btn" onclick="scanNetworks()">
          <span id="scanBtnText">Scan Networks</span>
        </button>
        <ul class="network-list hidden" id="networkList"></ul>
      </div>
      
      <!-- Step 2: Enter Password -->
      <div class="section hidden" id="passwordSection">
        <div class="section-title">
          <span>🔐</span>
          <span>Step 2: Enter Password</span>
        </div>
        <form onsubmit="connectWiFi(event)">
          <div class="form-group">
            <label class="form-label">Network (SSID)</label>
            <input type="text" class="form-input" id="ssidInput" readonly>
          </div>
          <div class="form-group">
            <label class="form-label">Password</label>
            <input type="password" class="form-input" id="passwordInput" placeholder="Enter WiFi password">
          </div>
          <button type="submit" class="btn" id="connectBtn">
            Connect
          </button>
          <button type="button" class="btn btn-secondary" onclick="backToScan()" style="margin-top: 8px;">
            Back
          </button>
        </form>
      </div>
      
      <!-- Status Messages -->
      <div class="status" id="statusMsg"></div>
    </div>
  </div>

  <script>
    let selectedSSID = '';
    
    async function scanNetworks() {
      const btn = document.getElementById('scanBtnText');
      const list = document.getElementById('networkList');
      
      btn.innerHTML = '<span class="spinner"></span>Scanning...';
      list.innerHTML = '';
      list.classList.add('hidden');
      
      try {
        const response = await fetch('/setup/scan');
        const networks = await response.json();
        
        if (networks.length === 0) {
          showStatus('No networks found', 'error');
        } else {
          list.classList.remove('hidden');
          networks.forEach(network => {
            const item = document.createElement('li');
            item.className = 'network-item';
            item.onclick = () => selectNetwork(network.ssid);
            
            const signalClass = network.rssi > -60 ? 'signal-strong' : 
                               network.rssi > -75 ? 'signal-medium' : 'signal-weak';
            
            const lockIcon = network.secure ? '🔒' : '🔓';
            
            item.innerHTML = `
              <span class="network-name">${lockIcon} ${network.ssid}</span>
              <span class="network-signal ${signalClass}">${network.rssi} dBm</span>
            `;
            list.appendChild(item);
          });
        }
      } catch (error) {
        showStatus('Scan failed: ' + error.message, 'error');
      }
      
      btn.textContent = 'Scan Again';
    }
    
    function selectNetwork(ssid) {
      selectedSSID = ssid;
      document.getElementById('ssidInput').value = ssid;
      document.getElementById('passwordInput').value = '';
      document.getElementById('scanSection').classList.add('hidden');
      document.getElementById('passwordSection').classList.remove('hidden');
      hideStatus();
    }
    
    function backToScan() {
      document.getElementById('scanSection').classList.remove('hidden');
      document.getElementById('passwordSection').classList.add('hidden');
      hideStatus();
    }
    
    async function connectWiFi(event) {
      event.preventDefault();
      
      const ssid = document.getElementById('ssidInput').value;
      const password = document.getElementById('passwordInput').value;
      const btn = document.getElementById('connectBtn');
      
      if (!password) {
        showStatus('Please enter password', 'error');
        return;
      }
      
      btn.disabled = true;
      btn.innerHTML = '<span class="spinner"></span>Connecting...';
      showStatus('Connecting to ' + ssid + '...', 'info');
      
      try {
        const response = await fetch('/setup/connect', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({ssid, password})
        });
        
        const result = await response.json();
        
        if (result.success) {
          showStatus('✓ Connected successfully! Device will restart...', 'success');
          setTimeout(() => {
            showStatus('You can now close this window and reconnect to your WiFi', 'success');
          }, 3000);
        } else {
          showStatus('✗ Connection failed: ' + result.message, 'error');
          btn.disabled = false;
          btn.textContent = 'Retry';
        }
      } catch (error) {
        showStatus('✗ Error: ' + error.message, 'error');
        btn.disabled = false;
        btn.textContent = 'Retry';
      }
    }
    
    function showStatus(message, type) {
      const status = document.getElementById('statusMsg');
      status.textContent = message;
      status.className = 'status status-' + type + ' show';
    }
    
    function hideStatus() {
      const status = document.getElementById('statusMsg');
      status.classList.remove('show');
    }
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html; charset=UTF-8", html);
}

/**
 * Scan WiFi networks and return JSON
 * GET /setup/scan
 */
void handleSetupScan() {
  DEBUG_PRINTLN("WiFi scan requested");
  
  String json = scanNetworksJSON();
  
  server.send(200, "application/json", json);
  DEBUG_PRINTLN("Scan results sent");
}

/**
 * Connect to WiFi network
 * POST /setup/connect
 * Body: {"ssid": "...", "password": "..."}
 */
void handleSetupConnect() {
  DEBUG_PRINTLN("\n--- Setup Connect Request ---");
  
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing request body\"}");
    return;
  }
  
  String body = server.arg("plain");
  DEBUG_PRINTF("Body: %s\n", body.c_str());
  
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    DEBUG_PRINTF("JSON parse error: %s\n", error.c_str());
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }
  
  String ssid = doc["ssid"].as<String>();
  String password = doc["password"].as<String>();
  
  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"SSID required\"}");
    return;
  }
  
  DEBUG_PRINTF("Saving WiFi: SSID='%s'\n", ssid.c_str());
  
  // Save credentials
  saveWiFiCredentials(ssid, password);
  setSetupDone(true);
  
  // Send success response
  server.send(200, "application/json", "{\"success\":true,\"message\":\"WiFi credentials saved. Restarting...\"}");
  
  DEBUG_PRINTLN("WiFi credentials saved. Restarting in 2 seconds...");
  
  // Restart after delay
  delay(2000);
  ESP.restart();
}

/**
 * Get connection status
 * GET /setup/status
 */
void handleSetupStatus() {
  String json = "{";
  json += "\"state\":\"" + getWiFiStateString() + "\",";
  json += "\"connected\":" + String((getWiFiState() == WIFI_CONNECTED) ? "true" : "false");
  if (getWiFiState() == WIFI_CONNECTED) {
    json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  }
  json += "}";
  
  server.send(200, "application/json", json);
}

#endif // WEB_SETUP_H
