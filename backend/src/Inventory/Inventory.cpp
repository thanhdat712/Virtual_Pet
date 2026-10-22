#include "GameState.h"
#include "User.h"
#include "Pet.h"
#include "Cat.h"
#include "Shop.h"

namespace {
constexpr int DEFAULT_PLAYER_ID = 1;
constexpr const char* DEFAULT_PLAYER_NAME = "Player";
constexpr int STARTING_COINS = 100;

constexpr int DEFAULT_PET_ID = 1;
constexpr const char* DEFAULT_PET_NAME = "Mimi";
}

GameState::GameState()
    : status(GameStatus::NOT_STARTED) {
    initialize();
}

GameState::~GameState() = default;

void GameState::initialize() {
    // Dựng đủ ba đối tượng vào biến tạm trước, nếu có lỗi giữa chừng thì trạng thái cũ không bị hỏng
    auto newPlayer = std::make_unique<User>(DEFAULT_PLAYER_ID, DEFAULT_PLAYER_NAME, STARTING_COINS);
    auto newPet = std::make_shared<Cat>(DEFAULT_PET_ID, DEFAULT_PET_NAME);
    newPlayer->setPet(newPet);
    auto newShop = std::make_unique<Shop>();

    player = std::move(newPlayer);
    pet = std::move(newPet);
    shop = std::move(newShop);
    status = GameStatus::NOT_STARTED;
}

GameStatus GameState::getStatus() const {
    return status;
}

void GameState::setStatus(GameStatus newStatus) {
    status = newStatus;
}

User& GameState::getPlayer() { return *player; }
const User& GameState::getPlayer() const { return *player; }
Pet& GameState::getPet() { return *pet; }
const Pet& GameState::getPet() const { return *pet; }
Shop& GameState::getShop() { return *shop; }
const Shop& GameState::getShop() const { return *shop; }
