#ifndef I_INVENTORY_REPOSITORY_H
#define I_INVENTORY_REPOSITORY_H

#include <vector>
#include "../Core/GameSnapshot.h"

// Interface định nghĩa các thao tác lưu trữ túi đồ (Inventory) theo Mẫu Repository
class IInventoryRepository {
public:
    virtual ~IInventoryRepository() = default;

    virtual std::vector<InventoryEntrySnapshot> getInventory(int userId) = 0;
    virtual int getQuantity(int userId, int itemId) = 0;
    virtual bool hasItem(int userId, int itemId) = 0;

    virtual bool setItemQuantity(int userId, int itemId, int quantity) = 0;
    virtual bool addItem(int userId, int itemId, int quantity) = 0;
    virtual bool removeItem(int userId, int itemId, int quantity) = 0;
    virtual bool clear(int userId) = 0;
};

#endif
