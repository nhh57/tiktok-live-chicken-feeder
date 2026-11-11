/*
 * Storage Module - Persistent Data Management
 * 
 * Quản lý lưu trữ dữ liệu vào NVS (Non-Volatile Storage) của ESP32
 * Dữ liệu được giữ nguyên khi restart hoặc mất điện
 * 
 * Data stored:
 * - WiFi credentials (SSID + Password)
 * - Admin credentials (Username + Password hash)
 * - AP password
 * - Statistics (cam/ngo/thoc feed counts)
 * - Device name
 * - Setup completion flag
 * 
 * Uses ESP32 Preferences library (wrapper for NVS)
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>
#include <mbedtls/md.h>
#include "config.h"

// External Preferences object (defined in main file)
extern Preferences preferences;

// ==================== Function Prototypes ====================
void initStorage();
bool saveWiFiCredentials(String ssid, String password);
bool loadWiFiCredentials(String &ssid, String &password);
bool saveAdminCredentials(String username, String passwordHash);
bool loadAdminCredentials(String &username, String &passwordHash);
bool verifyAdminCredentials(String username, String password);
bool saveAPPassword(String password);
String loadAPPassword();
void updateStats(String motor);
void getStats(uint32_t &cam, uint32_t &ngo, uint32_t &thoc);
void clearAllData();
bool isSetupDone();
void setSetupDone(bool done);
String hashPassword(String password);
bool saveDeviceName(String name);
String loadDeviceName();

// ==================== Function Implementations ====================

/**
 * Initialize storage system
 * Opens Preferences with namespace and sets defaults if first boot
 */
void initStorage() {
  DEBUG_PRINTLN("Initializing storage (NVS)...");
  
  // Open Preferences with namespace "feeder"
  preferences.begin(PREFS_NAMESPACE, false); // false = read/write mode
  
  // Check if this is first boot
  if (!preferences.isKey(KEY_SETUP_DONE)) {
    DEBUG_PRINTLN("First boot detected - setting defaults");
    
    // Set default admin credentials
    String defaultPasswordHash = hashPassword(DEFAULT_ADMIN_PASSWORD);
    saveAdminCredentials(DEFAULT_ADMIN_USERNAME, defaultPasswordHash);
    
    // Set default AP password
    saveAPPassword(DEFAULT_AP_PASSWORD);
    
    // Initialize statistics to 0
    preferences.putUInt(KEY_CAM_COUNT, 0);
    preferences.putUInt(KEY_NGO_COUNT, 0);
    // THOC count removed - only 2 motors with DRV8833
    
    // Set default device name
    saveDeviceName(DEVICE_NAME);
    
    // Mark setup as not done (will be done after WiFi config)
    setSetupDone(false);
    
    DEBUG_PRINTLN("Defaults initialized");
  } else {
    DEBUG_PRINTLN("Storage initialized - existing data found");
  }
  
  // Note: We keep preferences open for the entire runtime
  // ESP32 handles this efficiently with lazy writes
}

/**
 * Save WiFi credentials to NVS
 * @param ssid WiFi SSID (max 32 chars)
 * @param password WiFi password (max 64 chars)
 * @return true if saved successfully
 */
bool saveWiFiCredentials(String ssid, String password) {
  if (ssid.length() > MAX_SSID_LENGTH || password.length() > MAX_PASSWORD_LENGTH) {
    DEBUG_PRINTLN("Error: WiFi credentials too long");
    return false;
  }
  
  preferences.putString(KEY_WIFI_SSID, ssid);
  preferences.putString(KEY_WIFI_PASS, password);
  
  DEBUG_PRINTF("WiFi credentials saved: SSID='%s'\n", ssid.c_str());
  return true;
}

/**
 * Load WiFi credentials from NVS
 * @param ssid Output SSID
 * @param password Output password
 * @return true if credentials exist and loaded
 */
bool loadWiFiCredentials(String &ssid, String &password) {
  if (!preferences.isKey(KEY_WIFI_SSID)) {
    DEBUG_PRINTLN("No WiFi credentials found");
    return false;
  }
  
  ssid = preferences.getString(KEY_WIFI_SSID, "");
  password = preferences.getString(KEY_WIFI_PASS, "");
  
  if (ssid.length() == 0) {
    DEBUG_PRINTLN("WiFi SSID is empty");
    return false;
  }
  
  DEBUG_PRINTF("WiFi credentials loaded: SSID='%s'\n", ssid.c_str());
  return true;
}

/**
 * Save admin credentials to NVS
 * @param username Admin username
 * @param passwordHash Hashed password (SHA256 hex)
 * @return true if saved successfully
 */
bool saveAdminCredentials(String username, String passwordHash) {
  if (username.length() > MAX_USERNAME_LENGTH) {
    DEBUG_PRINTLN("Error: Username too long");
    return false;
  }
  
  preferences.putString(KEY_ADMIN_USER, username);
  preferences.putString(KEY_ADMIN_PASS, passwordHash);
  
  DEBUG_PRINTF("Admin credentials saved: username='%s'\n", username.c_str());
  return true;
}

/**
 * Load admin credentials from NVS
 * @param username Output username
 * @param passwordHash Output password hash
 * @return true if loaded successfully
 */
bool loadAdminCredentials(String &username, String &passwordHash) {
  username = preferences.getString(KEY_ADMIN_USER, "");
  passwordHash = preferences.getString(KEY_ADMIN_PASS, "");
  
  if (username.length() == 0 || passwordHash.length() == 0) {
    DEBUG_PRINTLN("Error: Admin credentials not found");
    return false;
  }
  
  DEBUG_PRINTF("Admin credentials loaded: username='%s'\n", username.c_str());
  return true;
}

