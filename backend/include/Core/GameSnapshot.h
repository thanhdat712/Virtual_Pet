#ifndef GAME_SNAPSHOT_H
#define GAME_SNAPSHOT_H

#include <string>
#include <vector>
#include "../Item/Decoration.h" // DecorationSlot

// Ảnh chụp dữ liệu của một ván game dưới dạng struct thuần (không con trỏ, không class game).
// Đây là "hợp đồng" giữa Game và tầng lưu trữ (KAN-72 / SQLite): tầng lưu trữ chỉ cần
// chuyển qua lại giữa GameSnapshot và các dòng trong database, không cần biết Game bên trong.
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
    int hunger = 0;
    int energy = 0;
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
