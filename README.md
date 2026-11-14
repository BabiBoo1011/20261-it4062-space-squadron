# Phi Đội Không Gian (Space Squadron)

Bài tập lớn môn **Thực hành Lập trình mạng**

Đề tài số 7

Ngôn ngữ: **C** 

## 📌 Giới thiệu

**Phi Đội Không Gian** là trò chơi trực tuyến nhiều người chơi theo mô hình **Client – Server**, viết bằng ngôn ngữ C với socket TCP. Người chơi chia thành các đội, điều khiển phi thuyền để tấn công đối phương. **Đội nào bị hạ hết phi thuyền trước sẽ thua.**

Trong trận, người chơi trả lời câu hỏi để mở rương kho báu, nhận coin, rồi dùng coin mua vũ khí, giáp và sửa tàu.

## 👥 Thành viên nhóm

| STT | Họ và tên            | MSSV | Vai trò |
|-----|----------------------|------|---------|
| 1   | Nguyễn Danh Khải     | 20235750 |     |
| 2   | Đinh Đức Mạnh        | 20235774 |     |
| 3   | Nguyễn Trọng Hoàng   | 20235725 |     |
| 4   | -                    | -        | -   |

## 🎮 Luật chơi

### Đội chơi
- Mỗi đội có **tối đa 3 người chơi**.
- Hai đội thách đấu nhau. Đội bị hạ toàn bộ phi thuyền trước sẽ thua.

### Chỉ số ban đầu của phi thuyền
| Thuộc tính | Giá trị |
|------------|---------|
| HP (sinh lực) | 1000 |
| Armor (giáp)  | 0 |
| Vũ khí mặc định | Pháo tự động 30mm, 50 viên, sát thương 10 điểm Dam/viên |

### Giới hạn trang bị (mỗi tàu)
- Tối đa **4 khẩu pháo**
- Tối đa **4 tên lửa**
- Tối đa **2 lớp giáp**

### Cơ chế sát thương
- Mỗi 1 điểm Dam làm giảm **1 điểm Armor** nếu tàu còn giáp.
- Khi hết giáp, mỗi 1 điểm Dam làm giảm **1 điểm HP**.
- Giáp có thể được thay thế.

### Rương kho báu
Rương xuất hiện ngẫu nhiên. Người chơi trả lời câu hỏi (trắc nghiệm hoặc trả lời ngắn) để mở. **Chỉ người trả lời đúng đầu tiên mới nhận được coin.**

| Loại rương | Phần thưởng |
|------------|-------------|
| 🥉 Rương đồng | 100 coin |
| 🥈 Rương bạc  | 500 coin |
| 🥇 Rương vàng | 2000 coin |

### Cửa hàng

**Vũ khí tấn công**
| Vật phẩm | Giá | Sát thương | Ghi chú |
|----------|-----|------------|---------|
| Đạn pháo tự động | 100 coin/1 thùng 50 viên | 10 Dam/viên | |
| Pháo laze | 1000 coin/1 khẩu | 100 Dam/phát | Cần nguồn cấp 100 coin/1 bộ cho phép 10 lần bắn |
| Tên lửa | 2000 coin/1 quả | 800 Dam/quả | |

**Trang bị phòng thủ**
| Vật phẩm | Giá | Armor |
|----------|-----|-------|
| Giáp cơ bản | 1000 coin | 500 |
| Giáp tăng cường | 2000 coin | 1500 |

**Dịch vụ**
| Dịch vụ | Giá |
|---------|-----|
| Sửa chữa tàu | 1 coin/1 HP |

## ✅ Danh sách chức năng & thang điểm 

### Phần 1: Hạ tầng mạng & quản lý
- [ ] Xử lý truyền dòng – 1đ
- [ ] Cài đặt cơ chế vào/ra socket trên server – 2đ
- [ ] Đăng ký và quản lý tài khoản – 2đ
- [ ] Đăng nhập và quản lý phiên – 2đ
- [ ] Kiểm soát quyền truy cập – 2đ
- [ ] Tạo đội chơi – 1đ
- [ ] Xóa đội chơi – 1đ
- [ ] Liệt kê danh sách đội chơi – 1đ
- [ ] Liệt kê danh sách thành viên trong đội – 1đ
- [ ] Yêu cầu tham gia một đội và phê duyệt – 2đ
- [ ] Mời tham gia vào đội và phê duyệt – 2đ
- [ ] Rời đội – 1đ
- [ ] Xóa thành viên khỏi đội – 1đ

### Phần 2: Gameplay
- [ ] Gửi/chấp nhận lời thách đấu – 2đ
- [ ] Bắt đầu cuộc chơi – 1đ
- [ ] Thả rương kho báu – 2đ
- [ ] Mở rương kho báu – 2đ
- [ ] Mua giáp trang bị – 2đ
- [ ] Mua vũ khí trang bị – 4đ
- [ ] Tấn công – 4đ
- [ ] Sửa chữa tàu – 1đ
- [ ] Xác định kết quả cuộc chơi – 1đ
- [ ] Ghi log hoạt động – 1đ

## 🏗️ Kiến trúc dự kiến

```
+-----------+        TCP Socket        +-------------------+
|  Client 1 | <----------------------> |                   |
+-----------+                          |      SERVER       |
+-----------+                          |  - Quản lý TK     |
|  Client 2 | <----------------------> |  - Quản lý phiên  |
+-----------+                          |  - Quản lý đội    |
     ...                               |  - Game logic     |
+-----------+                          |  - Ghi log        |
|  Client N | <----------------------> |                   |
+-----------+                          +-------------------+
```

- **Ngôn ngữ:** C (chuẩn C99/C11)
- **Thư viện mạng:** POSIX Socket API (`socket`, `bind`, `listen`, `accept`, `send`, `recv`)
- **Giao thức:** TCP, tự định nghĩa giao thức tầng ứng dụng, có cơ chế đóng gói/tách thông điệp (xử lý truyền dòng)
- **Xử lý đồng thời ở server:** `select()`/`poll()` hoặc đa luồng `pthread` – *sẽ chốt sau*
- **Lưu trữ:** file văn bản cho tài khoản, bộ câu hỏi và log
- **Môi trường:** Linux, biên dịch bằng `gcc` + `Makefile`

## 📁 Cấu trúc thư mục (dự kiến)

```
.
├── server/       # Mã nguồn server (.c, .h)
├── client/       # Mã nguồn client (.c, .h)
├── common/       # Định nghĩa giao thức, cấu trúc thông điệp dùng chung
├── data/         # Dữ liệu tài khoản, bộ câu hỏi
├── logs/         # File log hoạt động
├── docs/         # Tài liệu thiết kế, đặc tả giao thức
├── Makefile
└── README.md
```

## 🗓️ Kế hoạch thực hiện (sơ bộ)

| Giai đoạn | Nội dung |
|-----------|----------|
| 1 | Phân tích đề tài, thiết kế giao thức & định dạng thông điệp |
| 2 | Cài đặt socket server/client, xử lý truyền dòng |
| 3 | Tài khoản, phiên đăng nhập, phân quyền |
| 4 | Quản lý đội chơi |
| 5 | Thách đấu, logic trận đấu, rương kho báu |
| 6 | Cửa hàng, tấn công, sửa tàu, xác định kết quả |
| 7 | Ghi log, kiểm thử, hoàn thiện báo cáo |

## 🚀 Hướng dẫn build & chạy (dự kiến)

```bash
make            # Biên dịch server và client
./server <port>
./client <server_ip> <port>
```

*Sẽ cập nhật chi tiết khi có mã nguồn.*
