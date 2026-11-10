import time
import queue
import threading
import requests
import json
from flask import Flask, request, jsonify

app = Flask(__name__)

# ============================================
# KẾT NỐI ESP32 QUA WiFi
# ============================================
ESP32_IP = "172.20.10.2"  # IP của ESP32
ESP32_URL = f"http://{ESP32_IP}/feed"

esp32_connected = False

def connect_esp32():
    """Kiểm tra kết nối HTTP với ESP32"""
    global esp32_connected
    try:
        response = requests.get(f"http://{ESP32_IP}/", timeout=3)
        if response.status_code == 200:
            print(f"✓ Đã kết nối ESP32 WiFi: {ESP32_IP}")
            esp32_connected = True
            return True
    except Exception as e:
        print(f"✗ Không kết nối ESP32: {e}")
        print("  → Chạy test mode (không có phần cứng)")
        esp32_connected = False
        return False

def send_to_esp32(duration_ms):
    """Gửi lệnh FEED đến ESP32 qua HTTP"""
    if not esp32_connected:
        print(f"  ⚠️ Test mode: Would send FEED:{duration_ms}")
        return False
    
    try:
        # Gửi POST request đến ESP32
        payload = {
            "motors": ["cam", "ngo", "thoc"],  # Hoặc chỉ 1 motor
            "duration": duration_ms
        }
        
        response = requests.post(ESP32_URL, json=payload, timeout=5)
        
        if response.status_code == 200:
            result = response.json()
            print(f"  → ESP32: {result.get('message', 'OK')}")
            return True
        else:
            print(f"  ✗ ESP32 error: {response.status_code}")
            return False
            
    except Exception as e:
        print(f"  ✗ Lỗi gửi lệnh: {e}")
        return False

# ============================================
# 1. Khởi tạo 3 hàng đợi độc lập cho 3 máy trục vít
# ============================================
queues = {
    "cam": queue.Queue(),
    "ngo": queue.Queue(),
    "thoc": queue.Queue()
}

# ============================================
# 2. Bảng định tuyến quà tặng theo thiết kế
# ============================================
GIFT_CONFIG = {
    "rose":  {"targets": ["cam"],                 "cycles_per_gift": 1},
    "rosa":  {"targets": ["cam"],                 "cycles_per_gift": 1},
    "heart": {"targets": ["ngo"],                 "cycles_per_gift": 2},
    "tim":   {"targets": ["ngo"],                 "cycles_per_gift": 2},
    "donut": {"targets": ["cam", "ngo", "thoc"], "cycles_per_gift": 4} # Bão quà: quay cả 3 máy
}

def feeder_worker(feeder_name):
    """Luồng worker riêng biệt cho từng máy, chạy ngầm không block nhau"""
    q = queues[feeder_name]
    while True:
        task = q.get()
        user = task["user"]
        gift = task["gift"]
        cycles = task["cycles"]

        print(f"\n=============================================")
        print(f"[{feeder_name.upper()}] BẮT ĐẦU XẢ CHO: {user}")
        print(f"-> Quà: {gift} | Số chu kỳ motor quay: {cycles} nhịp")
        print(f"=============================================")

        # Tính thời gian cho ESP32 (mỗi chu kỳ = 300ms)
        duration_ms = cycles * 300

        # Gửi lệnh đến ESP32
        send_to_esp32(duration_ms)

        # Giả lập trục vít quay theo từng chu kỳ (để log)
        for i in range(1, cycles + 1):
            print(f"  -> [{feeder_name.upper()}] Motor đang quay nhịp {i}/{cycles} (đùn thức ăn)...")
            time.sleep(0.3)  # Giả lập thời gian motor quay 300ms

        print(f"[{feeder_name.upper()}] HOÀN THÀNH lượt của {user}.\n")
        time.sleep(0.2)  # Nghỉ đệm 200ms trước khi nhận task tiếp
        q.task_done()

# Khởi động 3 luồng Worker chạy song song
for name in ["cam", "ngo", "thoc"]:
    threading.Thread(target=feeder_worker, args=(name,), daemon=True).start()

@app.route('/gift_trigger', methods=['GET', 'POST'])
def receive_gift():
    # Lấy thông số từ query params do Streamer.bot / TikFinity gửi sang
    gift_name = request.args.get('giftName', 'Rose')
    user = request.args.get('user', 'Ẩn danh')
    coins = request.args.get('coins', '1')

    # ÉP KIỂU VÀ ĐỌC BIẾN REPEATCOUNT/GIFTCOUNT
    # TikFinity dùng 'giftCount', Streamer.bot dùng 'repeatCount'
    try:
        repeat_count = int(request.args.get('repeatCount') or request.args.get('giftCount') or 1)
    except ValueError:
        repeat_count = 1

    # Tìm cấu hình quà trong từ điển
    key = gift_name.strip().lower()
    matched = None
    for k, conf in GIFT_CONFIG.items():
        if k in key:
            matched = conf
            break

    # Mặc định xả cám 1 nhịp nếu là quà lạ
    if not matched:
        matched = {"targets": ["cam"], "cycles_per_gift": 1}

    # Giới hạn an toàn tối đa 20 chu kỳ cho một lần xả
    total_cycles = min(repeat_count * matched["cycles_per_gift"], 20)

    # Đẩy tác vụ vào hàng đợi tương ứng (Fan-out nếu là quà Donut)
    for target in matched["targets"]:
        queues[target].put({
            "user": user,
            "gift": gift_name,
            "cycles": total_cycles
        })

    return jsonify({
        "status": "success",
        "user": user,
        "gift": gift_name,
        "repeatCount": repeat_count,
        "allocated_cycles": total_cycles,
        "targets": matched["targets"]
    }), 200

