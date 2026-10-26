#ifndef REWARD_H
#define REWARD_H

#include <vector>
#include <memory>
#include "../Item/Item.h"

class PlayerData;

// Phan thuong nguoi choi nhan duoc (vi du sau khi hoan thanh bai tap / nhiem vu).
// Gom mot so xu va (tuy chon) cac item kem theo.
class Reward {
private:
    int coins;                                 // so xu thuong
    std::vector<std::shared_ptr<Item>> items;  // cac item thuong kem theo

public:
    Reward(int coins = 0);

    void addItem(std::shared_ptr<Item> item);  // them 1 item vao phan thuong

    int getCoins() const;
    const std::vector<std::shared_ptr<Item>>& getItems() const;

    // Trao phan thuong cho nguoi choi: cong xu + bo item vao tui do (noi Reward -> Inventory)
    void grantTo(PlayerData& player) const;
};

#endif
