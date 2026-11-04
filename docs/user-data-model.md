# Mô hình dữ liệu người dùng

Ứng dụng đang dùng một database SQLite chính: `backend/build/virtual_pet.db`.
Server đọc schema từ `database/schema.sql`; khi khởi động, migration trong
`backend/src/Server/Database.cpp` bổ sung cột còn thiếu cho database cũ mà không
xóa tài khoản, coins, inventory hay pet hiện có.

## Dữ liệu được lưu

| Bảng | Nội dung |
|---|---|
| `users` | username, email, password salt/hash, coins, cài đặt âm thanh |
| `pets` | tên, loài, level, happiness, hunger, energy |
| `sessions` | hash token đăng nhập và thời hạn |
| `inventory` | vật phẩm và số lượng theo user |
| `equipped_decorations` | vật trang trí đang trang bị theo slot |
| `purchase_transactions` | lịch sử giao dịch mua |
| `exercises`, `submissions`, `user_exercise_progress` | bài tập, câu trả lời và tiến độ |

Tên/loài pet và cài đặt âm thanh trước đây có thể đã nằm trong `localStorage`.
Frontend chuyển các giá trị cũ lên API khi đăng nhập thành công rồi xóa khóa cũ
sau khi server đã lưu. Từ đó trở đi SQLite là nguồn dữ liệu chính cho các mục
này. Session token không được lưu trong `localStorage`; browser nhận cookie
HttpOnly.

Cột `experience` có thể còn trong database hiện hữu để tránh migration làm mất dữ liệu,
nhưng không còn được dùng cho gameplay hoặc API. Database mới không tạo cột này.

## Cách chạy

Từ thư mục gốc của repository:

```powershell
.\backend\build.ps1 -Run -Port 8080
```

Mở `http://127.0.0.1:8080/`. Server phục vụ cả trang web và `/api/*`. Đường dẫn
database mặc định giữ nguyên database hiện có. Có thể chỉ định database khác để
test:

```powershell
.\backend\build.ps1 -Run -Port 8081 -DatabasePath "$env:TEMP\virtual-pet-test.db"
```

`backend/database/virtual_pet.template.db` chỉ là template cũ, không được server
mở khi chạy ứng dụng.