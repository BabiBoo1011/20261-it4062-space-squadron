# Đặc tả giao thức – Phi Đội Không Gian

## 1. Tổng quan

- Giao thức tầng ứng dụng dạng **văn bản**, chạy trên **TCP**, cổng mặc định `9999`.
- Mỗi thông điệp nằm trên **một dòng**, kết thúc bằng `\n` (server chấp nhận cả `\r\n`).
- Độ dài tối đa một dòng: `BUF_SIZE` = 4096 byte (định nghĩa trong `common/protocol.h`).
- Mã hóa: UTF-8. Tên lệnh, tham số định danh (username, tên đội, tên vật phẩm…) chỉ dùng ASCII.

## 2. Định dạng thông điệp

### 2.1. Client → Server (yêu cầu)

```
<LENH> [tham_so_1] [tham_so_2] ... [tham_so_cuoi]
```

- Các trường cách nhau **đúng 1 dấu cách**.
- Tên lệnh viết **HOA**.
- **Tham số cuối** của một số lệnh (đánh dấu `<...>*` trong bảng) được phép chứa dấu cách: là toàn bộ phần còn lại của dòng.

### 2.2. Server → Client

Mỗi dòng server gửi bắt đầu bằng một trong 3 tiền tố:

| Tiền tố | Ý nghĩa | Định dạng |
|---------|---------|-----------|
| `OK`  | Yêu cầu thành công | `OK <LENH> [du_lieu]` |
| `ERR` | Yêu cầu thất bại | `ERR <ma_loi> <mo_ta>` |
| `EVT` | Sự kiện server chủ động đẩy (không phải phản hồi) | `EVT <SU_KIEN> [du_lieu]` |

- Mỗi yêu cầu nhận **đúng một** dòng `OK` hoặc `ERR`, theo đúng thứ tự gửi.
- `EVT` có thể đến bất cứ lúc nào, xen giữa các phản hồi. Client phải phân biệt theo tiền tố.
- Dữ liệu trả về dạng `khoa=gia_tri` khi có nhiều trường, ví dụ: `OK STATUS hp=850 coin=1200`.
- Danh sách trả về trên một dòng, các phần tử cách nhau bằng dấu cách.

### 2.3. Quy tắc định danh

| Loại | Ràng buộc |
|------|-----------|
| Username, tên đội | 3–20 ký tự, gồm `A-Z a-z 0-9 _` |
| Mật khẩu | 4–32 ký tự, không chứa dấu cách |

## 3. Trạng thái của client (phân quyền)

```
CONNECTED ──LOGIN──▶ LOBBY ──vào đội──▶ IN_TEAM ──trận bắt đầu──▶ IN_MATCH
    ▲                  │                   │                         │
    └─────LOGOUT───────┴───────────────────┘          trận kết thúc ─┘ (về IN_TEAM)
```

| Trạng thái | Được phép dùng |
|------------|----------------|
| `CONNECTED` (chưa đăng nhập) | `PING`, `REGISTER`, `LOGIN` |
| `LOBBY` (đã đăng nhập, chưa có đội) | Lệnh tài khoản, `TEAM_CREATE`, `TEAM_LIST`, `TEAM_MEMBERS`, `TEAM_JOIN`, `INVITE_ACCEPT`, `INVITE_DECLINE` |
| `IN_TEAM` | Lệnh xem đội, `TEAM_LEAVE`; **đội trưởng** thêm: `TEAM_DELETE`, `TEAM_INVITE`, `TEAM_APPROVE`, `TEAM_REJECT`, `TEAM_KICK`, `CHALLENGE*`, `START` |
| `IN_MATCH` | Lệnh trong trận (mục 4.5–4.7). **Không** được rời/xóa đội |

Gọi lệnh sai trạng thái → `ERR 401` (chưa đăng nhập), `ERR 403` (không đủ quyền) hoặc `ERR 412` (sai trạng thái).

## 4. Danh sách lệnh

### 4.1. Chung

| Lệnh | Phản hồi thành công | Ghi chú |
|------|---------------------|---------|
| `PING` | `OK PONG` | Kiểm tra kết nối |

### 4.2. Tài khoản & phiên

| Lệnh | Phản hồi thành công | Lỗi có thể gặp |
|------|---------------------|----------------|
| `REGISTER <user> <pass>` | `OK REGISTER` | 400, 409 |
| `LOGIN <user> <pass>` | `OK LOGIN <user>` | 400, 404, 410, 420 |
| `LOGOUT` | `OK LOGOUT` | 401, 412 (đang trong trận) |
| `WHOAMI` | `OK WHOAMI user=<user> state=<trang_thai> team=<doi\|->` | 401 |

