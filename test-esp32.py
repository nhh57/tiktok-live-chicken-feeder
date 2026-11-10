#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple ESP32 Test Script
Tests connection and motor control
"""

import requests
import json

ESP32_IP = "172.20.10.2"
ESP32_URL = f"http://{ESP32_IP}"

print("\n" + "="*50)
print("ESP32 CONNECTION TEST")
print("="*50)

# Test 1: GET /
print("\n[1] Testing HTTP server (GET /)...")
try:
    response = requests.get(f"{ESP32_URL}/", timeout=5)
    if response.status_code == 200:
        print("✓ HTTP server is running!")
        print(f"  Status: {response.status_code}")
    else:
        print(f"✗ Unexpected status: {response.status_code}")
except Exception as e:
    print(f"✗ Connection failed: {e}")
    print("\nCheck:")
    print("  - ESP32 is powered on")
    print("  - Serial Monitor shows 'WiFi connected'")
    print("  - Computer connected to 'Hoang Hai's iPhone'")
    exit(1)

# Test 2: POST /feed (single motor)
print("\n[2] Testing motor control (cam motor)...")
try:
    payload = {
        "motor": "cam",
        "duration": 1000
    }
    
    response = requests.post(
        f"{ESP32_URL}/feed",
        json=payload,
        headers={"Content-Type": "application/json"},
        timeout=5
    )
    
    result = response.json()
    print(f"✓ Motor command sent!")
    print(f"  Response: {json.dumps(result, indent=2)}")
    print("\n  -> Check Serial Monitor for ESP32 logs")
    print("  -> Check if LED/relay activates on ESP32")
    
except Exception as e:
    print(f"✗ Motor command failed: {e}")

# Test 3: POST /feed (multiple motors)
print("\n[3] Testing multiple motors (cam + ngo)...")
try:
    payload = {
        "motors": ["cam", "ngo"],
        "duration": 500
    }
    
    response = requests.post(
        f"{ESP32_URL}/feed",
        json=payload,
        headers={"Content-Type": "application/json"},
        timeout=5
    )
    
    result = response.json()
    print(f"✓ Multiple motors command sent!")
    print(f"  Response: {json.dumps(result, indent=2)}")
    
except Exception as e:
    print(f"✗ Multiple motors failed: {e}")

# Test 4: GET /status
print("\n[4] Testing status endpoint...")
try:
    response = requests.get(f"{ESP32_URL}/status", timeout=5)
    result = response.json()
    print(f"✓ Status retrieved!")
    print(f"  Response: {json.dumps(result, indent=2)}")
except Exception as e:
    print(f"✗ Status failed: {e}")

print("\n" + "="*50)
print("TEST COMPLETE")
print("="*50)
print("\nOpen Serial Monitor (Arduino IDE, baud 115200)")
print("to see detailed ESP32 logs\n")
