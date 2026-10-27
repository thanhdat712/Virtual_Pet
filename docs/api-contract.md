# API Contract

Đây là hợp đồng của backend đang được build từ `backend/src/server_main.cpp`.
Frontend gọi API trong `frontend/js/api.js`; server C++ phục vụ cả frontend lẫn
API cùng origin và ghi dữ liệu vào SQLite.

```text
Browser -> frontend JavaScript -> HTTP API -> C++ Database layer -> SQLite
```

## Quy tắc chung

- Base path là `/api`; request/response dùng JSON UTF-8.
- Các API yêu cầu đăng nhập dùng cookie `vp_session` (HttpOnly, SameSite=Strict).
- Server lấy user ID từ session, không nhận user ID từ frontend.
- Giá vật phẩm, coins, inventory và chỉ số pet do server quyết định.
- Database chính: `backend/build/virtual_pet.db`. Schema runtime: `database/schema.sql`.
- Chạy game actions bằng `POST`; tải dữ liệu bằng `GET`.

## Đăng nhập

### `POST /api/auth/register`

Body: `{ "username": "player_01", "email": "player@example.com", "password": "at-least-8-characters" }`

Username dài 3–20 ký tự ASCII gồm chữ, số hoặc `_`; password tối thiểu 8 ký tự.
Đăng ký thành công trả `201` và tạo tài khoản cùng pet mặc định trong SQLite.

### `POST /api/auth/login`

Body: `{ "email": "player@example.com", "password": "..." }`.
Thành công trả `200`, thông tin user và cookie `vp_session`.

### `GET /api/auth/session`

Trả `200` với `authenticated` và user hiện tại; khi chưa đăng nhập, `user` là `null`.

### `POST /api/auth/logout`

Thu hồi session trong SQLite và xóa cookie.

## Game

### `GET /api/shop`

Trả danh sách catalog. Có thể gọi trước khi đăng nhập.

### `GET /api/game/state`

Yêu cầu đăng nhập. Trả trạng thái user, pet, inventory, equipment và settings.

```json
{
  "user": { "id": 1, "username": "player_01", "coins": 100 },
  "pet": {
    "name": "Mochi", "species": "cat", "level": 1,
    "experience": 0, "happiness": 85, "hunger": 30, "energy": 70
  },
  "inventory": [{ "itemId": 1, "quantity": 2 }],
  "equipment": { "headwear": null, "background": null, "floor": null },
  "settings": { "soundEnabled": true }
}
```

### Study

The Study page requires login and uses account-scoped SQLite progress instead of
browser storage. `GET /api/study/state` returns total Study points, the shared
coin balance, completed stage IDs, and current pet stats.

| Method | Route | Body | Purpose |
|---|---|---|---|
| `GET` | `/api/study/state` | Không có | Load Study totals, completed stages and pet stats |
| `POST` | `/api/study/time` | `{ "action": "start" }` | Start/resume visible Study time |
| `POST` | `/api/study/time` | `{ "action": "heartbeat" }` | Account for visible time; sent every 20 seconds |
| `POST` | `/api/study/time` | `{ "action": "pause" }` | Stop counting while the page is hidden |
| `POST` | `/api/study/complete` | `{ "level": "A1", "tier": "easy", "correctAnswers": 4 }` | Submit a finished five-question stage |

A stage requires 4 of 5 correct answers. Easy and medium award 15/18 points per
correct answer plus 25 points; hard awards 20 per correct answer plus 50 points
and 20 coins. A completed stage is review-only and cannot award twice. Every 50
Study points add one happiness point; the remainder is retained, so the 0.2%
conversion per 10 points is not lost to integer rounding. Happiness continues
to use the game's existing level-up behavior.

While Study is visible, each 210 accounted seconds subtracts one point from both
hunger and energy. The server caps each heartbeat interval at 30 seconds; the
page sends a heartbeat every 20 seconds and pauses on `visibilitychange`. Study
is blocked when either stat is below 10, and the page offers only a Home link.

The existing question bank and answer checks remain in `frontend/js/study-data.js`
and `frontend/js/study.js`; the completion API receives the resulting correct
answer count. The server owns reward calculations, unlock order, and one-time
completion records, but does not independently re-grade each answer. A
competitive/public deployment should move answer-key validation to the server.