- Một tài khoản chỉ được đăng nhập ở **một** kết nối tại một thời điểm (`ERR 410`).
- Client mất kết nối được coi như `LOGOUT`.
- Tài khoản lưu trong `data/accounts.txt`, mỗi dòng: `<user> <pass>` (mật khẩu dạng thô).

### 4.3. Đội chơi

Người tạo đội là **đội trưởng**. Mỗi đội tối đa `MAX_TEAM_SIZE` = 3 người.

| Lệnh | Ai dùng | Phản hồi thành công | Sự kiện phát sinh |
|------|---------|---------------------|-------------------|
| `TEAM_CREATE <team>` | Chưa có đội | `OK TEAM_CREATE <team>` | – |
| `TEAM_DELETE` | Đội trưởng | `OK TEAM_DELETE` | Thành viên: `EVT TEAM_DELETED <team>` |
| `TEAM_LIST` | Đã đăng nhập | `OK TEAM_LIST <team>:<so_nguoi>/3 ...` | – |
| `TEAM_MEMBERS <team>` | Đã đăng nhập | `OK TEAM_MEMBERS <team> captain=<user> members=<u1>,<u2>,<u3>` | – |
| `TEAM_JOIN <team>` | Chưa có đội | `OK TEAM_JOIN <team>` (đã gửi yêu cầu) | Đội trưởng: `EVT JOIN_REQUEST <user>` |
| `TEAM_APPROVE <user>` | Đội trưởng | `OK TEAM_APPROVE <user>` | `<user>`: `EVT JOIN_ACCEPTED <team>`; cả đội: `EVT MEMBER_JOINED <user>` |
| `TEAM_REJECT <user>` | Đội trưởng | `OK TEAM_REJECT <user>` | `<user>`: `EVT JOIN_REJECTED <team>` |
| `TEAM_INVITE <user>` | Đội trưởng | `OK TEAM_INVITE <user>` | `<user>`: `EVT INVITE <team> <doi_truong>` |
| `INVITE_ACCEPT <team>` | Người được mời | `OK INVITE_ACCEPT <team>` | Cả đội: `EVT MEMBER_JOINED <user>` |
| `INVITE_DECLINE <team>` | Người được mời | `OK INVITE_DECLINE <team>` | Đội trưởng: `EVT INVITE_DECLINED <user>` |
| `TEAM_LEAVE` | Thành viên | `OK TEAM_LEAVE` | Cả đội: `EVT MEMBER_LEFT <user>`. Nếu người rời là **đội trưởng** → đội bị giải tán, thành viên nhận `EVT TEAM_DELETED <team>` |
| `TEAM_KICK <user>` | Đội trưởng | `OK TEAM_KICK <user>` | `<user>`: `EVT KICKED <team>`; cả đội: `EVT MEMBER_LEFT <user>` |

Lỗi thường gặp: 403 (không phải đội trưởng), 404 (không có đội/người), 409 (tên đội đã tồn tại), 412 (đã có đội / đang trong trận), 421 (đội đã đủ người).

- Đội trưởng **đăng xuất / mất kết nối** khi chưa vào trận cũng được coi như rời đội → đội bị giải tán.
- Yêu cầu tham gia (`TEAM_JOIN`) và lời mời (`TEAM_INVITE`) **hết hạn sau 2 phút**. Khi hết hạn, server gửi
  `EVT JOIN_EXPIRED <team> <user>` / `EVT INVITE_EXPIRED <team> <user>` cho cả hai bên.
  Phản hồi một yêu cầu đã hết hạn → `ERR 404`.

### 4.4. Thách đấu & bắt đầu trận

| Lệnh | Ai dùng | Phản hồi thành công | Sự kiện phát sinh |
|------|---------|---------------------|-------------------|
| `CHALLENGE <team>` | Đội trưởng | `OK CHALLENGE <team>` | Đội trưởng đội kia: `EVT CHALLENGE <team_gui>` |
| `CHALLENGE_ACCEPT <team>` | Đội trưởng | `OK CHALLENGE_ACCEPT <team>` | Cả 2 đội: `EVT CHALLENGE_ACCEPTED <team_A> <team_B>` |
| `CHALLENGE_DECLINE <team>` | Đội trưởng | `OK CHALLENGE_DECLINE <team>` | Đội gửi: `EVT CHALLENGE_DECLINED <team>` |
| `START` | Đội trưởng (đã được chấp nhận thách đấu) | `OK START <match_id>` | Cả 2 đội: `EVT MATCH_START <match_id> <team_A> <team_B>` |

