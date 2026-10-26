#include "GameState.h"
#include "PlayerGameData.h"
#include "Pet.h"
#include "Cat.h"
#include "Shop.h"

namespace {
constexpr int DEFAULT_PLAYER_ID = 1;
constexpr const char* DEFAULT_PLAYER_NAME = "Player";
constexpr int STARTING_COINS = 100;
constexpr int DEFAULT_PET_ID = 1;
constexpr const char* DEFAULT_PET_NAME = "UIA";
}

GameState::GameState()
    : status(GameStatus::NOT_STARTED) {
    initialize();
}

void GameState::initialize() {
    auto newPlayer = std::make_unique<PlayerData>(
        DEFAULT_PLAYER_ID,
        DEFAULT_PLAYER_NAME,
        STARTING_COINS
    );

    auto newPet = std::make_shared<Cat>(
        DEFAULT_PET_ID,
        DEFAULT_PET_NAME,
        1,
        100,
        85,
        30,
        70
    );

    newPlayer->setPet(newPet);
    auto newShop = std::make_unique<Shop>();

    player = std::move(newPlayer);
    pet = std::move(newPet);
    shop = std::move(newShop);
    status = GameStatus::NOT_STARTED;
}

void GameState::restore(std::unique_ptr<PlayerData> newPlayer, std::shared_ptr<Pet> newPet) {
    player = std::move(newPlayer);
    pet = std::move(newPet);
    status = GameStatus::NOT_STARTED;
}

GameStatus GameState::getStatus() const {
    return status;
}

void GameState::setStatus(GameStatus newStatus) {
    status = newStatus;
}

PlayerData& GameState::getPlayer() { return *player; }
const PlayerData& GameState::getPlayer() const { return *player; }
Pet& GameState::getPet() { return *pet; }
const Pet& GameState::getPet() const { return *pet; }
Shop& GameState::getShop() { return *shop; }
const Shop& GameState::getShop() const { return *shop; }
