#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <memory>

// Khai báo trước để header nhẹ, tránh include vòng
class PlayerData;
class Pet;
class Shop;

enum class GameStatus {
    NOT_STARTED,
    RUNNING,
    PAUSED,
    ENDED
};

// Toàn bộ trạng thái của một ván game: trạng thái chạy + người chơi + pet + shop.
// Luôn ở trạng thái hợp lệ: constructor tự gọi initialize(), nên getPlayer/getPet/getShop
// không bao giờ trả về đối tượng rỗng.
class GameState {
private:
    GameStatus status;
    std::unique_ptr<PlayerData> player;
    std::unique_ptr<Pet> pet;
    std::unique_ptr<Shop> shop;

public:
    GameState();
    ~GameState(); // định nghĩa trong .cpp vì unique_ptr cần kiểu đầy đủ

    GameState(const GameState&) = delete;            // mỗi ván game chỉ có một trạng thái
    GameState& operator=(const GameState&) = delete;

    // Tạo lại người chơi (id 1, xu khởi điểm, túi đồ rỗng), pet mặc định và shop mới,
    // đặt trạng thái về NOT_STARTED. Gọi lại được để bắt đầu ván mới.
    void initialize();

    GameStatus getStatus() const;
    void setStatus(GameStatus newStatus);

    PlayerData& getPlayer();
    const PlayerData& getPlayer() const;
    Pet& getPet();
    const Pet& getPet() const;
    Shop& getShop();
    const Shop& getShop() const;
};

#endif
