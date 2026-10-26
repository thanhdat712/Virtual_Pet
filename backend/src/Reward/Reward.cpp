#include "Reward.h"
#include "../User/PlayerGameData.h"

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

void Reward::grantTo(PlayerData& player) const {
    player.addCoins(coins);
    for (const auto& item : items) {
        player.getInventory().addItem(item, 1);
    }
}
