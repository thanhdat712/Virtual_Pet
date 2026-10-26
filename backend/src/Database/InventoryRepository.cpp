#include "InventoryRepository.h"
#include "Statement.h"
#include <sqlite3.h>

InventoryRepository::InventoryRepository(DatabaseConnection& dbConnection)
    : db(dbConnection) {}

std::vector<InventoryEntrySnapshot> InventoryRepository::getInventory(int userId) {
    std::vector<InventoryEntrySnapshot> result;
    if (!db.isOpen()) return result;

    try {
        Statement stmt(db.getRawHandle(),
            "SELECT item_id, quantity FROM inventory WHERE user_id = ? ORDER BY item_id ASC;");
        stmt.bindInt(1, userId);

        while (stmt.step()) {
            InventoryEntrySnapshot entry;
            entry.itemId = stmt.getInt(0);
            entry.quantity = stmt.getInt(1);
            result.push_back(entry);
        }
    } catch (...) {
        return result;
    }

    return result;
}

int InventoryRepository::getQuantity(int userId, int itemId) {
    if (!db.isOpen()) return 0;

    try {
        Statement stmt(db.getRawHandle(),
            "SELECT quantity FROM inventory WHERE user_id = ? AND item_id = ?;");
        stmt.bindInt(1, userId);
        stmt.bindInt(2, itemId);

        if (stmt.step()) {
            return stmt.getInt(0);
        }
    } catch (...) {
        return 0;
    }

    return 0;
}

bool InventoryRepository::hasItem(int userId, int itemId) {
    return getQuantity(userId, itemId) > 0;
}

bool InventoryRepository::setItemQuantity(int userId, int itemId, int quantity) {
    if (!db.isOpen()) return false;

    try {
        if (quantity <= 0) {
            Statement stmt(db.getRawHandle(),
                "DELETE FROM inventory WHERE user_id = ? AND item_id = ?;");
            stmt.bindInt(1, userId);
            stmt.bindInt(2, itemId);
            stmt.execute();
            return true;
        }

        Statement stmt(db.getRawHandle(),
            "INSERT INTO inventory (user_id, item_id, quantity) VALUES (?, ?, ?) "
            "ON CONFLICT(user_id, item_id) DO UPDATE SET quantity = excluded.quantity;");
        stmt.bindInt(1, userId);
        stmt.bindInt(2, itemId);
        stmt.bindInt(3, quantity);
        stmt.execute();
        return true;
    } catch (...) {
        return false;
    }
}

bool InventoryRepository::addItem(int userId, int itemId, int quantity) {
    if (quantity <= 0) return false;
    int current = getQuantity(userId, itemId);
    return setItemQuantity(userId, itemId, current + quantity);
}

bool InventoryRepository::removeItem(int userId, int itemId, int quantity) {
    if (quantity <= 0) return false;
    int current = getQuantity(userId, itemId);
    if (current < quantity) return false; // Không đủ số lượng để trừ
    return setItemQuantity(userId, itemId, current - quantity);
}

bool InventoryRepository::clear(int userId) {
    if (!db.isOpen()) return false;

    try {
        Statement stmt(db.getRawHandle(),
            "DELETE FROM inventory WHERE user_id = ?;");
        stmt.bindInt(1, userId);
        stmt.execute();
        return true;
    } catch (...) {
        return false;
    }
}
