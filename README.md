# 🐔 TikTok Live Chicken Feeder

Hệ thống cho gà ăn tự động dựa trên TikTok Live gifts - sử dụng ESP32 WiFi + Flask webhook server.

**Version:** 2.0 (WiFi Manager + Web Admin)  
**Hardware:** ESP32 + 3 relay modules + 3 DC motors  
**Status:** 🟢 In Development

---

## 📋 Overview

Khi đang live TikTok, người xem tặng quà sẽ tự động kích hoạt máng cho gà ăn qua:
- **Tikfinity Webhook** → **Flask Server** → **ESP32 WiFi** → **3 Motors (Cám/Ngô/Thóc)**

### Version 2.0 Features:
- ✅ **WiFi Manager:** Setup WiFi qua web (không cần code)
- ✅ **Web Admin Panel:** Quản lý thiết bị từ xa
- ✅ **Authentication:** Bảo mật với login
- ✅ **Statistics:** Theo dõi số lần xả thóc
- ✅ **Mobile-Friendly UI:** Responsive design
- ✅ **Production Ready:** Sẵn sàng sản xuất hàng loạt

---

## 🗂️ Project Structure

```
tiktok-live-connector/
├── docs/                          # Documentation
│   ├── IMPLEMENTATION_PLAN.md    # Chi tiết implementation plan
│   └── project-overview-vi.md    # Project overview (Vietnamese)
│
├── esp32_wifi_feeder/            # Version 1.0 (hardcoded WiFi)
│   └── esp32_wifi_feeder.ino
│
├── esp32_wifi_feeder_v2/         # Version 2.0 (WiFi Manager + Admin)
│   ├── esp32_wifi_feeder_v2.ino # Main file
│   ├── config.h                  # Configuration
│   ├── wifi_manager.h            # WiFi management
│   ├── web_admin.h               # Admin panel
│   ├── web_setup.h               # Setup portal
│   ├── storage.h                 # Data storage
│   ├── auth.h                    # Authentication
│   └── motor_control.h           # Motor control
│
├── flask-webhook-test.py         # Flask webhook server
├── test-esp32.py                 # ESP32 test script
├── test-esp32-simple.ps1         # PowerShell test script
└── README.md                     # This file
```

---

## 🚀 Quick Start

### 1. Setup ESP32 (Version 2.0)

**First time:**
1. Upload code `esp32_wifi_feeder_v2.ino` to ESP32
2. ESP32 will create WiFi Access Point: `ChickenFeeder-XXXX`
3. Connect phone/laptop to that AP (password: `12345678`)
4. Browser auto-opens setup page at `http://192.168.4.1`
5. Select your WiFi network and enter password
6. ESP32 connects and shows its IP address

**Admin Panel:**
1. Connect to same WiFi as ESP32
2. Open browser: `http://<ESP32-IP>/admin`
3. Login: `admin` / `admin123`
4. Manage device, change settings, view statistics

### 2. Setup Flask Webhook Server

```powershell
# Install dependencies
pip install flask requests

# Run server
python flask-webhook-test.py
```

Server runs on: `http://localhost:5000`

### 3. Configure Tikfinity

Set webhook URL in Tikfinity:
```
http://localhost:5000/webhook
```

For external access, use ngrok:
```powershell
ngrok http 5000
```

---

## 🎁 Gift Mapping

| Gift | Motor | Cycles | Duration |
|------|-------|--------|----------|
| Rose | Cám | 1 | 300ms |
| Heart | Ngô | 2 | 600ms |
| Donut | All 3 | 4 each | 1200ms |

---

## 🔧 Hardware Setup

### Components:
- **ESP32 Dev Board** (WiFi enabled)
- **3x Relay Modules** (Active LOW, 5V)
- **3x DC Motors** (12V for feeders)
- **Power Supply** (12V for motors, 5V for ESP32)

### Wiring:
```
ESP32 GPIO 25 → Relay 1 (Cám)
ESP32 GPIO 26 → Relay 2 (Ngô)
ESP32 GPIO 27 → Relay 3 (Thóc)
ESP32 GPIO 2  → Status LED (built-in)
ESP32 GND     → Common GND
```

---

## 📡 API Endpoints

### Public Endpoints:
- `GET /` - Home page with device info
- `POST /feed` - Motor control (JSON)
- `GET /status` - Device status (JSON)

### Setup Portal (AP Mode):
- `GET /` - Setup page
- `GET /scan` - Scan WiFi networks
- `POST /connect` - Save WiFi credentials

### Admin Panel (Authenticated):
- `GET /admin` - Dashboard
- `GET /admin/api/status` - Status data
- `GET /admin/wifi/scan` - Scan networks
- `POST /admin/wifi/save` - Update WiFi
- `POST /admin/system/restart` - Restart device
- `POST /admin/system/reset` - Factory reset

---

## 🧪 Testing

### Test ESP32 connection:
```powershell
# Python test script
python test-esp32.py

# PowerShell test script
powershell -ExecutionPolicy Bypass -File test-esp32-simple.ps1
```

### Test motor control:
```powershell
# Using curl
curl.exe -X POST http://<ESP32-IP>/feed -H "Content-Type: application/json" -d "{\"motor\":\"cam\",\"duration\":1000}"
```

### Test Flask webhook:
```
http://localhost:5000/gift_trigger?giftName=Rose&user=TestUser&repeatCount=1
```

---

## 📖 Documentation

Full documentation available in `docs/` folder:
- [Implementation Plan](docs/IMPLEMENTATION_PLAN.md) - Detailed development plan
- [Project Overview (Vietnamese)](docs/project-overview-vi.md) - Original project docs

---

## 🔒 Default Credentials

**AP Mode (First Setup):**
- SSID: `ChickenFeeder-XXXX` (XXXX = last 4 digits of MAC)
- Password: `12345678`
- IP: `192.168.4.1`

**Admin Panel:**
- Username: `admin`
- Password: `admin123`

⚠️ **Change these in production!**

---

## 📊 Current Status

### Version 1.0 (Completed)
- ✅ ESP32 WiFi connection (hardcoded)
- ✅ Flask webhook server
- ✅ Motor control (3 relays)
- ✅ TikTok gift integration
- ✅ Queue system for concurrent gifts

### Version 2.0 (In Progress)
- ✅ Project structure created
- ✅ Configuration system (config.h)
- ⏳ WiFi Manager (in progress)
- ⏳ Web Admin Panel (in progress)
- ⏳ Authentication system (in progress)
- ⏳ Statistics tracking (in progress)

Progress: **10%** complete (Task 1/10)

---

## 🛠️ Development

### Arduino IDE Setup:
1. Install ESP32 board support (via Board Manager)
2. Install ArduinoJson library v6.21.5 (via Library Manager)
3. Select board: "ESP32 Dev Module"
4. Set upload speed: 115200
5. Compile and upload

### Serial Monitor:
- Baud rate: **115200**
- Shows WiFi connection status, IP address, and debug logs

---

## 🤝 Contributing

This is a personal project for automated chicken feeding during TikTok live streams.

---

## 📝 License

Private project - not for public distribution.

---

## 👤 Author

**Hoang Hai**  
Project: TikTok Live Automated Chicken Feeder  
Hardware: ESP32 + 3 Motor System  
Date: November 2024

---

## 📞 Support

For issues or questions:
1. Check Serial Monitor for ESP32 debug logs
2. Check Flask server console for webhook logs
3. Check browser console for web UI errors
4. Review documentation in `docs/` folder

---

**Last Updated:** 2024-11-10
