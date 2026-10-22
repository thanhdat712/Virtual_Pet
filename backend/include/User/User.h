#ifndef USER_H
#define USER_H

#include <string>
#include <memory>
#include "../Inventory/Inventory.h"

class Pet;

class User {
private:
    int id;
    std::string name;
    int coins;
    Inventory inventory;
    std::shared_ptr<Pet> pet;

public:
    User(int id, const std::string& name, int coins = 0);

    int getId() const;
    std::string getName() const;
    int getCoins() const;
    void addCoins(int amount);
    bool spendCoins(int amount);

    Inventory& getInventory();
    const Inventory& getInventory() const;

    void setPet(std::shared_ptr<Pet> ownedPet);
    std::shared_ptr<Pet> getPet();
    std::shared_ptr<const Pet> getPet() const;
    // Chi tru do an neu pet an thanh cong.
    bool feedPet(int foodItemId);
};

#endif
