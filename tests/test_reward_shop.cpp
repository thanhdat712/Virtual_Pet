#include <iostream>
#include "PlayerGameData.h"
#include "Reward.h"
#include "Shop.h"

int main() {
    PlayerData player(1, "Player", 100);
    Shop shop;
    if (!shop.buyItem(player, 1) || player.getCoins() != 95
        || player.getInventory().getQuantity(1) != 1) {
        std::cerr << "Shop purchase test failed.\n";
        return 1;
    }

    Reward reward(10);
    reward.addItem(shop.getItemById(2));
    reward.grantTo(player);
    if (player.getCoins() != 105 || player.getInventory().getQuantity(2) != 1) {
        std::cerr << "Reward grant test failed.\n";
        return 1;
    }

    std::cout << "Shop purchase and reward tests passed.\n";
    return 0;
}
