# 🧪 Testing Guide - ESP32 WiFi Feeder v2.0

## ✅ Testing Checklist

### **Phase 1: Compilation Test**

#### Test 1.1: Arduino IDE Compilation
- [ ] Mở Arduino IDE
- [ ] File → Open → `esp32_wifi_feeder_v2/esp32_wifi_feeder_v2.ino`
- [ ] Tools → Board → ESP32 Dev Module
- [ ] Tools → Port → (chọn COM port của ESP32)
- [ ] Click **Verify** (✓ icon)
- [ ] **Expected:** Compilation successful, no errors
- [ ] **Note memory usage:** Program storage space / Dynamic memory

**If compilation fails:**
- Check ArduinoJson v6.21.5 is installed (not v7)
- Check ESP32 board support is installed
- Read error messages in console

---

### **Phase 2: Upload & Boot Test**

#### Test 2.1: Upload Firmware
- [ ] Click **Upload** (→ icon) in Arduino IDE
- [ ] **Expected:** Upload successful
- [ ] LED on ESP32 should start blinking

#### Test 2.2: Serial Monitor Output
- [ ] Open Serial Monitor (Tools → Serial Monitor)
- [ ] Set baud rate to **115200**
- [ ] Press **EN** button on ESP32 to reset
- [ ] **Expected output:**

```
========================================
ESP32 WiFi Manager + Admin Portal
Chicken Feeder v2.0
========================================
Firmware: 2.0.0
Manufacturer: HoangHai Tech
========================================

Initializing hardware...
MAC Address: XX:XX:XX:XX:XX:XX
Pins initialized - All relays OFF
Motor pins initialized - All relays OFF
Hardware initialized
Initializing storage (NVS)...
First boot detected - setting defaults
Defaults initialized
Storage initialized - existing data found
Initializing WiFi Manager...
AP SSID will be: ChickenFeeder-XXXX
No saved WiFi credentials - starting AP mode
Starting Access Point mode...
✓ Access Point started!
  SSID: ChickenFeeder-XXXX
  Password: 12345678
  IP Address: 192.168.4.1
  Connect to this AP and navigate to http://192.168.4.1
HTTP server started
Ready at: http://192.168.4.1

========================================
System Ready!
========================================
```

**If output shows garbage:**
- Wrong baud rate (change to 115200)
- Press EN button to restart

**If WiFi credentials exist (not first boot):**
```
Found saved WiFi credentials
Connecting to WiFi: [SSID]
..........
✓ WiFi connected successfully!
  IP Address: 172.20.10.X
  Signal Strength: -XX dBm
  MAC Address: XX:XX:XX:XX:XX:XX
```

---

### **Phase 3: WiFi AP Mode Test**

#### Test 3.1: Connect to AP
- [ ] On phone/laptop, scan WiFi networks
- [ ] Find network: **ChickenFeeder-XXXX** (XXXX = last 4 digits from Serial Monitor)
- [ ] Connect with password: **12345678**
- [ ] **Expected:** Connected successfully

#### Test 3.2: LED Indicator
- [ ] **Expected:** LED blinking fast (~5 times/second) = AP mode
- [ ] If connected to WiFi: LED solid ON
- [ ] If connecting: LED slow blink (1/second)

#### Test 3.3: Access Web Interface
- [ ] Open browser on connected device
- [ ] Go to: `http://192.168.4.1`
- [ ] **Expected:** See basic home page with:
  - Device name: ChickenFeeder
  - Firmware: 2.0.0
  - Status: Online
  - IP: 192.168.4.1
  - Available endpoints list
  - Development mode notice

**If page doesn't load:**
- Check WiFi still connected to ChickenFeeder-XXXX
- Try `http://192.168.4.1/` (with trailing slash)
- Check Serial Monitor for errors

---

### **Phase 4: API Endpoints Test**

#### Test 4.1: Status Endpoint
**Method:** GET  
**URL:** `http://192.168.4.1/status`

