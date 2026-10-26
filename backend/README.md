# Backend API

## Một backend đang chạy

Entry point duy nhất của HTTP API là `src/server_main.cpp`. Script PowerShell và
CMake đều build cùng entry point này. Server dùng Winsock, phục vụ frontend và
API cùng origin, rồi đọc/ghi trạng thái trong SQLite.

```text
frontend/index.html
  -> frontend/js/api.js
  -> backend/src/server_main.cpp
  -> backend/src/Server/Database.cpp
  -> backend/build/virtual_pet.db
```

## Chạy

Từ thư mục gốc:

```powershell
.\backend\build.ps1 -Run -Port 8080
```

Mở `http://127.0.0.1:8080/`. Có thể đổi cổng hoặc database:

```powershell
.\backend\build.ps1 -Run -Port 8081 -DatabasePath "$env:TEMP\virtual-pet-test.db"
```

Mặc định server dùng `backend/build/virtual_pet.db`, là database cũ đã được giữ
lại và migration tại chỗ. Đừng xóa file này nếu cần giữ tiến trình người chơi.
Schema chuẩn duy nhất là `database/schema.sql`; server thực thi schema này khi
khởi động rồi tự bổ sung các cột mới cho database cũ.

## API chính

- Auth/session: `/api/auth/register`, `/api/auth/login`, `/api/auth/session`, `/api/auth/logout`
- Trạng thái/catalog: `/api/game/state`, `/api/shop`
- Game: mua hàng, cho ăn, chơi, ngủ, tắm, trang trí
- Profile/settings: đổi tên/loài pet, lưu tùy chọn âm thanh
- Bài tập: `GET /api/exercises` và `POST /api/exercises/{id}/submissions` dành cho luồng học tập riêng.
  Điểm, số lần thử và tiến độ được lưu trong SQLite.

Frontend dùng cookie HttpOnly; không tự giữ coins, pet stats hoặc inventory trong
trình duyệt. Một lần đăng nhập, các giá trị profile/âm thanh cũ từ `localStorage`
được nhập vào SQLite. `frontend/js/demo.js` là demo cũ, không được `index.html`
nạp trong luồng chạy hiện tại.

## Kiểm thử

```powershell
.\backend\tests\api_integration.ps1
```

Bộ test dùng database riêng trong thư mục tạm, bao gồm cookie auth, API game,
persistence sau restart và migration từ schema cũ; không thay đổi database người
chơi mặc định.
