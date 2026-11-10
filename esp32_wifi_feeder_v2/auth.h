/*
 * Authentication & Session Management
 * Task 5: Login, Session, Protected Routes
 * 
 * Features:
 * - Login page with beautiful UI
 * - Session management using cookies
 * - Protected route middleware
 * - Password verification with SHA256
 * - Logout functionality
 * - Change password API
 * 
 * Security Notes:
 * - Passwords hashed with SHA256 (defined in storage.h)
 * - Sessions expire after SESSION_TIMEOUT
 * - Session tokens are random 32-char strings
 * - Only one active session per device (single admin)
 */

#ifndef AUTH_H
#define AUTH_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "storage.h"

// ==================== Session Management ====================

// Current active session
struct Session {
  String token;
  unsigned long loginTime;
  bool isValid;
};

Session currentSession = {"", 0, false};

/**
 * Generate random session token
 */
String generateSessionToken() {
  String token = "";
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  
  for (int i = 0; i < 32; i++) {
    token += charset[random(0, sizeof(charset) - 1)];
  }
  
  return token;
}

/**
 * Create new session
 */
void createSession() {
  currentSession.token = generateSessionToken();
  currentSession.loginTime = millis();
  currentSession.isValid = true;
  
  DEBUG_PRINTF("New session created: %s\n", currentSession.token.c_str());
}

/**
 * Destroy current session (logout)
 */
void destroySession() {
  currentSession.token = "";
  currentSession.loginTime = 0;
  currentSession.isValid = false;
  
  DEBUG_PRINTLN("Session destroyed");
}

/**
 * Check if session is valid
 */
bool isSessionValid(String token) {
  if (!currentSession.isValid) {
    return false;
  }
  
  if (token != currentSession.token) {
    return false;
  }
  
  // Check timeout
  unsigned long sessionAge = millis() - currentSession.loginTime;
  if (sessionAge > SESSION_TIMEOUT) {
    DEBUG_PRINTLN("Session expired");
    destroySession();
    return false;
  }
  
  return true;
}

/**
 * Extract session token from cookie header
 */
String getSessionTokenFromCookie(WebServer &server) {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    
    // Find "session=" in cookie string
    int sessionIndex = cookie.indexOf("session=");
    if (sessionIndex >= 0) {
      int start = sessionIndex + 8; // length of "session="
      int end = cookie.indexOf(";", start);
      
      if (end < 0) {
        end = cookie.length();
      }
      
      return cookie.substring(start, end);
    }
  }
  
  return "";
}

/**
 * Check if current request is authenticated
 */
bool isAuthenticated(WebServer &server) {
  String token = getSessionTokenFromCookie(server);
  return isSessionValid(token);
}

/**
 * Middleware: Require authentication
 * Returns true if authenticated, false if not (and sends redirect)
 */
bool requireAuth(WebServer &server) {
  if (isAuthenticated(server)) {
    return true;
  }
  
  // Not authenticated - redirect to login
  DEBUG_PRINTLN("Access denied - not authenticated");
  server.sendHeader("Location", "/login");
  server.send(302, "text/plain", "Redirecting to login...");
  return false;
}

// ==================== HTTP Route Handlers ====================

/**
 * Handle GET /login
 * Show login page
 */