- Lời thách đấu **hết hạn sau 2 phút** → cả hai đội trưởng nhận `EVT CHALLENGE_EXPIRED <team_A> <team_B>`.
- Khi trận bắt đầu, **mọi tàu được reset** về trạng thái ban đầu: HP = 1000, Armor = 0, coin = 0,
  1 pháo 30mm với 50 viên, không có laze / tên lửa / giáp. Coin và trang bị **không** được giữ lại sau trận.

### 4.5. Trong trận – thông tin & rương kho báu

| Lệnh | Phản hồi thành công |
|------|---------------------|
| `STATUS` | `OK STATUS hp=<hp> armor=<lop1>,<lop2> coin=<coin> ammo=<vien_30mm> lasers=<so_khau> laser_power=<so_lan_ban> missiles=<so_qua>` |
| `SHIPS` | `OK SHIPS <user>:<team>:<hp>:<tong_armor>:<alive\|dead> ...` |
| `OPEN_CHEST <chest_id> <dap_an>*` | `OK OPEN_CHEST <chest_id> coin=<so_coin_nhan> total=<tong_coin>` |

**Thả rương** (server chủ động, ngẫu nhiên trong trận):

```
EVT CHEST <chest_id> <BRONZE|SILVER|GOLD> MCQ <cau_hoi>|<A>|<B>|<C>|<D>
EVT CHEST <chest_id> <BRONZE|SILVER|GOLD> SHORT <cau_hoi>
```

- Trắc nghiệm: đáp án là `A`/`B`/`C`/`D`. Trả lời ngắn: so sánh **không phân biệt hoa thường**, bỏ khoảng trắng thừa.
- **Chỉ người trả lời đúng đầu tiên** nhận coin. Server phát cho cả trận: `EVT CHEST_OPENED <chest_id> <user> <coin>`.
- Trả lời sai: `ERR 433`. Rương đã có người mở: `ERR 434`.

### 4.6. Trong trận – mua sắm & sửa chữa

| Lệnh | Giá | Hiệu quả |
|------|-----|----------|
| `BUY AMMO_30MM [so_thung]` | 100 coin/thùng | +50 viên đạn 30mm mỗi thùng |
| `BUY LASER` | 1000 coin | +1 khẩu pháo laze (tối đa 3 khẩu, vì pháo 30mm mặc định chiếm 1 trong 4 slot pháo) |
| `BUY LASER_POWER [so_bo]` | 100 coin/bộ | +10 lần bắn laze mỗi bộ |
| `BUY MISSILE [so_qua]` | 2000 coin/quả | +1 tên lửa mỗi quả |
| `BUY ARMOR_BASIC [vi_tri]` | 1000 coin | Lắp giáp Armor = 500 |
| `BUY ARMOR_ENHANCED [vi_tri]` | 2000 coin | Lắp giáp Armor = 1500 |
| `REPAIR <so_hp>` | 1 coin/HP | Hồi HP, tối đa 1000 |

- Số lượng mặc định là 1 nếu bỏ trống.
- Giáp: nếu còn ô trống thì lắp vào ô trống. Nếu đã đủ 2 lớp thì **phải** chỉ định `vi_tri` (1 hoặc 2) để **thay thế** lớp giáp đó.
- Phản hồi: `OK BUY <vat_pham> coin=<coin_con_lai>` / `OK REPAIR hp=<hp> coin=<coin_con_lai>`.
- Lỗi: 430 (không đủ coin), 431 (vượt giới hạn: 4 pháo **tính cả pháo 30mm**, 4 tên lửa, 2 lớp giáp).

### 4.7. Trong trận – tấn công & kết thúc

| Lệnh | Phản hồi thành công |
|------|---------------------|
| `ATTACK <target_user> <CANNON\|LASER\|MISSILE>` | `OK ATTACK <target_user> dam=<dam>` |

| Vũ khí | Tiêu hao mỗi lần | Dam |
|--------|------------------|-----|
| `CANNON` (pháo 30mm) | 1 viên đạn | 10 |
| `LASER` | 1 lần bắn từ nguồn (cần có ít nhất 1 khẩu laze) | 100 |
| `MISSILE` | 1 quả tên lửa | 800 |