**Expected JSON response:**
```json
{
  "success": true,
  "device": "ChickenFeeder",
  "firmware": "2.0.0",
  "wifi_state": "AP Mode",
  "wifi_connected": false,
  "ap_mode": true,
  "ap_ip": "192.168.4.1",
  "ap_ssid": "ChickenFeeder-XXXX",
  "uptime_seconds": 123,
  "free_heap": 250000,
  "statistics": {
    "cam": 0,
    "ngo": 0,
    "thoc": 0,
    "total": 0
  },
  "motors": [
    {"name": "cam", "pin": 25, "running": false},
    {"name": "ngo", "pin": 26, "running": false},
    {"name": "thoc", "pin": 27, "running": false}
  ]
}
```

**Test methods:**
- Browser: Just paste URL
- PowerShell: `Invoke-RestMethod -Uri "http://192.168.4.1/status"`
- curl: `curl http://192.168.4.1/status`

#### Test 4.2: Motor Control (Single Motor)
**Method:** POST  
**URL:** `http://192.168.4.1/feed`  
**Content-Type:** application/json  
**Body:**
```json
{
  "motor": "cam",
  "duration": 1000
}
```

**Test with PowerShell:**
```powershell
$body = @{motor="cam"; duration=1000} | ConvertTo-Json
Invoke-RestMethod -Uri "http://192.168.4.1/feed" -Method POST -ContentType "application/json" -Body $body
```

**Expected response:**
```json
{
  "success": true,
  "message": "Motor cam activated for 1000ms",
  "timestamp": 12345
}
```

**Expected behavior:**
- Serial Monitor shows: `Motor cam ON (Pin 25) for 1000 ms`
- After 1 second: `Motor cam OFF (completed)`
- If you have relay connected: relay clicks ON then OFF
- Statistics incremented: cam count = 1

**Check statistics:**
```powershell
Invoke-RestMethod -Uri "http://192.168.4.1/status"
```
Should show `"cam": 1`

#### Test 4.3: Motor Control (Multiple Motors)
**Body:**
```json
{
  "motors": ["cam", "ngo", "thoc"],
  "duration": 500
}
```

**Test:**
```powershell
$body = @{motors=@("cam","ngo","thoc"); duration=500} | ConvertTo-Json
Invoke-RestMethod -Uri "http://192.168.4.1/feed" -Method POST -ContentType "application/json" -Body $body
```

**Expected:**
- All 3 motors activate simultaneously
- Each runs for 500ms
- Statistics updated for all 3

#### Test 4.4: Emergency Stop
**Method:** POST  
**URL:** `http://192.168.4.1/stop`

**Test:**
```powershell
Invoke-RestMethod -Uri "http://192.168.4.1/stop" -Method POST
```

**Expected:**
- All running motors stop immediately
- Response: `{"success": true, "message": "All motors stopped"}`

---

### **Phase 5: Storage Persistence Test**

#### Test 5.1: Statistics Persistence
1. [ ] Activate motor several times
2. [ ] Check statistics via `/status`
3. [ ] Press **EN** button to restart ESP32
4. [ ] Wait for boot
5. [ ] Check `/status` again
6. [ ] **Expected:** Statistics retained (same values before restart)

#### Test 5.2: First Boot vs Normal Boot
**First boot (fresh ESP32 or after factory reset):**
- Serial shows "First boot detected"
- Default admin credentials set
- AP password = "12345678"
- Statistics = 0

**Normal boot:**
- Serial shows "Storage initialized - existing data found"
- Saved settings loaded

---

### **Phase 6: WiFi Manager Test** (If you have WiFi credentials saved)

#### Test 6.1: Save WiFi Credentials (Manual via Serial)
Currently no web UI to save WiFi (Task 4 not done yet).

**Temporary test via code modification:**
Add to `setup()` after `initStorage()`:
```cpp
// TEST: Save WiFi credentials
saveWiFiCredentials("Your-WiFi-SSID", "Your-WiFi-Password");
setSetupDone(true);
```

