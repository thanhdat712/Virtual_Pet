
#ifndef GAME_H
#define GAME_H

#include "GameState.h"

class Game {
private:
    GameState gameState;

public:
    Game();

    void initialize();
    void start();
    void pause();
    void resume();
    void end();

    GameStatus getStatus() const;
};

#endif
