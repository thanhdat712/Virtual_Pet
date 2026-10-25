#ifndef GAME_H
#define GAME_H

#include <memory>
#include "GameState.h"
#include "ActionResult.h"
#include "GameSnapshot.h"
#include "PlayerGameData.h"

class Reward;
class Inventory;

// Điểm truy cập DUY NHẤT của tầng API vào game (KAN-37/38/39).
class Game {
private:
    GameState gameState;
    int timeCarrySeconds = 0;

public:
    static constexpr int TICK_SECONDS = 60;

    Game();

    void initialize();
    ActionResult start();
    ActionResult pause();
    ActionResult resume();
    ActionResult end();

    GameStatus getStatus() const;

    ActionResult feed(int foodId);
    ActionResult play();
    ActionResult sleep();
    ActionResult bath();
    ActionResult buy(int itemId, int quantity = 1);
    ActionResult decorate(int itemId);
    ActionResult unequipDecoration(DecorationSlot slot);
    ActionResult claimReward(const Reward& reward);
    ActionResult updateOverTime(int elapsedSeconds);

    GameSnapshot exportSnapshot() const;
    ActionResult importSnapshot(const GameSnapshot& snapshot);

    PlayerData& getPlayer();
    const PlayerData& getPlayer() const;
    Pet& getPet();
    const Pet& getPet() const;
    Shop& getShop();
    const Shop& getShop() const;
    const Inventory& getInventory() const;
    int getCoins() const;
};

#endif
