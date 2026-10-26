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
    std::shared_ptr<Pet> pet;
    std::unique_ptr<Shop> shop;

public:
    GameState();
    ~GameState() = default;

    GameState(const GameState&) = delete;
    GameState& operator=(const GameState&) = delete;

    void initialize();
    void restore(std::unique_ptr<PlayerData> newPlayer, std::shared_ptr<Pet> newPet);

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
