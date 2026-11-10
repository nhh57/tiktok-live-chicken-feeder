/*
 * ESP32 WiFi Manager + Admin Portal - Configuration
 * 
 * Tất cả các constants và cấu hình cho hệ thống
 * Thay đổi các giá trị ở đây thay vì hard-code trong code chính
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==================== Device Information ====================
#define DEVICE_NAME "ChickenFeeder"
#define FIRMWARE_VERSION "2.0.0"
#define MANUFACTURER "HoangHai Tech"

// ==================== Pin Definitions ====================
// Motor control pins (Active LOW relays)
#define RELAY_CAM   25  // GPIO 25 - Relay for Cám (corn feed)
#define RELAY_NGO   26  // GPIO 26 - Relay for Ngô (corn)
#define RELAY_THOC  27  // GPIO 27 - Relay for Thóc (rice)

// Status LED pin
#define LED_STATUS  2   // GPIO 2 - Built-in LED (Active HIGH)

// ==================== Motor Control Limits ====================
#define MIN_DURATION   100     // Minimum motor duration (ms)
#define MAX_DURATION   10000   // Maximum motor duration (ms) - 10 seconds
#define CYCLE_DURATION 300     // One cycle duration (ms)
#define MAX_CYCLES     20      // Maximum cycles per command

// ==================== WiFi Configuration ====================
// AP Mode (khi chưa có WiFi hoặc setup mode)
#define AP_SSID_PREFIX "ChickenFeeder-"  // Will append last 4 digits of MAC
#define DEFAULT_AP_PASSWORD "12345678"    // Default AP password (8 chars minimum)
#define AP_IP_ADDRESS "192.168.4.1"       // AP mode IP address
#define AP_GATEWAY "192.168.4.1"
#define AP_SUBNET "255.255.255.0"

// WiFi connection settings
#define WIFI_CONNECT_TIMEOUT 10   // Seconds to wait for WiFi connection
#define WIFI_MAX_ATTEMPTS 3       // Max connection attempts before AP mode
#define WIFI_RECONNECT_INTERVAL 30000  // Check connection every 30 seconds (ms)

// ==================== Admin Credentials (Default) ====================
#define DEFAULT_ADMIN_USERNAME "admin"
#define DEFAULT_ADMIN_PASSWORD "admin123"  // Will be hashed in storage

// ==================== Session Management ====================
#define SESSION_TIMEOUT 1800000   // 30 minutes in milliseconds
#define MAX_SESSIONS 5            // Maximum concurrent sessions
#define TOKEN_LENGTH 32           // Session token length (characters)

// ==================== HTTP Server ====================
#define HTTP_PORT 80              // Web server port

// ==================== LED Indicator Patterns ====================
// Blinking patterns for different states
#define LED_BLINK_FAST 200        // Fast blink: AP mode (ms)
#define LED_BLINK_SLOW 1000       // Slow blink: Connecting (ms)
#define LED_SOLID_ON 0            // Solid ON: Connected
#define LED_OFF -1                // OFF: Error/disabled

// ==================== Storage Keys (Preferences) ====================
// Namespace for NVS storage
#define PREFS_NAMESPACE "feeder"

// Storage keys
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PASS "wifi_pass"
#define KEY_ADMIN_USER "admin_user"
#define KEY_ADMIN_PASS "admin_pass_hash"
#define KEY_AP_PASS "ap_pass"
#define KEY_CAM_COUNT "cam_count"
#define KEY_NGO_COUNT "ngo_count"
#define KEY_THOC_COUNT "thoc_count"
#define KEY_SETUP_DONE "setup_done"
#define KEY_DEVICE_NAME "device_name"

// ==================== Timing Constants ====================
#define LOOP_DELAY 10             // Main loop delay (ms)
#define STATUS_UPDATE_INTERVAL 5000  // Dashboard refresh interval (ms)
#define CLEANUP_INTERVAL 60000    // Session cleanup interval (ms)

// ==================== Web UI Colors (CSS) ====================
#define COLOR_PRIMARY "#667eea"
#define COLOR_SUCCESS "#10b981"
#define COLOR_WARNING "#f59e0b"
#define COLOR_DANGER "#ef4444"
#define COLOR_INFO "#3b82f6"
#define COLOR_DARK "#1f2937"
#define COLOR_LIGHT "#f9fafb"

// ==================== Debug Settings ====================
#define SERIAL_BAUD_RATE 115200
#define DEBUG_MODE true           // Set to false in production

// Debug logging macro
#if DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, ...)
#endif

// ==================== Feature Flags ====================
#define ENABLE_CAPTIVE_PORTAL true   // Enable DNS captive portal in AP mode
#define ENABLE_OTA_UPDATE false       // OTA firmware update (not implemented yet)
#define ENABLE_MQTT false             // MQTT support (not implemented yet)

// ==================== System Limits ====================
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define MAX_USERNAME_LENGTH 32
#define HASH_LENGTH 64            // SHA256 hash length in hex

#endif // CONFIG_H
