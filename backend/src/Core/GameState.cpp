
#include "GameState.h"

GameState::GameState()
    : status(GameStatus::NOT_STARTED) {
}

GameStatus GameState::getStatus() const {
    return status;
}

void GameState::setStatus(GameStatus newStatus) {
    status = newStatus;
}
