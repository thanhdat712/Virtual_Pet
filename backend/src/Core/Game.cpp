
#include "Game.h"

Game::Game() {
    // GameState đã tự khởi tạo trong constructor của nó
}

void Game::initialize() {
    gameState.initialize();
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

PlayerData& Game::getPlayer() { return gameState.getPlayer(); }
const PlayerData& Game::getPlayer() const { return gameState.getPlayer(); }
Pet& Game::getPet() { return gameState.getPet(); }
const Pet& Game::getPet() const { return gameState.getPet(); }
Shop& Game::getShop() { return gameState.getShop(); }
const Shop& Game::getShop() const { return gameState.getShop(); }
