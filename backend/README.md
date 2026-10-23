# Backend: quy ước luồng dữ liệu (GAME-05)

```
request (API) -> Game -> module (Pet / User / Shop / Inventory / Reward) -> ActionResult -> response
```

1. **Chỉ Game được API gọi.** API không gọi Shop/Pet/User trực tiếp (GAME-03).
2. **Mọi hành động** (`feed, play, sleep, bath, buy, decorate, claimReward`) trả `ActionResult`:
   `code` (`toString(code)` -> chuỗi như `NOT_ENOUGH_COINS`), `message`, `levelsGained` (GAME-04).
3. **Chỉ chạy khi game `RUNNING`.** Sai trạng thái -> `INVALID_STATE`. Kiểm tra hợp lệ xong mới đổi dữ liệu,
   lỗi thì không đổi gì (không trừ xu, không mất đồ).
4. **Thời gian:** `updateOverTime(giây)`; cứ `Game::TICK_SECONDS` (60s) pet giảm hunger và energy. PAUSED thì không giảm.
5. **Lưu / nạp (KAN-72):** tầng database chỉ làm việc với `GameSnapshot`:
   - Lưu: `game.exportSnapshot()` -> ghi vào các bảng users / pets / inventory.
   - Nạp: đọc database -> `GameSnapshot` -> `game.importSnapshot(s)` -> `game.start()` -> `game.updateOverTime(giây offline)`.

## Quy ước chỉ số pet
- `hunger` là mức **no** (100 = no căng), thức ăn làm tăng. Khớp frontend và bảng `pets`.
- `happiness` là thanh lên cấp: đầy 100 thì `level + 1`, giữ phần dư.
- Mặc định: happiness 85, hunger 30, energy 70 (khớp bảng `pets`).

## Giá trị cân bằng game
Chỉnh trong `Pet.h` (`PLAY_*`, `SLEEP_*`, `BATH_*`, `*_DECAY_PER_TICK`) và `Game::TICK_SECONDS`.
