#include "User.h"

User::User(int id, const std::string& name, int coins)
    : id(id), name(name), coins(coins) {}

int User::getId() const {
    return id;
}

std::string User::getName() const {
    return name;
}

int User::getCoins() const {
    return coins;
}

void User::addCoins(int amount) {
    if (amount > 0) {
        coins += amount;
    }
}

bool User::spendCoins(int amount) {
    if (amount < 0 || coins < amount) {
        return false;
    }
    coins -= amount;
    return true;
}

Inventory& User::getInventory() {
    return inventory;
}

const Inventory& User::getInventory() const {
    return inventory;
}