@app.route('/webhook', methods=['GET', 'POST'])
def webhook():
    """Route tương thích với Tikfinity webhook"""
    print(f"\n📨 WEBHOOK RECEIVED")

    # Hỗ trợ cả POST JSON
    if request.method == 'POST' and request.is_json:
        data = request.json
        gift_name = data.get('giftName', 'Rose')
        user = data.get('user', 'Ẩn danh')
        repeat_count = int(data.get('repeatCount', 1))

        # Xử lý giống gift_trigger
        key = gift_name.strip().lower()
        matched = None
        for k, conf in GIFT_CONFIG.items():
            if k in key:
                matched = conf
                break

        if not matched:
            matched = {"targets": ["cam"], "cycles_per_gift": 1}

        total_cycles = min(repeat_count * matched["cycles_per_gift"], 20)

        for target in matched["targets"]:
            queues[target].put({
                "user": user,
                "gift": gift_name,
                "cycles": total_cycles
            })

        return jsonify({
            "status": "success",
            "user": user,
            "gift": gift_name,
            "repeatCount": repeat_count,
            "allocated_cycles": total_cycles,
            "targets": matched["targets"]
        }), 200
    else:
        # GET hoặc form data → forward to gift_trigger
        return receive_gift()

@app.route('/', methods=['GET'])
def home():
    esp32_status = "✓ Connected" if esp32_connected else "✗ Not Connected"
    esp32_color = "#10b981" if esp32_connected else "#ef4444"

    return f"""
    <html>
    <head>
        <title>TikTok Live Chicken Feeder - 3 Motors</title>
        <meta charset="UTF-8">
    </head>
    <body style="font-family: Arial; padding: 50px; background: #f5f5f5;">
        <div style="background: white; padding: 30px; border-radius: 10px; max-width: 700px; margin: 0 auto;">
            <h1 style="color: #667eea;">🐔 TikTok Live Chicken Feeder - 3 Motors</h1>
            <p><span style="background: #10b981; color: white; padding: 5px 15px; border-radius: 20px; font-weight: bold;">✓ SERVER RUNNING</span></p>

            <h2>📟 ESP32 Status:</h2>
            <p><span style="background: {esp32_color}; color: white; padding: 5px 15px; border-radius: 20px; font-weight: bold;">{esp32_status}</span></p>
            <p style="color: #666;">WiFi IP: {ESP32_IP} | Connection: HTTP</p>

            <h2>🎯 Gift Mapping:</h2>
            <ul style="color: #666;">
                <li><strong>Rose</strong> → Máy Cám (1 nhịp)</li>
                <li><strong>Heart</strong> → Máy Ngô (2 nhịp)</li>
                <li><strong>Donut</strong> → CẢ 3 MÁY (4 nhịp mỗi máy)</li>
            </ul>

            <h2>📡 Webhook URL:</h2>
            <div style="background: #f9fafb; padding: 15px; border-radius: 5px; font-family: monospace; margin: 10px 0;">
                http://localhost:5000/webhook
            </div>
            <div style="background: #f9fafb; padding: 15px; border-radius: 5px; font-family: monospace; margin: 10px 0;">
                http://localhost:5000/gift_trigger
            </div>

            <h2>🧪 Test Links:</h2>
            <a href="/gift_trigger?giftName=Rose&user=TestUser&repeatCount=3"
               style="display: inline-block; background: #667eea; color: white; padding: 10px 20px;
                      text-decoration: none; border-radius: 5px; margin: 5px;">
                Test Rose x3
            </a>
            <a href="/gift_trigger?giftName=Heart&user=TestUser&repeatCount=2"
               style="display: inline-block; background: #ef4444; color: white; padding: 10px 20px;
                      text-decoration: none; border-radius: 5px; margin: 5px;">
                Test Heart x2
            </a>
            <a href="/gift_trigger?giftName=Donut&user=TestUser&repeatCount=1"
               style="display: inline-block; background: #f59e0b; color: white; padding: 10px 20px;
                      text-decoration: none; border-radius: 5px; margin: 5px;">
                Test Donut (All)
            </a>
        </div>
    </body>
    </html>
    """

if __name__ == '__main__':
    print("\n" + "="*50)
    print("🐔 TIKTOK LIVE CHICKEN FEEDER - 3 MOTORS + ESP32")
    print("="*50)
    print("\n🔌 Đang kết nối ESP32 qua WiFi...")
    connect_esp32()
    print("\n📡 Webhook URL:")
    print("   http://localhost:5000/webhook")
    print("   http://localhost:5000/gift_trigger")
    print("\n🌐 Dashboard:")
    print("   http://localhost:5000")
    print("\n✓ Server starting...\n")

    app.run(host='0.0.0.0', port=5000, debug=False)
