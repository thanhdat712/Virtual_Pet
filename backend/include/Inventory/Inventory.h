#ifndef INVENTORY_H
#define INVENTORY_H

#include <vector>
#include <memory>
#include "../Item/Item.h"

// 1 ô trong túi đồ: chứa loại item nào và có bao nhiêu cái
struct InventorySlot {
    std::shared_ptr<Item> item;
    int quantity;
};

class Inventory {
private:
    std::vector<InventorySlot> slots;

    int findSlotIndex(int itemId) const; // tìm vị trí item trong túi, -1 nếu không có

public:
    Inventory();

    void addItem(std::shared_ptr<Item> item, int quantity);
    bool removeItem(int itemId, int quantity); // trả về false nếu không đủ số lượng để xóa

    int getQuantity(int itemId) const;
    const std::vector<InventorySlot>& getSlots() const;
};

#endif