void handleLogin(WebServer &server) {
  // If already logged in, redirect to admin
  if (isAuthenticated(server)) {
    server.sendHeader("Location", "/admin");
    server.send(302, "text/plain", "Already logged in");
    return;
  }
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Login - ChickenFeeder Admin</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, )rawliteral" + String(COLOR_PRIMARY) + R"rawliteral( 0%, )rawliteral" + String(COLOR_SECONDARY) + R"rawliteral( 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    
    .login-container {
      background: white;
      padding: 40px;
      border-radius: 16px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      width: 100%;
      max-width: 400px;
      animation: slideUp 0.4s ease;
    }
    
    @keyframes slideUp {
      from { opacity: 0; transform: translateY(30px); }
      to { opacity: 1; transform: translateY(0); }
    }
    
    .logo {
      text-align: center;
      margin-bottom: 30px;
    }
    
    .logo-icon {
      font-size: 60px;
      margin-bottom: 10px;
    }
    
    h1 {
      color: #333;
      font-size: 24px;
      font-weight: 600;
      text-align: center;
      margin-bottom: 10px;
    }
    
    .subtitle {
      text-align: center;
      color: #666;
      font-size: 14px;
      margin-bottom: 30px;
    }
    
    .form-group {
      margin-bottom: 20px;
    }
    
    label {
      display: block;
      margin-bottom: 8px;
      color: #333;
      font-weight: 500;
      font-size: 14px;
    }
    
    input {
      width: 100%;
      padding: 12px 16px;
      border: 2px solid #e0e0e0;
      border-radius: 8px;
      font-size: 16px;
      transition: all 0.3s;
      font-family: inherit;
    }
    
    input:focus {
      outline: none;
      border-color: )rawliteral" + String(COLOR_PRIMARY) + R"rawliteral(;
      box-shadow: 0 0 0 3px )rawliteral" + String(COLOR_PRIMARY) + R"rawliteral(20);
    }
    
    button {
      width: 100%;
      padding: 14px;
      background: )rawliteral" + String(COLOR_PRIMARY) + R"rawliteral(;
      color: white;
      border: none;
      border-radius: 8px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
      margin-top: 10px;
    }
    
    button:hover {
      background: )rawliteral" + String(COLOR_SECONDARY) + R"rawliteral(;
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(0,0,0,0.2);
    }
    
    button:active {
      transform: translateY(0);
    }
    
    button:disabled {
      background: #ccc;
      cursor: not-allowed;
      transform: none;
    }
    
    .error {
      background: #fee;
      color: #c33;
      padding: 12px;
      border-radius: 8px;
      margin-bottom: 20px;
      font-size: 14px;
      display: none;
      border-left: 4px solid #c33;
    }
    
    .error.show {
      display: block;
      animation: shake 0.4s;
    }
    
    @keyframes shake {
      0%, 100% { transform: translateX(0); }
      25% { transform: translateX(-10px); }
      75% { transform: translateX(10px); }
    }
    
    .footer {
      text-align: center;
      margin-top: 20px;
      color: #999;
      font-size: 12px;
    }
  </style>
</head>
<body>
  <div class="login-container">
    <div class="logo">
      <div class="logo-icon">🐔</div>
      <h1>ChickenFeeder Admin</h1>
      <div class="subtitle">Đăng nhập để quản lý hệ thống</div>
    </div>
    
    <div id="error" class="error"></div>
    
    <form id="loginForm">
      <div class="form-group">
        <label for="username">Tên đăng nhập</label>
        <input type="text" id="username" name="username" required autocomplete="username" autofocus>
      </div>
      
      <div class="form-group">
        <label for="password">Mật khẩu</label>
        <input type="password" id="password" name="password" required autocomplete="current-password">
      </div>
      
      <button type="submit" id="submitBtn">Đăng nhập</button>
    </form>
    
    <div class="footer">
      ChickenFeeder v)rawliteral" + String(FIRMWARE_VERSION) + R"rawliteral( &copy; 2024
    </div>
  </div>
  
  <script>
    const form = document.getElementById('loginForm');
    const errorDiv = document.getElementById('error');
    const submitBtn = document.getElementById('submitBtn');
    
    form.addEventListener('submit', async (e) => {
      e.preventDefault();
      
      errorDiv.classList.remove('show');
      submitBtn.disabled = true;
      submitBtn.textContent = 'Đang đăng nhập...';
      
      const username = document.getElementById('username').value;
      const password = document.getElementById('password').value;
      
      try {
        const response = await fetch('/api/login', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ username, password })
        });
        
        const data = await response.json();
        
        if (data.success) {
          window.location.href = '/admin';
        } else {
          errorDiv.textContent = data.message || 'Đăng nhập thất bại';
          errorDiv.classList.add('show');
          submitBtn.disabled = false;
          submitBtn.textContent = 'Đăng nhập';
        }
      } catch (error) {
        errorDiv.textContent = 'Lỗi kết nối đến server';
        errorDiv.classList.add('show');
        submitBtn.disabled = false;
        submitBtn.textContent = 'Đăng nhập';
      }
    });
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

