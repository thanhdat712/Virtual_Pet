#ifndef INVENTORY_REPOSITORY_H
#define INVENTORY_REPOSITORY_H

#include "IInventoryRepository.h"
#include "DatabaseConnection.h"

// Triển khai IInventoryRepository sử dụng SQLite và Prepared Statements
class InventoryRepository : public IInventoryRepository {
private:
    DatabaseConnection& db;

public:
    explicit InventoryRepository(DatabaseConnection& dbConnection);

    std::vector<InventoryEntrySnapshot> getInventory(int userId) override;
    int getQuantity(int userId, int itemId) override;
    bool hasItem(int userId, int itemId) override;

    bool setItemQuantity(int userId, int itemId, int quantity) override;
    bool addItem(int userId, int itemId, int quantity) override;
    bool removeItem(int userId, int itemId, int quantity) override;
    bool clear(int userId) override;
};

#endif
