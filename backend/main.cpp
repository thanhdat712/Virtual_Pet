
#include <iostream>
#include "Game.h"

int main() {
    Game game;

    if (game.getStatus() == GameStatus::NOT_STARTED) {
        std::cout << "Game initialized successfully!\n";
    } else {
        std::cout << "Game initialization failed!\n";
    }

    return 0;
}
