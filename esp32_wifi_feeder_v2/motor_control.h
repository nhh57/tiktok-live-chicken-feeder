/*
 * Motor Control Module
 * 
 * Handles 3 relay-controlled DC motors for chicken feeder
 * - Cám (corn feed) - GPIO 25
 * - Ngô (maize) - GPIO 26
 * - Thóc (rice) - GPIO 27
 * 
 * Features:
 * - Non-blocking motor control
 * - Simultaneous multi-motor support
 * - Safety timeout protection
 * - Active LOW relay support
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "config.h"

// ==================== Motor Control Structure ====================
struct MotorControl {
  int pin;
  unsigned long startTime;
  unsigned long duration;
  bool isRunning;
  String name;
};

// Global motor instances
MotorControl motors[3] = {
  {RELAY_CAM, 0, 0, false, "cam"},
  {RELAY_NGO, 0, 0, false, "ngo"},
  {RELAY_THOC, 0, 0, false, "thoc"}
};

// ==================== Function Prototypes ====================
void initializeMotors();
bool activateMotor(String motorName, int duration);
void updateMotors();
void stopAllMotors();
bool isAnyMotorRunning();
int getMotorIndex(String motorName);

// ==================== Function Implementations ====================

/**
 * Initialize motor pins
 * Sets all relay pins as outputs and turns them OFF (HIGH for active LOW relays)
 */
void initializeMotors() {
  DEBUG_PRINTLN("Initializing motor control pins...");
  
  // Configure relay pins as outputs (relays are active LOW)
  pinMode(RELAY_CAM, OUTPUT);
  pinMode(RELAY_NGO, OUTPUT);
  pinMode(RELAY_THOC, OUTPUT);

  // Turn off all relays (HIGH = OFF for active LOW relays)
  digitalWrite(RELAY_CAM, HIGH);
  digitalWrite(RELAY_NGO, HIGH);
  digitalWrite(RELAY_THOC, HIGH);

  DEBUG_PRINTLN("Motor pins initialized - All relays OFF");
}

/**
 * Activate a specific motor
 * @param motorName Name of motor ("cam", "ngo", or "thoc")
 * @param duration Duration in milliseconds
 * @return true if motor found and activated, false otherwise
 */
bool activateMotor(String motorName, int duration) {
  motorName.toLowerCase();
  
  // Validate duration
  if (duration < MIN_DURATION) {
    duration = MIN_DURATION;
    DEBUG_PRINTF("Duration too short, set to minimum: %d ms\n", MIN_DURATION);
  }
  if (duration > MAX_DURATION) {
    duration = MAX_DURATION;
    DEBUG_PRINTF("Duration too long, set to maximum: %d ms\n", MAX_DURATION);
  }
  
  // Find and activate motor
  for (int i = 0; i < 3; i++) {
    if (motors[i].name == motorName) {
      motors[i].startTime = millis();
      motors[i].duration = duration;
      motors[i].isRunning = true;
      digitalWrite(motors[i].pin, LOW); // Active LOW relay - turn ON
      
      DEBUG_PRINTF("Motor %s ON (Pin %d) for %d ms\n", 
                    motors[i].name.c_str(), motors[i].pin, duration);
      return true;
    }
  }
  
  DEBUG_PRINTF("Motor not found: %s\n", motorName.c_str());
  return false;
}

/**
 * Update motor states (non-blocking)
 * Call this in main loop to check if motors should be turned off
 */
void updateMotors() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < 3; i++) {
    if (motors[i].isRunning) {
      // Check if duration has elapsed
      if (currentTime - motors[i].startTime >= motors[i].duration) {
        digitalWrite(motors[i].pin, HIGH); // Turn OFF (active LOW)
        motors[i].isRunning = false;
        DEBUG_PRINTF("Motor %s OFF (completed)\n", motors[i].name.c_str());
      }
    }
  }
}

/**
 * Emergency stop all motors immediately
 */
void stopAllMotors() {
  DEBUG_PRINTLN("EMERGENCY STOP - All motors");
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(motors[i].pin, HIGH); // Turn OFF
    motors[i].isRunning = false;
  }
  
  DEBUG_PRINTLN("All motors stopped");
}

/**
 * Check if any motor is currently running
 * @return true if at least one motor is running
 */
bool isAnyMotorRunning() {
  for (int i = 0; i < 3; i++) {
    if (motors[i].isRunning) {
      return true;
    }
  }
  return false;
}

/**
 * Get motor index by name
 * @param motorName Name of motor
 * @return motor index (0-2) or -1 if not found
 */
int getMotorIndex(String motorName) {
  motorName.toLowerCase();
  for (int i = 0; i < 3; i++) {
    if (motors[i].name == motorName) {
      return i;
    }
  }
  return -1;
}

/**
 * Get motor status as JSON-friendly string
 * @return JSON string with all motor statuses
 */
String getMotorsStatusJSON() {
  String json = "[";
  
  for (int i = 0; i < 3; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"name\":\"" + motors[i].name + "\",";
    json += "\"pin\":" + String(motors[i].pin) + ",";
    json += "\"running\":" + String(motors[i].isRunning ? "true" : "false");
    
    if (motors[i].isRunning) {
      unsigned long elapsed = millis() - motors[i].startTime;
      unsigned long remaining = (elapsed < motors[i].duration) ? 
                               (motors[i].duration - elapsed) : 0;
      json += ",\"remaining_ms\":" + String(remaining);
    }
    
    json += "}";
  }
  
  json += "]";
  return json;
}

#endif // MOTOR_CONTROL_H
