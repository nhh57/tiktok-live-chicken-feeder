# TÀI LIỆU ĐẶC TẢ NGHIỆP VỤ & KIẾN TRÚC HỆ THỐNG
## HỆ THỐNG CHO GÀ ĂN TỰ ĐỘNG THEO QUÀ TIKTOK LIVE (AUGER FEEDER)

---

## 1. TỔNG QUAN HỆ THỐNG (SYSTEM OVERVIEW)

Hệ thống cho gà ăn tự động theo thời gian thực (Interactive Real-time Feeder) tiếp nhận sự kiện quà tặng từ phòng livestream TikTok, chuyển đổi thành lệnh điều khiển cơ khí chính xác để phân phối 3 loại thức ăn riêng biệt:
* **Máng 1:** Cám viên
* **Máng 2:** Hạt bắp (Ngô)
* **Máng 3:** Thóc lúa

### 1.1 Mục tiêu nghiệp vụ (Business Goals)
1. **Phản hồi tức thì (Instant Gratification):** Người xem tặng quà phải thấy động cơ quay đùn hạt ra ngay lập tức, triệt tiêu cảm giác bị "nuốt quà" hoặc độ trễ hệ thống.
2. **Kích thích tương tác (Gamification):** Khuyến khích người xem nâng cấp từ quà nhỏ đơn lẻ (1 xu) lên quà giá trị cao (30 xu) để kích hoạt cơ chế "Bão thức ăn" (cả 3 máng cùng quay).
3. **Độc lập và Không nghẽn (Non-blocking Concurrency):** Cả 3 cụm cấp liệu vận hành độc lập. Việc xả thức ăn ở máng này không chặn (block) hay làm gián đoạn máng khác.
4. **Độ tin cậy phần cứng (Hardware Reliability):** Cơ chế trục vít tự khóa (self-locking) chống chảy tự do thức ăn và chống kẹt hạt tối ưu so với cơ chế cửa trượt.

---

## 2. CƠ CHẾ CƠ KHÍ & ĐỊNH LƯỢNG (AUGER DISPENSING UNIT)

Hệ thống sử dụng cơ cấu **trục vít tải (Auger Screw Feeder)** kết hợp động cơ gắn trục phía sau:

### 2.1 Đơn vị định lượng cơ bản (1 Chu kỳ đùn - Feeding Burst)
* **Định nghĩa 1 Chu kỳ:**
  * **Trường hợp dùng Động cơ bước (Stepper Motor):** Quay một góc $\mathbf{180^\circ}$ (hoặc 1 vòng $360^\circ$).
  * **Trường hợp dùng Động cơ giảm tốc (DC Geared Motor):** Kích hoạt cấp điện quay trong **300ms**, sau đó ngắt điện.
  * **Lượng thức ăn đùn ra:** Khoảng **1.5g – 3.0g** cho mỗi chu kỳ.
* **Cơ chế nhịp (Pulsing):** Với combo nhiều quà (ví dụ tặng 3 hoa hồng), động cơ không quay liền một mạch dài, mà quay ngắt nhịp:
  $$\text{Quay 300ms} \longrightarrow \text{Nghỉ 150ms} \longrightarrow \text{Quay 300ms} \dots$$
  Tạo âm thanh cơ khí và tiếng hạt rơi lắt rắt liên tục, kích thích gà mổ theo nhịp trên livestream.

---

## 3. MA TRẬN ĐỊNH TUYẾN QUÀ TẶNG (GIFT-TO-FEEDER ROUTING MATRIX)

| Tên Quà tặng (TikTok) | Giá trị (Coins) | Máng chỉ định | Công thức tính chu kỳ quay | Hành vi cơ cấu |
| :--- | :---: | :---: | :--- | :--- |
| **Bông Hoa Hồng** | 1 | **Máng Cám** | $\text{Chu kỳ} = \text{repeatCount} \times 1$ | Trục vít Cám quay theo số lượng hoa |
| **Bắn Tim** | 5 | **Máng Ngô** | $\text{Chu kỳ} = \text{repeatCount} \times 2$ | Trục vít Ngô quay đùn hạt bắp |
| **Vỏ Chuối** | 10 | **Máng Cám** | $\text{Chu kỳ} = \text{repeatCount} \times 3$ | Trục vít Cám quay đùn lượng cám lớn hơn |
| **Bánh Vòng** *(Jackpot)* | 30 | **Cả 3 máy** (Cám + Ngô + Thóc) | $\text{Chu kỳ} = \text{repeatCount} \times 4$ | **Fan-out:** 3 trục xoắn cùng quay đồng loạt đùn 3 loại hạt |
| **Quà khác chưa định danh** | Bất kỳ | **Máng Cám** | $\text{Chu kỳ} = 1$ | Mặc định quay 1 chu kỳ máng Cám |

---

## 4. KIẾN TRÚC XỬ LÝ SỰ KIỆN ĐA LUỒNG (SYSTEM PIPELINE)

```text
[ TikTok Live ]
       │
       ▼
[ TikFinity ] ──(Webhook HTTP POST)──► [ API Ingestion Service ] ──► Trả về HTTP 200 (< 5ms)
                                                     │
                                                     ▼
                                        [ Validator & Safety Filter ]
                                      (Áp dụng Safety Cap: Max 20 chu kỳ)
                                                     │
                          ┌──────────────────────────┼──────────────────────────┐
                          ▼                          ▼                          ▼
                  [Queue Máng Cám]           [Queue Máng Ngô]           [Queue Máng Thóc]
                          │                          │                          │
                          ▼                          ▼                          ▼
                   [Worker Cám]               [Worker Ngô]               [Worker Thóc]
                   (Thread / Task)            (Thread / Task)            (Thread / Task)
                          │                          │                          │
                          ▼                          ▼                          ▼
                   [Motor Vít Cám]            [Motor Vít Ngô]            [Motor Vít Thóc]
```