### Các thao tác yêu cầu đăng nhập

| Method | Route | Body |
|---|---|---|
| `POST` | `/api/shop/purchases` | `{ "itemId": 1, "quantity": 2 }` |
| `POST` | `/api/pet/feed` | `{ "itemId": 1 }` |
| `POST` | `/api/pet/play` | Không có |
| `POST` | `/api/pet/sleep` | Không có |
| `POST` | `/api/pet/bath` | Không có |
| `POST` | `/api/pet/decorate` | `{ "itemId": 8 }` |
| `POST` | `/api/pet/unequip` | `{ "slot": "background" }` |
| `PUT` | `/api/pet` | `{ "name": "Mochi", "species": "dog" }` |
| `PUT` | `/api/settings` | `{ "soundEnabled": false }` |
| `GET` | `/api/exercises` | Không có; trả danh sách bài công khai |
| `POST` | `/api/exercises/{id}/submissions` | `{ "answer": "..." }` |

Action thành công trả envelope có `success: true` và `state` mới. Action thất bại
trả HTTP error cùng `success: false`, `code`, `message`. Từng thay đổi game được
ghi vào SQLite; mua hàng và cho ăn chạy trong transaction.

`species` chỉ nhận `cat` hoặc `dog`; `slot` chỉ nhận `headwear`, `background`
hoặc `floor`. Happiness đủ 100 sẽ tăng level và giữ phần dư.

Các endpoint bài tập được giữ riêng cho luồng học tập; chúng không thuộc màn hình
pet này. Server chấm câu trả lời và lưu tiến độ bài tập trong SQLite.

## API tương thích cũ

Các route `/api/register`, `/api/login`, `/api/me` và `/api/logout` vẫn được giữ
để không làm hỏng client/test cũ. Chúng gọi cùng backend và cùng file SQLite,
nhưng frontend hiện tại chỉ dùng route `/api/auth/*` và `/api/game/*` ở trên.

Reward claim và offline decay chưa được triển khai.

## Nộp mã nguồn (SUB-02/SUB-05)

`POST /api/exercises/{id}/submissions` nhận JSON `{ "sourceCode": "...", "language": "cpp" }`.
Yêu cầu session đăng nhập hợp lệ. Mã nguồn tối đa 32.768 byte; hiện chỉ nhận `cpp`; giới hạn 5 lần nộp/phút theo user và tiến trình server. Thành công trả `202 Accepted`, gồm `submissionId`, `status: "queued"`, `score`, `message`, `submittedAt`, `finishedAt`, `finished` và header `Location: /api/submissions/{id}`.

`GET /api/submissions/{id}` trả trạng thái bài nộp do chính user tạo. Mã lỗi chuẩn gồm `INVALID_SUBMISSION`, `RATE_LIMITED`, `INVALID_SUBMISSION_ID`, `SUBMISSION_NOT_FOUND`, `FORBIDDEN` và `SUBMISSION_STORAGE_ERROR`.

Worker chấm bài nằm tại `backend/evaluator/worker.py` và dùng cùng SQLite file với server. Khởi chạy worker riêng sau khi Docker Engine hoạt động:

```powershell
python backend/evaluator/worker.py --db backend/build/virtual_pet.db
```

Worker lấy bài `queued`, chuyển sang `evaluating`, chạy từng test trong container Docker riêng (network disabled, root filesystem read-only, capabilities dropped, unprivileged UID, PID/CPU/memory/resource limits, timeout và giới hạn output), rồi ghi trạng thái cuối. Kết quả cuối gồm `accepted`, `wrong_answer`, `compile_error`, `runtime_error`, `time_limit_exceeded`, `memory_limit_exceeded` hoặc `internal_error`. Khi đạt lần đầu một bài, worker cộng 10 xu; bảng `code_submission_rewards` có khóa chính `(user_id, exercise_id)` để tránh thưởng trùng. Xem `backend/evaluator/README.md` để cấu hình test cases và vận hành.

Worker là tiến trình riêng nên phải được giám sát/khởi động cùng backend trong triển khai thực tế. Cần Docker Engine hoạt động và image judge đã tải. Các test worker tự động không thay thế kiểm thử end-to-end trên máy có Docker; không triển khai public trước khi hoàn tất đánh giá bảo mật của môi trường container.
