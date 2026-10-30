
#ifndef GAME_SNAPSHOT_H
#define GAME_SNAPSHOT_H

#include <string>
#include <vector>
#include "../Item/Decoration.h"

struct InventoryEntrySnapshot {
    int itemId;
    int quantity;
};

struct EquipmentEntrySnapshot {
    DecorationSlot slot;
    int itemId;
};

struct PetSnapshot {
    int id = 1;
    std::string name;
    int level = 1;
    int health = 100;
    int happiness = 0;
    int hunger = 50;
    int energy = 50;
    int experience = 0;
};

struct GameSnapshot {
    int userId = 1;
    std::string userName;
    int coins = 0;
    PetSnapshot pet;
    std::vector<InventoryEntrySnapshot> inventory;
    std::vector<EquipmentEntrySnapshot> equipment;
};

#endif