### 4.1 Chi tiết 4 tầng xử lý

#### Tầng 1: Ingestion Layer (Tiếp nhận sự kiện)
* Endpoint: `POST /webhook/tiktok_gift`
* Nhận payload JSON từ TikFinity:
  ```json
  {
    "giftName": "Bông Hoa hồng đầu tiên của tôi",
    "repeatCount": 5,
    "user": "viewer_123",
    "coins": 1
  }
  ```
* **Quy tắc bắt buộc:** Trả về mã phản hồi `HTTP 200 OK` ngay lập tức ($< 5\text{ms}$) để giải phóng kết nối webhook của nền tảng stream.

#### Tầng 2: Routing & Safety Layer (Định tuyến & Lọc an toàn)
* So khớp `giftName` với **Ma trận định tuyến quà tặng**.
* **Nguyên tắc an toàn (Safety Cap):** Giới hạn tối đa **20 chu kỳ** cho 1 đợt xả nhằm bảo vệ động cơ chống cháy cuộn dây và chống tràn thức ăn:
  $$\text{Số chu kỳ thực tế} = \min(\text{repeatCount} \times \text{Hệ số},\ 20)$$

#### Tầng 3: Queue & Fan-out Layer (Hàng đợi đa luồng)
* **Xử lý đơn mục tiêu (Hoa hồng, Bắn tim, Vỏ chuối):** Đẩy task `{user, gift, cycles}` vào hàng đợi của máng tương ứng (`Queue_Cam`, `Queue_Ngo`, hoặc `Queue_Thoc`).
* **Xử lý đa mục tiêu - Fan-out (Bánh vòng):** Tự động nhân bản task và đẩy đồng thời vào cả 3 hàng đợi `Queue_Cam`, `Queue_Ngo`, `Queue_Thoc` cùng lúc.

#### Tầng 4: Dispatcher / Worker Layer (Thực thi phần cứng)
* 3 luồng Worker chạy song song độc lập:
  * Đọc task từ Queue theo thứ tự FIFO (First In, First Out).
  * Điều khiển động cơ thực hiện $N$ chu kỳ quay nhấp nhả: `Quay 300ms -> Nghỉ 150ms`.
  * Sau khi hoàn tất lượt của một người dùng, nghỉ đệm **200ms** để triệt tiêu quán tính cơ học trước khi đón lượt tiếp theo.

---

## 5. ĐẶC TẢ GIAO TIẾP VỚI ESP32 (HARDWARE PAYLOAD SCHEMA)

Tầng Backend (chạy trên Cloud/Server) giao tiếp với bo mạch điều khiển ESP32 tại chuồng gà qua MQTT Topic `feeder/commands` (hoặc HTTP Endpoint `/api/dispense`):

```json
{
  "feeder": "cam",
  "cycles": 4,
  "spin_duration_ms": 300,
  "interval_ms": 150,
  "task_id": "tx_20261106_001"
}
```

* `feeder`: Tên máng cần kích hoạt (`cam`, `ngo`, `thoc`, hoặc `all`).
* `cycles`: Số chu kỳ quay cần thực hiện.
* `spin_duration_ms`: Thời gian động cơ được cấp điện cho mỗi chu kỳ.
* `interval_ms`: Thời gian nghỉ đệm giữa các chu kỳ.
* `task_id`: Mã định danh để đối soát và ghi log lịch sử.

---

## 6. XỬ LÝ CÁC TÌNH HUỐNG BIÊN & AN TOÀN HỆ THỐNG (EDGE CASES)

1. **Người xem tặng quà khác nhau tại cùng 1 giây:**
   * Máng Cám và Máng Ngô kích hoạt động cơ quay song song, không bên nào phải chờ bên nào.
2. **Nhiều người liên tiếp tặng quà cho cùng 1 máng:**
   * Xử lý theo thứ tự hàng đợi FIFO: Chạy hết chu kỳ của User A $\rightarrow$ Nghỉ 200ms $\rightarrow$ Chạy tiếp chu kỳ của User B.
3. **Kích hoạt Bánh Vòng khi 1 máng đang bận:**
   * Máng đang trống (ví dụ Ngô và Thóc) sẽ kích hoạt quay ngay lập tức.
   * Máng đang bận (ví dụ Cám) sẽ xếp chu kỳ Bánh Vòng vào ngay sau lượt cám đang xả dở, đảm bảo không bỏ sót quà của người xem.
4. **Bảo vệ sụt áp & Khởi động động cơ:**
   * Cả 3 động cơ khi kích hoạt đồng thời có thể rút dòng đỉnh lớn. Nguồn cấp cho hệ thống động cơ bắt buộc dùng bộ đổi nguồn độc lập (5V/12V $\ge 3\text{A}$), không dùng chung nguồn từ chân 5V của bo ESP32.
5. **Cơ chế chống trôi lệnh khi rớt mạng:**
   * Nếu ESP32 mất kết nối Wi-Fi/Server quá **15 giây**, toàn bộ tác vụ đang xếp hàng trong Queue sẽ tự động xóa sạch (Clear Queue). Điều này ngăn chặn việc khi có mạng trở lại, máy dồn xả một lượng lớn thức ăn thừa tích tụ ra máng.