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
- Study: `/api/study/state`, `/api/study/time`, `/api/study/complete`; điểm, xu, màn đã qua,
  happiness và thời gian tiêu hao hunger/energy được lưu theo tài khoản trong SQLite.
- Practice: `/api/practice/state`, `/api/practice/time`, `/api/practice/game/start`,
  `/api/practice/game/abandon`, `/api/practice/finish`; kỷ lục chỉ thành chính thức
  cùng điểm/xu khi game over, timer Practice giảm hunger/energy.
- Bài tập: `GET /api/exercises` và `POST /api/exercises/{id}/submissions` dành cho luồng học tập riêng.
  Điểm, số lần thử và tiến độ được lưu trong SQLite.

Frontend dùng cookie HttpOnly; không tự giữ coins, pet stats hoặc inventory trong
trình duyệt. Một lần đăng nhập, các giá trị profile/âm thanh cũ từ `localStorage`
được nhập vào SQLite. `frontend/js/demo.js` là demo cũ, không được `index.html`
nạp trong luồng chạy hiện tại.

Trang Study yêu cầu đăng nhập. Study chỉ tính thời gian khi trang đang hiển thị;
heartbeat gửi mỗi 20 giây và server giới hạn mỗi nhịp tối đa 30 giây. Khi hunger
hoặc energy dưới 10, Study bị khóa cho đến khi người chơi về Home và chăm pet.
`backend/src/Exercise/Exercise.cpp` chứa quy tắc điểm riêng cho Study; luồng này
không dùng `Submission.cpp` hay evaluator chấm code. Hiện front-end vẫn tự kiểm
tra câu trả lời và gửi số câu đúng; server tính thưởng, khóa màn và ngăn cộng
thưởng lặp. Nếu cần chống sửa kết quả từ trình duyệt, cần chuyển đáp án chuẩn
sang backend để server tự chấm từng câu.

Practice tính thời gian khi trang Practice mở (kể cả menu/tạm dừng), và chốt điểm
vào happiness cùng xu đủ điều kiện khi kết thúc ván. Record mới trong ván chỉ là
tạm tính, được lưu thành kỷ lục tài khoản ở game over và bị bỏ nếu rời trang
hoặc bắt đầu ván khác. Điểm đếm thức ăn do trình duyệt gửi, nên không được coi là
xác thực chống gian lận.

## Kiểm thử

```powershell
.\backend\tests\api_integration.ps1
.\backend\tests\study_integration.ps1
.\backend\tests\practice_integration.ps1
```

Bộ test dùng database riêng trong thư mục tạm, bao gồm cookie auth, API game,
persistence sau restart và migration từ schema cũ; không thay đổi database người
chơi mặc định.
Study integration test cũng dùng database tạm và kiểm tra phần thưởng, review,
shop, quy đổi happiness, bộ đếm thời gian và điều kiện khóa pet.
Practice integration test dùng database tạm để kiểm tra timer, record, thưởng
chỉ khi game over, restart bỏ thưởng chưa chốt, mua Shop, idempotency và persistence.
