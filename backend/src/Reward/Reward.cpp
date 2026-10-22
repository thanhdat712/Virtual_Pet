#include "Reward.h"
#include "../User/User.h"

Reward::Reward(int coins) : coins(coins) {}

void Reward::addItem(std::shared_ptr<Item> item) {
    items.push_back(item);
}

int Reward::getCoins() const {
    return coins;
}

const std::vector<std::shared_ptr<Item>>& Reward::getItems() const {
    return items;
}

void Reward::grantTo(User& user) const {
    user.addCoins(coins);
    for (const auto& item : items) {
        user.getInventory().addItem(item, 1); // moi item thuong them 1 cai vao tui
    }
}