/**
 * Handle POST /api/login
 * Process login request
 */
void handleLoginAPI(WebServer &server) {
  DEBUG_PRINTLN("\n--- Login Request ---");
  
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing request body\"}");
    return;
  }
  
  String body = server.arg("plain");
  DEBUG_PRINTF("Body: %s\n", body.c_str());
  
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }
  
  String username = doc["username"] | "";
  String password = doc["password"] | "";
  
  if (username.length() == 0 || password.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Username and password required\"}");
    return;
  }
  
  // Verify credentials
  if (verifyAdminCredentials(username, password)) {
    // Login successful - create session
    createSession();
    
    // Set session cookie
    String cookie = "session=" + currentSession.token + "; Path=/; Max-Age=" + String(SESSION_TIMEOUT / 1000) + "; HttpOnly; SameSite=Strict";
    server.sendHeader("Set-Cookie", cookie);
    
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Login successful\"}");
    DEBUG_PRINTF("Login successful: %s\n", username.c_str());
  } else {
    // Login failed
    server.send(401, "application/json", "{\"success\":false,\"message\":\"Tên đăng nhập hoặc mật khẩu không đúng\"}");
    DEBUG_PRINTLN("Login failed: invalid credentials");
  }
}

/**
 * Handle GET /logout
 * Logout and destroy session
 */
void handleLogout(WebServer &server) {
  DEBUG_PRINTLN("Logout request");
  
  destroySession();
  
  // Clear session cookie
  server.sendHeader("Set-Cookie", "session=; Path=/; Max-Age=0");
  server.sendHeader("Location", "/login");
  server.send(302, "text/plain", "Logged out");
}

/**
 * Handle POST /api/change-password
 * Change admin password (requires authentication)
 */
void handleChangePassword(WebServer &server) {
  if (!requireAuth(server)) return;
  
  DEBUG_PRINTLN("\n--- Change Password Request ---");
  
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing request body\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }
  
  String currentPassword = doc["current_password"] | "";
  String newPassword = doc["new_password"] | "";
  
  if (currentPassword.length() == 0 || newPassword.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Current and new password required\"}");
    return;
  }
  
  if (newPassword.length() < 6) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"New password must be at least 6 characters\"}");
    return;
  }
  
  // Load current admin credentials
  String username, storedPasswordHash;
  loadAdminCredentials(username, storedPasswordHash);
  
  // Verify current password
  String currentPasswordHash = hashPassword(currentPassword);
  if (currentPasswordHash != storedPasswordHash) {
    server.send(401, "application/json", "{\"success\":false,\"message\":\"Current password is incorrect\"}");
    DEBUG_PRINTLN("Password change failed: incorrect current password");
    return;
  }
  
  // Save new password
  String newPasswordHash = hashPassword(newPassword);
  saveAdminCredentials(username, newPasswordHash);
  
  // Destroy current session - user must login again with new password
  destroySession();
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Password changed successfully. Please login again.\"}");
  DEBUG_PRINTLN("Password changed successfully");
}

/**
 * Setup authentication routes
 */
void setupAuthRoutes(WebServer &server) {
  server.on("/login", HTTP_GET, [&server]() { handleLogin(server); });
  server.on("/api/login", HTTP_POST, [&server]() { handleLoginAPI(server); });
  server.on("/logout", HTTP_GET, [&server]() { handleLogout(server); });
  server.on("/api/change-password", HTTP_POST, [&server]() { handleChangePassword(server); });
  
  DEBUG_PRINTLN("Authentication routes registered");
}

#endif // AUTH_H
