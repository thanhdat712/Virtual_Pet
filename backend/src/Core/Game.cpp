
#include "Game.h"

Game::Game() {
    initialize();
}

void Game::initialize() {
    gameState.setStatus(GameStatus::NOT_STARTED);
}

void Game::start() {
    if (gameState.getStatus() != GameStatus::NOT_STARTED) {
        return;
    }

    gameState.setStatus(GameStatus::RUNNING);
}

void Game::pause() {
    if (gameState.getStatus() != GameStatus::RUNNING) {
        return;
    }

    gameState.setStatus(GameStatus::PAUSED);
}

void Game::resume() {
    if (gameState.getStatus() != GameStatus::PAUSED) {
        return;
    }

    gameState.setStatus(GameStatus::RUNNING);
}

void Game::end() {
    GameStatus currentStatus = gameState.getStatus();

    if (currentStatus != GameStatus::RUNNING &&
        currentStatus != GameStatus::PAUSED) {
        return;
    }

    gameState.setStatus(GameStatus::ENDED);
}

GameStatus Game::getStatus() const {
    return gameState.getStatus();
}
