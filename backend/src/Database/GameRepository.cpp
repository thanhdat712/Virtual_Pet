#include "GameRepository.h"
#include "Transaction.h"
#include "Statement.h"
#include <sqlite3.h>

GameRepository::GameRepository(DatabaseConnection& dbConnection)
    : db(dbConnection),
      userRepo(dbConnection),
      petRepo(dbConnection),
      inventoryRepo(dbConnection) {}

bool GameRepository::createInitialGame(int userId, const std::string& playerName) {
    if (!db.isOpen()) return false;

    try {
        Transaction txn(db.getRawHandle());

        // 1. Hồ sơ người chơi (100 coins khởi điểm)
        Statement profileStmt(db.getRawHandle(),
            "INSERT INTO player_profiles (user_id, display_name, coins) "
            "VALUES (?, ?, 100) "
            "ON CONFLICT(user_id) DO NOTHING;");
        profileStmt.bindInt(1, userId);
        profileStmt.bindString(2, playerName.empty() ? "Player" : playerName);
        profileStmt.execute();

        // 2. Thú cưng ban đầu (UIA, Lv 1, Vui 85, Đói 30, Năng lượng 70)
        Statement petStmt(db.getRawHandle(),
            "INSERT INTO pets (user_id, name, level, experience, happiness, hunger, energy) "
            "VALUES (?, 'UIA', 1, 0, 0, 50, 50) "
            "ON CONFLICT(user_id) DO NOTHING;");
        petStmt.bindInt(1, userId);
        petStmt.execute();

        txn.commit();
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<GameSnapshot> GameRepository::loadGame(int userId) {
    if (!db.isOpen()) return std::nullopt;

    try {
        GameSnapshot snapshot;
        snapshot.userId = userId;

        // 1. Đọc Player profile
        Statement profileStmt(db.getRawHandle(),
            "SELECT display_name, coins FROM player_profiles WHERE user_id = ?;");
        profileStmt.bindInt(1, userId);
        if (profileStmt.step()) {
            snapshot.userName = profileStmt.getString(0);
            snapshot.coins = profileStmt.getInt(1);
        } else {
            return std::nullopt; // Người chơi chưa có hồ sơ
        }

        // 2. Đọc Pet
        auto petOpt = petRepo.getPetByUserId(userId);
        if (!petOpt.has_value()) {
            return std::nullopt; // Chưa có thông tin thú cưng
        }
        snapshot.pet = *petOpt;

        // 3. Đọc Túi đồ (Inventory)
        snapshot.inventory = inventoryRepo.getInventory(userId);

        // 4. Đọc Đồ trang trí đang trang bị (Equipment)
        Statement eqStmt(db.getRawHandle(),
            "SELECT slot, item_id FROM equipped_decorations WHERE user_id = ?;");
        eqStmt.bindInt(1, userId);
        while (eqStmt.step()) {
            EquipmentEntrySnapshot eq;
            eq.slot = static_cast<DecorationSlot>(eqStmt.getInt(0));
            eq.itemId = eqStmt.getInt(1);
            snapshot.equipment.push_back(eq);
        }

        return snapshot;
    } catch (...) {
        return std::nullopt;
    }
}

bool GameRepository::saveGame(const GameSnapshot& snapshot) {
    if (!db.isOpen()) return false;

    try {
        // Áp dụng GIAO DỊCH (Transaction) đảm bảo tính toàn vẹn (ACID).
        // Nếu bất kỳ thao tác nào bị lỗi, destructor của txn sẽ tự động ROLLBACK toàn bộ.
        Transaction txn(db.getRawHandle());

        // 1. Cập nhật Player Profile (coins & display name)
        Statement profileStmt(db.getRawHandle(),
            "INSERT INTO player_profiles (user_id, display_name, coins) "
            "VALUES (?, ?, ?) "
            "ON CONFLICT(user_id) DO UPDATE SET "
            "display_name = excluded.display_name, "
            "coins = excluded.coins;");
        profileStmt.bindInt(1, snapshot.userId);
        profileStmt.bindString(2, snapshot.userName.empty() ? "Player" : snapshot.userName);
        profileStmt.bindInt(3, snapshot.coins);
        profileStmt.execute();

        // 2. Cập nhật Thú cưng (Pet)
        if (!petRepo.savePet(snapshot.userId, snapshot.pet)) {
            txn.rollback();
            return false;
        }

        // 3. Làm mới Túi đồ (Inventory)
        Statement delInv(db.getRawHandle(), "DELETE FROM inventory WHERE user_id = ?;");
        delInv.bindInt(1, snapshot.userId);
        delInv.execute();

        for (const auto& item : snapshot.inventory) {
            Statement insInv(db.getRawHandle(),
                "INSERT INTO inventory (user_id, item_id, quantity) VALUES (?, ?, ?);");
            insInv.bindInt(1, snapshot.userId);
            insInv.bindInt(2, item.itemId);
            insInv.bindInt(3, item.quantity);
            insInv.execute();
        }

        // 4. Làm mới Đồ trang trí đang mang (Equipment)
        Statement delEq(db.getRawHandle(), "DELETE FROM equipped_decorations WHERE user_id = ?;");
        delEq.bindInt(1, snapshot.userId);
        delEq.execute();

        for (const auto& eq : snapshot.equipment) {
            Statement insEq(db.getRawHandle(),
                "INSERT INTO equipped_decorations (user_id, slot, item_id) VALUES (?, ?, ?);");
            insEq.bindInt(1, snapshot.userId);
            insEq.bindInt(2, static_cast<int>(eq.slot));
            insEq.bindInt(3, eq.itemId);
            insEq.execute();
        }

        // Thành công trọn vẹn mọi bước -> Cam kết lưu (COMMIT)
        txn.commit();
        return true;
    } catch (...) {
        // Bất kỳ ngoại lệ nào xảy ra sẽ dẫn tới việc không commit, txn sẽ tự rollback
        return false;
    }
}
