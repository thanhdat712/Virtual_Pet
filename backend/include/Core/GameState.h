
#ifndef GAME_STATE_H
#define GAME_STATE_H

enum class GameStatus {
    NOT_STARTED,
    RUNNING,
    PAUSED,
    ENDED
};

class GameState {
private:
    GameStatus status;

public:
    GameState();

    GameStatus getStatus() const;
    void setStatus(GameStatus newStatus);
};

#endif
