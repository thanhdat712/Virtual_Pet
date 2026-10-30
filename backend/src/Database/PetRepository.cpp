#include "PetRepository.h"
#include "Statement.h"
#include <sqlite3.h>
#include <algorithm>

namespace {
int clampStat(int val) {
    if (val < 0) return 0;
    if (val > 100) return 100;
    return val;
}
}

PetRepository::PetRepository(DatabaseConnection& dbConnection)
    : db(dbConnection) {}

std::optional<PetSnapshot> PetRepository::getPetByUserId(int userId) {
    if (!db.isOpen()) return std::nullopt;

    try {
        Statement stmt(
            db.getRawHandle(),
            "SELECT name, level, happiness, hunger, energy, experience "
            "FROM pets WHERE user_id = ?;"
        );

        stmt.bindInt(1, userId);

        if (stmt.step()) {
            PetSnapshot snapshot;
            snapshot.id = userId;
            snapshot.name = stmt.getString(0);
            snapshot.level = std::max(1, stmt.getInt(1));
            snapshot.health = 100;
            snapshot.happiness = clampStat(stmt.getInt(2));
            snapshot.hunger = clampStat(stmt.getInt(3));
            snapshot.energy = clampStat(stmt.getInt(4));
            snapshot.experience = std::max(0, stmt.getInt(5));

            return snapshot;
        }
    } catch (...) {
        return std::nullopt;
    }

    return std::nullopt;
}

bool PetRepository::savePet(int userId, const PetSnapshot& pet) {
    if (!db.isOpen()) return false;

    try {
        Statement stmt(
            db.getRawHandle(),
            "INSERT INTO pets "
            "(user_id, name, level, experience, happiness, hunger, energy) "
            "VALUES (?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(user_id) DO UPDATE SET "
            "name = excluded.name, "
            "level = excluded.level, "
            "experience = excluded.experience, "
            "happiness = excluded.happiness, "
            "hunger = excluded.hunger, "
            "energy = excluded.energy;"
        );

        stmt.bindInt(1, userId);
        stmt.bindString(2, pet.name.empty() ? "UIA" : pet.name);
        stmt.bindInt(3, std::max(1, pet.level));
        stmt.bindInt(4, std::max(0, pet.experience));
        stmt.bindInt(5, clampStat(pet.happiness));
        stmt.bindInt(6, clampStat(pet.hunger));
        stmt.bindInt(7, clampStat(pet.energy));

        stmt.execute();
        return true;
    } catch (...) {
        return false;
    }
}

bool PetRepository::renamePet(int userId, const std::string& name) {
    if (!db.isOpen() || name.empty()) return false;

    try {
        Statement stmt(
            db.getRawHandle(),
            "UPDATE pets SET name = ? WHERE user_id = ?;"
        );

        stmt.bindString(1, name);
        stmt.bindInt(2, userId);
        stmt.execute();

        return sqlite3_changes(db.getRawHandle()) > 0;
    } catch (...) {
        return false;
    }
}

bool PetRepository::updateStats(
    int userId,
    int health,
    int happiness,
    int hunger,
    int energy,
    int level,
    int experience
) {
    (void)health;

    if (!db.isOpen()) return false;

    try {
        Statement stmt(
            db.getRawHandle(),
            "UPDATE pets SET "
            "level = ?, happiness = ?, hunger = ?, "
            "energy = ?, experience = ? "
            "WHERE user_id = ?;"
        );

        stmt.bindInt(1, std::max(1, level));
        stmt.bindInt(2, clampStat(happiness));
        stmt.bindInt(3, clampStat(hunger));
        stmt.bindInt(4, clampStat(energy));
        stmt.bindInt(5, std::max(0, experience));
        stmt.bindInt(6, userId);

        stmt.execute();
        return sqlite3_changes(db.getRawHandle()) > 0;
    } catch (...) {
        return false;
    }
}