/**
 * Verify admin credentials
 * @param username Username to verify
 * @param password Plain text password to verify
 * @return true if credentials match
 */
bool verifyAdminCredentials(String username, String password) {
  String storedUsername, storedPasswordHash;
  
  if (!loadAdminCredentials(storedUsername, storedPasswordHash)) {
    return false;
  }
  
  // Compare username (case sensitive)
  if (username != storedUsername) {
    return false;
  }
  
  // Hash provided password and compare
  String providedPasswordHash = hashPassword(password);
  
  if (providedPasswordHash != storedPasswordHash) {
    return false;
  }
  
  return true;
}

/**
 * Save AP password to NVS
 * @param password AP password
 * @return true if saved successfully
 */
bool saveAPPassword(String password) {
  if (password.length() < 8 || password.length() > MAX_PASSWORD_LENGTH) {
    DEBUG_PRINTLN("Error: AP password must be 8-64 chars");
    return false;
  }
  
  preferences.putString(KEY_AP_PASS, password);
  DEBUG_PRINTLN("AP password saved");
  return true;
}

/**
 * Load AP password from NVS
 * @return AP password or default if not found
 */
String loadAPPassword() {
  String password = preferences.getString(KEY_AP_PASS, DEFAULT_AP_PASSWORD);
  DEBUG_PRINTLN("AP password loaded");
  return password;
}

/**
 * Update feed statistics
 * Increments counter for specified motor
 * @param motor Motor name ("cam" or "ngo")
 */
void updateStats(String motor) {
  motor.toLowerCase();
  
  if (motor == "cam") {
    uint32_t count = preferences.getUInt(KEY_CAM_COUNT, 0);
    preferences.putUInt(KEY_CAM_COUNT, count + 1);
    DEBUG_PRINTF("Stats updated: cam=%d\n", count + 1);
  } else if (motor == "ngo") {
    uint32_t count = preferences.getUInt(KEY_NGO_COUNT, 0);
    preferences.putUInt(KEY_NGO_COUNT, count + 1);
    DEBUG_PRINTF("Stats updated: ngo=%d\n", count + 1);
  }
  // "thoc" case removed - only 2 motors with DRV8833
}

/**
 * Get feed statistics
 * @param cam Output cam count
 * @param ngo Output ngo count
 */
void getStats(uint32_t &cam, uint32_t &ngo) {
  cam = preferences.getUInt(KEY_CAM_COUNT, 0);
  ngo = preferences.getUInt(KEY_NGO_COUNT, 0);
  
  DEBUG_PRINTF("Stats retrieved: cam=%d, ngo=%d\n", cam, ngo);
}

/**
 * Clear all stored data (Factory Reset)
 * Removes all keys from NVS
 */
void clearAllData() {
  DEBUG_PRINTLN("FACTORY RESET - Clearing all data...");
  
  preferences.clear();
  
  DEBUG_PRINTLN("All data cleared");
  
  // Reinitialize with defaults
  initStorage();
}

/**
 * Check if initial setup is complete
 * @return true if WiFi has been configured
 */
bool isSetupDone() {
  return preferences.getBool(KEY_SETUP_DONE, false);
}

/**
 * Set setup completion flag
 * @param done true if setup is complete
 */
void setSetupDone(bool done) {
  preferences.putBool(KEY_SETUP_DONE, done);
  DEBUG_PRINTF("Setup done flag set to: %s\n", done ? "true" : "false");
}

/**
 * Save custom device name
 * @param name Device name
 * @return true if saved successfully
 */
bool saveDeviceName(String name) {
  if (name.length() > MAX_USERNAME_LENGTH) {
    DEBUG_PRINTLN("Error: Device name too long");
    return false;
  }
  
  preferences.putString(KEY_DEVICE_NAME, name);
  DEBUG_PRINTF("Device name saved: '%s'\n", name.c_str());
  return true;
}

/**
 * Load device name
 * @return Device name or default if not set
 */
String loadDeviceName() {
  return preferences.getString(KEY_DEVICE_NAME, DEVICE_NAME);
}

/**
 * Hash password using SHA256
 * @param password Plain text password
 * @return Hex string of SHA256 hash (64 chars)
 */
String hashPassword(String password) {
  // Buffer for hash output (SHA256 = 32 bytes)
  byte hash[32];
  
  // Use mbedtls SHA256
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char*)password.c_str(), password.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);
  
  // Convert to hex string
  String hashHex = "";
  for (int i = 0; i < 32; i++) {
    char hex[3];
    sprintf(hex, "%02x", hash[i]);
    hashHex += hex;
  }
  
  DEBUG_PRINTF("Password hashed: %s... (%d chars)\n", hashHex.substring(0, 8).c_str(), hashHex.length());
  return hashHex;
}

/**
 * Get storage usage statistics
 * @return JSON string with storage info
 */
String getStorageInfo() {
  size_t usedEntries = preferences.freeEntries();
  
  String json = "{";
  json += "\"namespace\":\"" + String(PREFS_NAMESPACE) + "\",";
  json += "\"used_entries\":" + String(usedEntries) + ",";
  json += "\"wifi_configured\":" + String(preferences.isKey(KEY_WIFI_SSID) ? "true" : "false") + ",";
  json += "\"setup_done\":" + String(isSetupDone() ? "true" : "false");
  json += "}";
  
  return json;
}

#endif // STORAGE_H
