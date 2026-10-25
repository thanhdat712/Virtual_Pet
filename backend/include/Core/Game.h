#ifndef GAME_H
#define GAME_H

#include <memory>
#include "GameState.h"
#include "ActionResult.h"
#include "GameSnapshot.h"
#include "PlayerGameData.h"

class Reward;
class Inventory;

// Điểm truy cập DUY NHẤT của tầng API vào game (KAN-37/38/39).
//
// Luồng dữ liệu: request -> Game -> module (Pet/User/Shop/Inventory/Reward) -> ActionResult.
//   - API chỉ gọi các hàm hành động bên dưới, không tự gọi Shop/Pet/User trực tiếp.
//   - Mọi hành động đều kiểm tra trạng thái game (phải RUNNING) và trả ActionResult kèm mã lỗi.
//   - Hành động chỉ đổi dữ liệu sau khi đã kiểm tra hợp lệ hết, thất bại thì không đổi gì.
//   - Lưu/nạp: exportSnapshot() / importSnapshot(); tầng database (KAN-72) chỉ làm việc với GameSnapshot.
class Game {
private:
    GameState gameState;
    int timeCarrySeconds = 0; // số giây lẻ chưa đủ 1 tick, giữ lại cho lần cập nhật sau

public:
    static constexpr int TICK_SECONDS = 60; // cứ mỗi 60 giây game chạy thì pet bị giảm chỉ số 1 lần

    Game();

    // --- Vòng đời ván game. Gọi sai trạng thái thì trả INVALID_STATE, không im lặng bỏ qua. ---
    void initialize(); // tạo lại người chơi, pet, shop và đưa game về NOT_STARTED
    ActionResult start();   // NOT_STARTED -> RUNNING
    ActionResult pause();   // RUNNING -> PAUSED
    ActionResult resume();  // PAUSED -> RUNNING
    ActionResult end();     // RUNNING/PAUSED -> ENDED

    GameStatus getStatus() const;

    // --- Hành động của người chơi (chỉ chạy khi game RUNNING) ---
    ActionResult feed(int foodId);              // lấy 1 đồ ăn trong túi cho pet ăn
    ActionResult play();
    ActionResult sleep();
    ActionResult bath();
    ActionResult buy(int itemId, int quantity = 1);
    ActionResult decorate(int itemId);          // gắn đồ trang trí đã có trong túi lên pet/phòng
    ActionResult unequipDecoration(DecorationSlot slot);
    ActionResult claimReward(const Reward& reward); // nhận thưởng: cộng xu + item vào túi

    // --- Thời gian: giảm chỉ số pet theo thời gian đã trôi (chỉ khi RUNNING; PAUSED thì pet không đói) ---
    // Giây lẻ không bị mất, được cộng dồn sang lần gọi sau.
    ActionResult updateOverTime(int elapsedSeconds);

    // --- Lưu / nạp (KAN-72) ---
    GameSnapshot exportSnapshot() const;
    // Nạp dữ liệu đã lưu. Dữ liệu sai thì trả INVALID_SNAPSHOT và giữ nguyên ván hiện tại.
    // Sau khi nạp game ở NOT_STARTED: gọi start(), rồi updateOverTime(thời gian offline) nếu cần.
    ActionResult importSnapshot(const GameSnapshot& snapshot);

    // --- Đọc dữ liệu để hiển thị (dùng bản const). Bản không-const giữ lại cho test cũ;
    //     tầng API không nên dùng chúng để đổi dữ liệu vòng qua các kiểm tra của Game. ---
    PlayerData& getPlayer();
    const PlayerData& getPlayer() const;
    Pet& getPet();
    const Pet& getPet() const;
    Shop& getShop();
    const Shop& getShop() const;
    const Inventory& getInventory() const; // túi đồ của người chơi
    int getCoins() const;
};

#endif