- Mỗi lệnh `ATTACK` bắn **đúng 1 phát**: CANNON tiêu 1 viên, LASER tiêu 1 lần bắn, MISSILE tiêu 1 quả.
- **Không có thời gian hồi** giữa các lần tấn công.

**Tính sát thương:** mỗi 1 điểm Dam trừ 1 điểm Armor của lớp giáp ngoài cùng còn giáp. Hết giáp thì phần Dam còn lại trừ vào HP.

Sự kiện phát cho cả trận:

```
EVT HIT <attacker> <target> <weapon> <dam> hp=<hp_con_lai> armor=<tong_armor_con_lai>
EVT SHIP_DESTROYED <user>                  # khi HP về 0
EVT MATCH_END <match_id> winner=<team>     # khi một đội bị hạ toàn bộ tàu
```

- Không được tấn công đồng đội hoặc tàu đã bị hạ (`ERR 435`).
- Tàu đã bị hạ không được thực hiện lệnh tấn công/mua/sửa (`ERR 412`).
- Hết đạn / hết nguồn / hết tên lửa: `ERR 432`.

## 5. Bảng mã lỗi

| Mã | Tên | Ý nghĩa |
|----|-----|---------|
| 400 | BAD_SYNTAX | Sai cú pháp, thiếu tham số, lệnh không tồn tại |
| 401 | NOT_LOGGED_IN | Chưa đăng nhập |
| 403 | FORBIDDEN | Không đủ quyền (vd: không phải đội trưởng) |
| 404 | NOT_FOUND | Không tìm thấy tài khoản / đội / người chơi / rương |
| 409 | ALREADY_EXISTS | Tài khoản hoặc tên đội đã tồn tại |
| 410 | ALREADY_ONLINE | Tài khoản đang đăng nhập ở nơi khác |
| 412 | WRONG_STATE | Lệnh không hợp lệ ở trạng thái hiện tại |
| 420 | WRONG_PASSWORD | Sai mật khẩu |
| 421 | TEAM_FULL | Đội đã đủ 3 người |
| 430 | NOT_ENOUGH_COIN | Không đủ coin |
| 431 | LIMIT_REACHED | Vượt giới hạn trang bị |
| 432 | OUT_OF_AMMO | Hết đạn / nguồn laze / tên lửa |
| 433 | WRONG_ANSWER | Trả lời sai câu hỏi rương |
| 434 | CHEST_TAKEN | Rương đã có người mở |
| 435 | INVALID_TARGET | Mục tiêu không hợp lệ |
| 500 | SERVER_ERROR | Lỗi phía server |

## 6. Ghi log hoạt động

Server ghi mỗi yêu cầu vào `logs/server.log`, một dòng một sự kiện:

```
[2026-11-20 21:05:13] [127.0.0.1:53012] [manh] ATTACK khai MISSILE -> OK dam=800
[2026-11-20 21:05:20] [127.0.0.1:53020] [-] LOGIN hoang ****** -> ERR 420
```

- `[-]` khi chưa đăng nhập.
- **Không** ghi mật khẩu ra log (thay bằng `******`).

## 7. Ví dụ phiên làm việc

```
C: REGISTER manh 123456
S: OK REGISTER
C: LOGIN manh 123456
S: OK LOGIN manh
C: TEAM_CREATE Rong_Xanh
S: OK TEAM_CREATE Rong_Xanh
C: TEAM_INVITE khai
S: OK TEAM_INVITE khai
S: EVT MEMBER_JOINED khai
C: CHALLENGE Phuong_Hoang
S: OK CHALLENGE Phuong_Hoang
S: EVT CHALLENGE_ACCEPTED Rong_Xanh Phuong_Hoang
C: START
S: OK START 1
S: EVT MATCH_START 1 Rong_Xanh Phuong_Hoang
S: EVT CHEST 7 SILVER MCQ Giao thuc nao huong ket noi?|UDP|TCP|ICMP|ARP
C: OPEN_CHEST 7 B
S: OK OPEN_CHEST 7 coin=500 total=500
S: EVT CHEST_OPENED 7 manh 500
C: BUY AMMO_30MM 2
S: OK BUY AMMO_30MM coin=300
C: ATTACK hoang CANNON
S: OK ATTACK hoang dam=10
S: EVT HIT manh hoang CANNON 10 hp=990 armor=0
```