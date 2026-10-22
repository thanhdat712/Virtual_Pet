#include "PlayerGameData.h"

PlayerData::PlayerData(int userId, const std::string& name, int coins)
    : userId(userId), name(name), coins(coins) {}

int PlayerData::getId() const { return userId; }
int PlayerData::getUserId() const { return userId; }
const std::string& PlayerData::getName() const { return name; }
int PlayerData::getCoins() const { return coins; }

void PlayerData::addCoins(int amount) {
    if (amount > 0) {
        coins += amount;
    }
}

bool PlayerData::spendCoins(int amount) {
    if (amount < 0 || coins < amount) {
        return false;
    }
    coins -= amount;
    return true;
}

Inventory& PlayerData::getInventory() { return inventory; }
const Inventory& PlayerData::getInventory() const { return inventory; }
