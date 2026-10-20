#ifndef USER_H
#define USER_H

#include <string>
#include "../Inventory/Inventory.h"

// Người chơi: có xu để mua đồ và sở hữu một túi đồ (Inventory)
class User {
private:
    int id;
    std::string name;
    int coins;            // số xu hiện có, dùng để mua đồ ở Shop
    Inventory inventory;  // túi đồ riêng của người chơi này

public:
    User(int id, const std::string& name, int coins = 0);

    int getId() const;
    std::string getName() const;

    int getCoins() const;
    void addCoins(int amount);       // cộng xu (ví dụ nhận thưởng)
    bool spendCoins(int amount);     // trừ xu, trả về false nếu không đủ

    Inventory& getInventory();             // lấy túi đồ để thêm/bớt item
    const Inventory& getInventory() const; // bản chỉ-đọc
};

#endif
