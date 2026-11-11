/*
 * Motor Control Module
 * 
 * Handles 2 DRV8833 H-Bridge controlled DC motors for chicken feeder
 * - Motor 1: Cám (corn feed) - GPIO 18/19 (IN1/IN2)
 * - Motor 2: Ngô (corn) - GPIO 16/17 (IN3/IN4)
 * 
 * Features:
 * - Non-blocking motor control
 * - Simultaneous multi-motor support
 * - Safety timeout protection
 * - H-Bridge forward direction control
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "config.h"

// ==================== Motor Control Structure ====================
struct MotorControl {
  int pinA;          // INA (IN1 or IN3) - H-bridge input A
  int pinB;          // INB (IN2 or IN4) - H-bridge input B
  unsigned long startTime;
  unsigned long duration;
  bool isRunning;
  String name;
};

// Global motor instances - ONLY 2 MOTORS for DRV8833
MotorControl motors[2] = {
  {MOTOR1_IN1, MOTOR1_IN2, 0, 0, false, "cam"},   // Motor 1 (Cám)
  {MOTOR2_IN3, MOTOR2_IN4, 0, 0, false, "ngo"}    // Motor 2 (Ngô)
  // Motor 3 (thoc) removed - only 2 motors with DRV8833 hardware
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
 * Sets all DRV8833 pins as outputs and stops motors (both pins LOW)
 */
void initializeMotors() {
  DEBUG_PRINTLN("Initializing motor control pins (DRV8833 H-Bridge)...");
  
  // Configure Motor 1 pins (Cám)
  pinMode(MOTOR1_IN1, OUTPUT);
  pinMode(MOTOR1_IN2, OUTPUT);
  
  // Configure Motor 2 pins (Ngô)
  pinMode(MOTOR2_IN3, OUTPUT);
  pinMode(MOTOR2_IN4, OUTPUT);

  // Stop all motors (both pins LOW = brake/stop for H-bridge)
  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, LOW);
  digitalWrite(MOTOR2_IN3, LOW);
  digitalWrite(MOTOR2_IN4, LOW);

  DEBUG_PRINTLN("Motor pins initialized - DRV8833 ready, all motors stopped");
}

/**
 * Activate a specific motor
 * @param motorName Name of motor ("cam" or "ngo")
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
  for (int i = 0; i < 2; i++) {  // Only 2 motors with DRV8833
    if (motors[i].name == motorName) {
      motors[i].startTime = millis();
      motors[i].duration = duration;
      motors[i].isRunning = true;
      
      // H-bridge forward direction: INA=HIGH, INB=LOW
      digitalWrite(motors[i].pinA, HIGH);
      digitalWrite(motors[i].pinB, LOW);
      
      DEBUG_PRINTF("Motor %s FORWARD (Pin A=%d HIGH, Pin B=%d LOW) for %d ms\n", 
                    motors[i].name.c_str(), motors[i].pinA, motors[i].pinB, duration);
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
  
  for (int i = 0; i < 2; i++) {  // Only 2 motors
    if (motors[i].isRunning) {
      // Check if duration has elapsed
      if (currentTime - motors[i].startTime >= motors[i].duration) {
        // Stop motor: both pins LOW (brake/stop)
        digitalWrite(motors[i].pinA, LOW);
        digitalWrite(motors[i].pinB, LOW);
        motors[i].isRunning = false;
        DEBUG_PRINTF("Motor %s STOPPED (completed)\n", motors[i].name.c_str());
      }
    }
  }
}

/**
 * Emergency stop all motors immediately
 */
void stopAllMotors() {
  DEBUG_PRINTLN("EMERGENCY STOP - All motors");
  
  for (int i = 0; i < 2; i++) {  // Only 2 motors
    // Stop: both pins LOW
    digitalWrite(motors[i].pinA, LOW);
    digitalWrite(motors[i].pinB, LOW);
    motors[i].isRunning = false;
  }
  
  DEBUG_PRINTLN("All motors stopped");
}

/**
 * Check if any motor is currently running
 * @return true if at least one motor is running
 */
bool isAnyMotorRunning() {
  for (int i = 0; i < 2; i++) {  // Only 2 motors
    if (motors[i].isRunning) {
      return true;
    }
  }
  return false;
}

/**
 * Get motor index by name
 * @param motorName Name of motor
 * @return motor index (0-1) or -1 if not found
 */
int getMotorIndex(String motorName) {
  motorName.toLowerCase();
  for (int i = 0; i < 2; i++) {  // Only 2 motors
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
  
  for (int i = 0; i < 2; i++) {  // Only 2 motors
    if (i > 0) json += ",";
    json += "{";
    json += "\"name\":\"" + motors[i].name + "\",";
    json += "\"pinA\":" + String(motors[i].pinA) + ",";
    json += "\"pinB\":" + String(motors[i].pinB) + ",";
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
