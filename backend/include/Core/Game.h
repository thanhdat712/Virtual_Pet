#ifndef GAME_H
#define GAME_H

#include "GameState.h"
#include "PlayerGameData.h"

class Game {
private:
    GameState gameState;

public:
    Game();

    void initialize(); // tạo lại người chơi, pet, shop và đưa game về NOT_STARTED
    void start();
    void pause();
    void resume();
    void end();

    GameStatus getStatus() const;

    // Truy cập các đối tượng của ván game (dùng cho KAN-37/38 và tầng API)
    PlayerData& getPlayer();
    const PlayerData& getPlayer() const;
    Pet& getPet();
    const Pet& getPet() const;
    Shop& getShop();
    const Shop& getShop() const;
};

#endif