Upload, then restart.

#### Test 6.2: Auto-Connect Test
- [ ] ESP32 should connect to saved WiFi
- [ ] Serial shows connection success with IP address
- [ ] LED solid ON
- [ ] Access web via new IP: `http://[ESP32-IP]`

#### Test 6.3: Auto-Reconnect Test
- [ ] While ESP32 connected, turn off WiFi router
- [ ] **Expected:** LED goes OFF, then slow blink
- [ ] Serial shows reconnection attempts
- [ ] Turn router back on
- [ ] **Expected:** ESP32 reconnects automatically

#### Test 6.4: Fallback to AP Mode
- [ ] Save wrong WiFi credentials (bad password)
- [ ] Restart ESP32
- [ ] **Expected:** After 3 connection attempts (~30 seconds), switches to AP mode
- [ ] LED fast blink
- [ ] Can connect to ChickenFeeder-XXXX AP again

---

## 📊 Test Results Template

Copy this and fill in results:

```
=== ESP32 WiFi Feeder v2.0 Test Results ===
Date: ___________
Tester: ___________

✅ = Pass | ❌ = Fail | ⚠️ = Partial

Phase 1: Compilation
[ ] Test 1.1: Compilation - ___ KB program, ___ KB RAM

Phase 2: Upload & Boot
[ ] Test 2.1: Upload successful
[ ] Test 2.2: Serial output correct

Phase 3: WiFi AP Mode
[ ] Test 3.1: Can connect to AP ChickenFeeder-____
[ ] Test 3.2: LED blinking fast
[ ] Test 3.3: Web page loads at 192.168.4.1

Phase 4: API Endpoints
[ ] Test 4.1: GET /status works
[ ] Test 4.2: POST /feed (single motor)
[ ] Test 4.3: POST /feed (multiple motors)
[ ] Test 4.4: POST /stop

Phase 5: Storage Persistence
[ ] Test 5.1: Statistics persist after restart

Phase 6: WiFi Manager (if tested)
[ ] Test 6.1: Save WiFi credentials
[ ] Test 6.2: Auto-connect works
[ ] Test 6.3: Auto-reconnect works
[ ] Test 6.4: Fallback to AP mode

=== Issues Found ===
(list any problems)

=== Notes ===
(any observations)
```

---

## 🐛 Common Issues & Solutions

### Issue: Compilation error "config.h not found"
**Solution:** Make sure all files are in same folder: `esp32_wifi_feeder_v2/`

### Issue: "Preferences.h not found"
**Solution:** Select ESP32 board (not Arduino board)

### Issue: Serial Monitor shows garbage
**Solution:** Set baud rate to 115200

### Issue: Can't find ChickenFeeder WiFi
**Solution:** 
- Check Serial Monitor for actual SSID
- Restart ESP32 (EN button)
- Check LED blinking fast (= AP mode)

### Issue: Web page times out
**Solution:**
- Confirm connected to ChickenFeeder-XXXX AP
- Try 192.168.4.1 (not https)
- Check Serial for errors

### Issue: Motor command returns error
**Solution:**
- Check Content-Type header: application/json
- Check JSON syntax (use quotes for strings)
- Motor names: "cam", "ngo", "thoc" (lowercase)

---

## ✅ Success Criteria

**Minimum to pass:**
- ✅ Compiles without errors
- ✅ Creates AP mode on first boot
- ✅ Web interface accessible at 192.168.4.1
- ✅ GET /status returns valid JSON
- ✅ POST /feed activates motors
- ✅ Statistics increment and persist

**Full pass:**
- ✅ All Phase 1-5 tests pass
- ✅ LED indicators work correctly
- ✅ Storage persistence verified
- ✅ No memory leaks (heap stable over time)

---

**Next:** After testing Tasks 1-3, continue with Task 4 (Setup Portal web UI)
