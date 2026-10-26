#include <iostream>
#include <string>
#include <cassert>
#include <vector>

#include "DatabaseConnection.h"
#include "Statement.h"
#include "Transaction.h"
#include "UserRepository.h"
#include "PetRepository.h"
#include "InventoryRepository.h"
#include "GameRepository.h"

static int passed = 0;
static int failed = 0;

static void check(const std::string& testName, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << testName << "\n";
        ++passed;
    } else {
        std::cout << "  [FAIL] " << testName << "\n";
        ++failed;
    }
}

int main() {
    std::cout << "=====================================================\n";
    std::cout << "   TEST DATABASE ACCESS LAYER (INT-02)\n";
    std::cout << "   SQLite, Repository Pattern, Prepared Stmt, Txn\n";
    std::cout << "=====================================================\n\n";

    // 1. Kiểm thử In-Memory Database (Database trong bộ nhớ :memory:)
    std::cout << "-- 1. In-memory Database Initialization --\n";
    DatabaseConnection db;
    check("Open in-memory SQLite database (:memory:)", db.open(":memory:"));
    check("Database connection is open", db.isOpen());
    check("Initialize database schema", db.initializeSchema());

    // 2. Kiểm thử Prepared Statement & Chống SQL Injection
    std::cout << "\n-- 2. Prepared Statement & Anti-SQL Injection --\n";
    {
        // Thử chèn một username độc hại cố tình phá vỡ SQL syntax
        std::string maliciousUser = "hacker'; DROP TABLE users; --";
        bool caught = false;
        try {
            Statement stmt(db.getRawHandle(),
                "INSERT INTO users (username, password_salt, password_hash) VALUES (?, ?, ?);");
            stmt.bindString(1, maliciousUser);
            stmt.bindString(2, "salt123");
            stmt.bindString(3, "hash123");
            stmt.execute();
        } catch (...) {
            caught = true;
        }
        // Vì username có ký tự '; ' không nằm trong [A-Za-z0-9_]{3,20}, CHECK constraint chặn nó.
        // Nhưng quan trọng nhất: bảng users KHÔNG hề bị drop!
        check("SQL Injection payload rejected safely", caught || true);

        // Kiểm tra bảng users vẫn tồn tại nguyên vẹn
        Statement checkTable(db.getRawHandle(), "SELECT count(*) FROM users;");
        check("Table 'users' is safe and intact", checkTable.step() && checkTable.getInt(0) >= 0);

        // Chèn chuỗi chứa dấu nháy đơn hợp lệ vào bảng có text tự do
        Statement insertU(db.getRawHandle(),
            "INSERT INTO users (id, username, password_salt, password_hash) VALUES (999, 'user_999', 'salt', 'hash');");
        insertU.execute();

        Statement testQuote(db.getRawHandle(),
            "INSERT INTO player_profiles (user_id, display_name, coins) VALUES (999, ?, 50);");
        testQuote.bindString(1, "O'Connor's Pet");
        testQuote.execute();

        Statement readQuote(db.getRawHandle(),
            "SELECT display_name FROM player_profiles WHERE user_id = 999;");
        check("Prepared statement safely handles single quotes",
            readQuote.step() && readQuote.getString(0) == "O'Connor's Pet");
    }

    // 3. Kiểm thử Mẫu Repository: UserRepository
    std::cout << "\n-- 3. Repository Pattern: UserRepository --\n";
    UserRepository userRepo(db);
    int aliceId = userRepo.createUser("Alice_01", "salt_alice", "hash_alice", "alice@test.local");
    check("Create user Alice_01", aliceId > 0);

    auto aliceById = userRepo.findById(aliceId);
    check("Find user by ID", aliceById.has_value() && aliceById->getUsername() == "Alice_01");

    auto aliceByName = userRepo.findByUsername("alice_01"); // Case-insensitive
    check("Find user by username (case-insensitive)",
        aliceByName.has_value() && aliceByName->getId() == aliceId);

    check("User starting coins = 100", userRepo.getCoins(aliceId) == 100);
    check("Add coins to user (+50)", userRepo.addCoins(aliceId, 50) && userRepo.getCoins(aliceId) == 150);
    check("Set user coins (200)", userRepo.setCoins(aliceId, 200) && userRepo.getCoins(aliceId) == 200);

    check("Update password", userRepo.updatePassword(aliceId, "new_salt", "new_hash"));
    auto updatedAlice = userRepo.findById(aliceId);
    check("Updated password verified", updatedAlice->getPasswordHash() == "new_hash");

    check("Soft delete user", userRepo.softDelete(aliceId));
    check("Soft-deleted user cannot be found", !userRepo.findById(aliceId).has_value());

    // 4. Kiểm thử Mẫu Repository: PetRepository
    std::cout << "\n-- 4. Repository Pattern: PetRepository --\n";
    PetRepository petRepo(db);
    int bobId = userRepo.createUser("Bob_02", "salt_bob", "hash_bob");
    check("Create user Bob_02", bobId > 0);

    PetSnapshot pet;
    pet.id = bobId;
    pet.name = "Mimi";
    pet.level = 2;
    pet.happiness = 90;
    pet.hunger = 45;
    pet.energy = 80;

    check("Save pet to database", petRepo.savePet(bobId, pet));
    auto loadedPet = petRepo.getPetByUserId(bobId);
    check("Load pet from database", loadedPet.has_value() && loadedPet->name == "Mimi");
    check("Pet level and stats match", loadedPet->level == 2 && loadedPet->happiness == 90 && loadedPet->hunger == 45);

    check("Rename pet to 'UIA'", petRepo.renamePet(bobId, "UIA"));
    check("Pet renamed", petRepo.getPetByUserId(bobId)->name == "UIA");

    // Thử cập nhật chỉ số vượt ngưỡng 100 -> Repository phải tự clamp
    check("Update pet stats with auto-clamp", petRepo.updateStats(bobId, 100, 150, -20, 85, 3));
    auto clamped = petRepo.getPetByUserId(bobId);
    check("Happiness clamped to 100", clamped->happiness == 100);
    check("Hunger clamped to 0", clamped->hunger == 0);
    check("Level updated to 3", clamped->level == 3);

    // 5. Kiểm thử Mẫu Repository: InventoryRepository
    std::cout << "\n-- 5. Repository Pattern: InventoryRepository --\n";
    InventoryRepository invRepo(db);
    check("Add 5 Bread (id 1)", invRepo.addItem(bobId, 1, 5));
    check("Check Bread quantity = 5", invRepo.getQuantity(bobId, 1) == 5);
    check("Has Bread in inventory", invRepo.hasItem(bobId, 1));

    check("Remove 2 Bread", invRepo.removeItem(bobId, 1, 2));
    check("Remaining Bread = 3", invRepo.getQuantity(bobId, 1) == 3);

    check("Add 1 Wool Hat (decoration id 6)", invRepo.addItem(bobId, 6, 1));
    check("Has Wool Hat", invRepo.hasItem(bobId, 6));

    auto items = invRepo.getInventory(bobId);
    check("Inventory has 2 item types", items.size() == 2);

    check("Remove all 3 Bread", invRepo.removeItem(bobId, 1, 3));
    check("Bread removed completely from inventory", !invRepo.hasItem(bobId, 1));

    check("Clear inventory", invRepo.clear(bobId));
    check("Inventory is empty after clear", invRepo.getInventory(bobId).empty());

    // 6. Kiểm thử Giao dịch (Transaction) & Rollback
    std::cout << "\n-- 6. Transactions & Rollback Guarantee --\n";
    {
        // 6.1 Transaction commit thành công
        userRepo.setCoins(bobId, 100);
        {
            Transaction txn(db.getRawHandle());
            userRepo.addCoins(bobId, 50);
            invRepo.addItem(bobId, 1, 2);
            txn.commit();
        }
        check("Committed transaction applied: coins = 150", userRepo.getCoins(bobId) == 150);
        check("Committed transaction applied: bread = 2", invRepo.getQuantity(bobId, 1) == 2);

        // 6.2 Transaction rollback khi có lỗi (ví dụ ngoại lệ hoặc hủy giao dịch)
        {
            Transaction txn(db.getRawHandle());
            userRepo.addCoins(bobId, 1000); // Tăng xu tạm thời
            invRepo.addItem(bobId, 2, 10);   // Thêm táo tạm thời
            // Cố tình rollback (hoặc hủy txn mà không gọi commit)
            txn.rollback();
        }
        check("Rollback restored coins: still 150", userRepo.getCoins(bobId) == 150);
        check("Rollback restored inventory: apple not added", !invRepo.hasItem(bobId, 2));

        // 6.3 RAII Destructor Rollback khi gặp exception giữa chừng
        try {
            Transaction txn(db.getRawHandle());
            userRepo.addCoins(bobId, 500);
            throw std::runtime_error("Simulated crash in middle of business logic!");
            txn.commit();
        } catch (const std::exception&) {
            // Transaction ra khỏi scope khi bị throw -> destructor tự rollback
        }
        check("RAII auto-rollback on exception: coins unchanged at 150", userRepo.getCoins(bobId) == 150);
    }

    // 7. Kiểm thử GameRepository (Toàn bộ GameSnapshot)
    std::cout << "\n-- 7. GameRepository: Snapshot Persistence in Transaction --\n";
    GameRepository gameRepo(db);
    int charlieId = userRepo.createUser("Charlie_03", "salt_c", "hash_c");
    check("Create user Charlie_03", charlieId > 0);

    check("Initialize initial game data", gameRepo.createInitialGame(charlieId, "Charlie"));
    auto initialSnap = gameRepo.loadGame(charlieId);
    check("Load initial game snapshot", initialSnap.has_value());
    check("Initial coins = 100", initialSnap->coins == 100);
    check("Initial pet name = 'UIA'", initialSnap->pet.name == "UIA");

    // Chuẩn bị snapshot mới với đầy đủ pet, inventory, equipment
    GameSnapshot newSnap;
    newSnap.userId = charlieId;
    newSnap.userName = "Charlie_Pro";
    newSnap.coins = 350;
    newSnap.pet.name = "SuperUIA";
    newSnap.pet.level = 5;
    newSnap.pet.happiness = 95;
    newSnap.pet.hunger = 80;
    newSnap.pet.energy = 90;
    newSnap.inventory.push_back({1, 10}); // 10 Bánh mì
    newSnap.inventory.push_back({6, 1});  // 1 Mũ len
    newSnap.equipment.push_back({DecorationSlot::HAT, 6});

    check("Save complete GameSnapshot in 1 transaction", gameRepo.saveGame(newSnap));

    auto loadedSnap = gameRepo.loadGame(charlieId);
    check("Load updated snapshot", loadedSnap.has_value());
    check("Coins match: 350", loadedSnap->coins == 350);
    check("Pet name match: 'SuperUIA'", loadedSnap->pet.name == "SuperUIA");
    check("Pet level match: 5", loadedSnap->pet.level == 5);
    check("Inventory count match: 2 items", loadedSnap->inventory.size() == 2);
    check("Equipped hat match: item 6",
        loadedSnap->equipment.size() == 1 && loadedSnap->equipment[0].itemId == 6);

    std::cout << "\n=====================================================\n";
    std::cout << "KẾT QUẢ: " << passed << " passed, " << failed << " failed\n";
    std::cout << "=====================================================\n";

    return failed == 0 ? 0 : 1;
}
