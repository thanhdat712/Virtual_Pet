#ifndef PLAYER_GAME_DATA_H
#define PLAYER_GAME_DATA_H

#include <memory>
#include <string>
#include "../Inventory/Inventory.h"

class Pet;

class PlayerData {
private:
    int userId;
    std::string name;
    int coins;
    Inventory inventory;
    std::shared_ptr<Pet> pet;

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

    void setPet(std::shared_ptr<Pet> ownedPet);
    std::shared_ptr<Pet> getPet();
    std::shared_ptr<const Pet> getPet() const;
    bool feedPet(int foodItemId);
};

#endif
