/*
 * Web Admin Panel Module
 * Task 6: Complete admin dashboard with motor control and system management
 * 
 * Features:
 * - Real-time statistics dashboard
 * - Manual motor control (test buttons)
 * - WiFi signal monitoring
 * - System information
 * - Reset statistics
 * - Restart device
 * - Mobile-responsive design
 */

#ifndef WEB_ADMIN_H
#define WEB_ADMIN_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "storage.h"
#include "motor_control.h"

// External references
extern WebServer server;
extern unsigned long bootTime;

/**
 * Handle GET /admin
 * Main admin dashboard
 */
void handleAdminDashboard(WebServer &server) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Admin - ChickenFeeder v)rawliteral" + String(FIRMWARE_VERSION) + R"rawliteral(</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
      background: #f5f5f5;
      color: #333;
    }
    
    .header {
      background: linear-gradient(135deg, )rawliteral" + String(COLOR_PRIMARY) + R"rawliteral(, )rawliteral" + String(COLOR_SECONDARY) + R"rawliteral();
      color: white;
      padding: 20px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    }
    
    .header-content {
      max-width: 1200px;
      margin: 0 auto;
    }
    
    .header h1 {
      font-size: 28px;
      font-weight: 600;
      margin-bottom: 5px;
    }
    
    .header .subtitle {
      opacity: 0.9;
      font-size: 14px;
    }
    
    .container {
      max-width: 1200px;
      margin: 0 auto;
      padding: 20px;
    }
    
    .stats-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 15px;
      margin-bottom: 20px;
    }
    
    .stat-card {
      background: white;
      padding: 20px;
      border-radius: 12px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
      text-align: center;
      transition: transform 0.2s;
    }
    
    .stat-card:hover {
      transform: translateY(-3px);
      box-shadow: 0 4px 12px rgba(0,0,0,0.15);
    }
    
    .stat-icon {
      font-size: 32px;
      margin-bottom: 10px;
    }
    
    .stat-value {
      font-size: 36px;
      font-weight: bold;
      color: )rawliteral" + String(COLOR_PRIMARY) + R"rawliteral(;
      margin-bottom: 5px;
    }
    
    .stat-label {
      color: #666;
      font-size: 14px;
    }
    
    .card {
      background: white;
      padding: 25px;
      border-radius: 12px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
      margin-bottom: 20px;
    }
    
    .card h2 {
      font-size: 20px;
      margin-bottom: 20px;
      color: #333;
      display: flex;
      align-items: center;
      gap: 10px;
    }
    
    .motor-controls {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 15px;
      margin-bottom: 15px;
    }
    
    .motor-btn {
      padding: 20px;
      border: none;
      border-radius: 10px;
      font-size: 18px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
      color: white;
      box-shadow: 0 2px 6px rgba(0,0,0,0.2);
    }
    
    .motor-btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(0,0,0,0.3);
    }
    
    .motor-btn:active {
      transform: translateY(0);
    }
    
    .motor-btn:disabled {
      opacity: 0.5;
      cursor: not-allowed;
      transform: none;
    }
    
    .btn-cam { background: linear-gradient(135deg, #f59e0b, #d97706); }
    .btn-ngo { background: linear-gradient(135deg, #10b981, #059669); }
    .btn-stop { background: linear-gradient(135deg, #ef4444, #dc2626); }
    
    .info-table {
      width: 100%;
      border-collapse: collapse;
    }
    
    .info-table tr {
      border-bottom: 1px solid #eee;
    }
    
    .info-table tr:last-child {
      border-bottom: none;
    }
    
    .info-table td {
      padding: 12px 0;
    }
    
    .info-table td:first-child {
      font-weight: 600;
      color: #666;
      width: 40%;
    }
    
    .info-table td:last-child {
      color: #333;
    }
    
    .signal-bar {
      display: inline-block;
      width: 120px;
      height: 10px;
      background: #e0e0e0;
      border-radius: 5px;
      overflow: hidden;
      vertical-align: middle;
      margin-left: 10px;
    }
    
    .signal-fill {
      height: 100%;
      border-radius: 5px;
      transition: width 0.3s;
    }
    
    .signal-excellent { background: #10b981; }
    .signal-good { background: #3b82f6; }
    .signal-fair { background: #f59e0b; }
    .signal-poor { background: #ef4444; }
    
    .action-buttons {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
    }
    
    .btn {
      padding: 12px 24px;
      border: none;
      border-radius: 8px;
      font-size: 15px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
      display: inline-flex;
      align-items: center;
      gap: 8px;
    }
    
    .btn-primary {
      background: )rawliteral" + String(COLOR_PRIMARY) + R"rawliteral(;
      color: white;
    }
    
    .btn-secondary {
      background: #6b7280;
      color: white;
    }
    
    .btn-danger {
      background: )rawliteral" + String(COLOR_DANGER) + R"rawliteral(;
      color: white;
    }
    
    .btn-warning {
      background: )rawliteral" + String(COLOR_WARNING) + R"rawliteral(;
      color: white;
    }
    
    .btn:hover {
      opacity: 0.9;
      transform: translateY(-2px);
      box-shadow: 0 4px 8px rgba(0,0,0,0.2);
    }
    
    .btn:disabled {
      opacity: 0.5;
      cursor: not-allowed;
      transform: none;
    }
    
    .status-indicator {
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      margin-right: 8px;
    }
    
    .status-online { background: #10b981; }
    .status-offline { background: #ef4444; }
    
    .alert {
      padding: 15px;
      border-radius: 8px;
      margin-bottom: 20px;
      display: none;
    }
    
    .alert.show {
      display: block;
      animation: slideDown 0.3s;
    }
    
    .alert-success { background: #d1fae5; color: #065f46; border-left: 4px solid #10b981; }
    .alert-error { background: #fee2e2; color: #991b1b; border-left: 4px solid #ef4444; }
    .alert-warning { background: #fef3c7; color: #92400e; border-left: 4px solid #f59e0b; }
    
    @keyframes slideDown {
      from { opacity: 0; transform: translateY(-10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    
    .footer {
      text-align: center;
      padding: 20px;
      color: #999;
      font-size: 14px;
    }
    
    @media (max-width: 768px) {
      .header h1 { font-size: 22px; }
      .stats-grid { grid-template-columns: repeat(2, 1fr); }
      .motor-controls { grid-template-columns: 1fr; }
      .action-buttons { flex-direction: column; }
      .btn { width: 100%; justify-content: center; }
    }
  </style>
</head>
<body>
  <div class="header">
    <div class="header-content">
      <h1>🐔 ChickenFeeder Admin Panel</h1>
      <div class="subtitle">Quản lý hệ thống cho gà ăn tự động - Firmware v)rawliteral" + String(FIRMWARE_VERSION) + R"rawliteral(</div>
    </div>
  </div>
  
  <div class="container">
    <div id="alert" class="alert"></div>
    
    <!-- Statistics -->
    <div class="stats-grid">
      <div class="stat-card">
        <div class="stat-icon">🌾</div>
        <div class="stat-value" id="stat-cam">)rawliteral";
  
  uint32_t cam, ngo;
  getStats(cam, ngo);
  html += String(cam);
  
  html += R"rawliteral(</div>
        <div class="stat-label">Cám</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon">🌽</div>
        <div class="stat-value" id="stat-ngo">)rawliteral" + String(ngo) + R"rawliteral(</div>
        <div class="stat-label">Ngô</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon">📊</div>
        <div class="stat-value" id="stat-total">)rawliteral" + String(cam + ngo) + R"rawliteral(</div>
        <div class="stat-label">Tổng cộng</div>
      </div>
    </div>
    
    <!-- Motor Control -->
    <div class="card">
      <h2>🎮 Điều khiển Motors</h2>
      <div class="motor-controls">
        <button class="motor-btn btn-cam" onclick="feedMotor('cam')">
          🌾 Cho ăn Cám
        </button>
        <button class="motor-btn btn-ngo" onclick="feedMotor('ngo')">
          🌽 Cho ăn Ngô
        </button>
        <button class="motor-btn btn-stop" onclick="stopAll()">
          🛑 Dừng tất cả
        </button>
      </div>
      <p style="color: #666; font-size: 14px; margin-top: 10px;">
        ⏱️ Mỗi lần nhấn sẽ chạy motor trong 1.5 giây
      </p>
    </div>
    
    <!-- Device Info -->
    <div class="card">
      <h2>ℹ️ Thông tin thiết bị</h2>
      <table class="info-table">
        <tr>
          <td>Trạng thái</td>
          <td>
            <span class="status-indicator status-online"></span>
            <strong>Đang hoạt động</strong>
          </td>
        </tr>
        <tr>
          <td>Tên thiết bị</td>
          <td id="device-name">)rawliteral" + loadDeviceName() + R"rawliteral(</td>
        </tr>
        <tr>
          <td>IP Address</td>
          <td id="device-ip">)rawliteral" + WiFi.localIP().toString() + R"rawliteral(</td>
        </tr>
        <tr>
          <td>WiFi SSID</td>
          <td id="wifi-ssid">)rawliteral" + WiFi.SSID() + R"rawliteral(</td>
        </tr>
        <tr>
          <td>Tín hiệu WiFi</td>
          <td id="wifi-signal">)rawliteral" + String(WiFi.RSSI()) + R"rawliteral( dBm
            <div class="signal-bar">
              <div class="signal-fill" id="signal-fill"></div>
            </div>
          </td>
        </tr>
        <tr>
          <td>MAC Address</td>
          <td>)rawliteral" + WiFi.macAddress() + R"rawliteral(</td>
        </tr>
        <tr>
          <td>Thời gian hoạt động</td>
          <td id="uptime">)rawliteral" + String((millis() - bootTime) / 1000) + R"rawliteral( giây</td>
        </tr>
        <tr>
          <td>Bộ nhớ còn trống</td>
          <td id="free-memory">)rawliteral" + String(ESP.getFreeHeap()) + R"rawliteral( bytes</td>
        </tr>
      </table>
    </div>
    
    <!-- System Controls -->
    <div class="card">
      <h2>⚙️ Quản lý hệ thống</h2>
      <div class="action-buttons">
        <button class="btn btn-primary" onclick="refreshStats()">
          🔄 Làm mới dữ liệu
        </button>
        <button class="btn btn-warning" onclick="resetStats()">
          ♻️ Reset thống kê
        </button>
        <button class="btn btn-secondary" onclick="window.location.href='/'">
          🏠 Về trang chủ
        </button>
        <button class="btn btn-danger" onclick="restartDevice()">
          🔌 Khởi động lại
        </button>
      </div>
    </div>
  </div>
  
  <div class="footer">
    ChickenFeeder v)rawliteral" + String(FIRMWARE_VERSION) + R"rawliteral( &copy; 2024 - )rawliteral" + String(MANUFACTURER) + R"rawliteral(
  </div>
  
  <script>
    // Update signal bar
    function updateSignalBar() {
      const rssi = )rawliteral" + String(WiFi.RSSI()) + R"rawliteral(;
      const signalFill = document.getElementById('signal-fill');
      
      let percentage, className;
      if (rssi >= -50) {
        percentage = 100;
        className = 'signal-excellent';
      } else if (rssi >= -60) {
        percentage = 80;
        className = 'signal-good';
      } else if (rssi >= -70) {
        percentage = 60;
        className = 'signal-fair';
      } else {
        percentage = 40;
        className = 'signal-poor';
      }
      
      signalFill.style.width = percentage + '%';
      signalFill.className = 'signal-fill ' + className;
    }
    
    updateSignalBar();
    
    // Show alert
    function showAlert(message, type = 'success') {
      const alert = document.getElementById('alert');
      alert.textContent = message;
      alert.className = 'alert alert-' + type + ' show';
      setTimeout(() => {
        alert.classList.remove('show');
      }, 3000);
    }
    
    // Feed motor
    async function feedMotor(motor) {
      const buttons = document.querySelectorAll('.motor-btn');
      buttons.forEach(btn => btn.disabled = true);
      
      try {
        const response = await fetch('/feed', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ motor: motor, duration: 1500 })
        });
        
        const data = await response.json();
        
        if (data.success) {
          showAlert('✅ ' + data.message, 'success');
          setTimeout(refreshStats, 1500);
        } else {
          showAlert('❌ ' + data.message, 'error');
        }
      } catch (error) {
        showAlert('❌ Lỗi kết nối: ' + error.message, 'error');
      } finally {
        setTimeout(() => {
          buttons.forEach(btn => btn.disabled = false);
        }, 1500);
      }
    }
    
    // Stop all motors
    async function stopAll() {
      try {
        const response = await fetch('/stop', { method: 'POST' });
        const data = await response.json();
        
        if (data.success) {
          showAlert('🛑 ' + data.message, 'warning');
        }
      } catch (error) {
        showAlert('❌ Lỗi: ' + error.message, 'error');
      }
    }
    
    // Refresh statistics
    async function refreshStats() {
      try {
        const response = await fetch('/status');
        const data = await response.json();
        
        if (data.success) {
          document.getElementById('stat-cam').textContent = data.statistics.cam;
          document.getElementById('stat-ngo').textContent = data.statistics.ngo;
          document.getElementById('stat-total').textContent = data.statistics.total;
          document.getElementById('uptime').textContent = data.uptime_seconds + ' giây';
          document.getElementById('free-memory').textContent = data.free_heap + ' bytes';
          
          showAlert('✅ Dữ liệu đã được cập nhật', 'success');
        }
      } catch (error) {
        showAlert('❌ Lỗi khi làm mới dữ liệu', 'error');
      }
    }
    
    // Reset statistics
    async function resetStats() {
      if (!confirm('Bạn có chắc muốn reset tất cả thống kê về 0?')) return;
      
      try {
        const response = await fetch('/api/reset-stats', { method: 'POST' });
        const data = await response.json();
        
        if (data.success) {
          showAlert('✅ ' + data.message, 'success');
          refreshStats();
        } else {
          showAlert('❌ ' + data.message, 'error');
        }
      } catch (error) {
        showAlert('❌ Lỗi: ' + error.message, 'error');
      }
    }
    
    // Restart device
    async function restartDevice() {
      if (!confirm('Bạn có chắc muốn khởi động lại thiết bị?')) return;
      
      showAlert('🔌 Đang khởi động lại thiết bị...', 'warning');
      
      try {
        await fetch('/api/restart', { method: 'POST' });
        
        setTimeout(() => {
          showAlert('⏳ Vui lòng đợi 10 giây...', 'warning');
        }, 1000);
        
        setTimeout(() => {
          window.location.reload();
        }, 10000);
      } catch (error) {
        // Expected - device is restarting
      }
    }
    
    // Auto refresh every 10 seconds
    setInterval(refreshStats, 10000);
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

/**
 * Handle POST /api/reset-stats
 * Reset all statistics to 0
 */
void handleResetStats(WebServer &server) {
  DEBUG_PRINTLN("\n--- Reset Statistics Request ---");
  
  preferences.putUInt(KEY_CAM_COUNT, 0);
  preferences.putUInt(KEY_NGO_COUNT, 0);
  // THOC_COUNT removed - only 2 motors
  
  DEBUG_PRINTLN("Statistics reset to 0");
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Thống kê đã được reset về 0\"}");
}

/**
 * Handle POST /api/restart
 * Restart ESP32 device
 */
void handleRestart(WebServer &server) {
  DEBUG_PRINTLN("\n--- Restart Request ---");
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Đang khởi động lại...\"}");
  
  delay(1000);
  ESP.restart();
}

/**
 * Setup admin panel routes
 */
void setupAdminRoutes(WebServer &server) {
  server.on("/admin", HTTP_GET, [&server]() { handleAdminDashboard(server); });
  server.on("/api/reset-stats", HTTP_POST, [&server]() { handleResetStats(server); });
  server.on("/api/restart", HTTP_POST, [&server]() { handleRestart(server); });
  
  DEBUG_PRINTLN("Admin panel routes registered");
}

#endif // WEB_ADMIN_H
