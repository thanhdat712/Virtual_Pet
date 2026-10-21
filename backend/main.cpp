#include <iostream>
#include "Game.h"
#include "User.h"
#include "Pet.h"
#include "Shop.h"

int main() {
    Game game;

    if (game.getStatus() != GameStatus::NOT_STARTED) {
        std::cout << "Game initialization failed!\n";
        return 1;
    }

    std::cout << "Game initialized successfully!\n";
    std::cout << "Player: " << game.getPlayer().getName()
              << " (" << game.getPlayer().getCoins() << " coins)\n";
    std::cout << "Pet: " << game.getPet().getName() << "\n";
    std::cout << "Shop items: " << game.getShop().getItemsForSale().size() << "\n";

    return 0;
}
