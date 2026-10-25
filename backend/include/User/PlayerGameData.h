#ifndef PLAYER_GAME_DATA_H
#define PLAYER_GAME_DATA_H

#include <string>
#include "../Inventory/Inventory.h"

class PlayerData {
private:
    int userId;
    std::string name;
    int coins;
    Inventory inventory;

public:
    PlayerData(int userId, const std::string& name, int coins = 0);

    int getId() const;
    int getUserId() const;
    const std::string& getName() const;

    int getCoins() const;
    void addCoins(int amount);
    bool spendCoins(int amount);

    Inventory& getInventory();
    const Inventory& getInventory() const;
};

#endif
