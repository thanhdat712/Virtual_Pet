#include "UserRepository.h"
#include "Statement.h"
#include <sqlite3.h>

UserRepository::UserRepository(DatabaseConnection& dbConnection)
    : db(dbConnection) {}

std::optional<User> UserRepository::findById(int id) {
    if (!db.isOpen()) return std::nullopt;

    try {
        Statement stmt(db.getRawHandle(),
            "SELECT id, username, password_salt, password_hash, created_at, email, last_login_at "
            "FROM users WHERE id = ? AND deleted_at IS NULL;");
        stmt.bindInt(1, id);

        if (stmt.step()) {
            int uid = stmt.getInt(0);
            std::string username = stmt.getString(1);
            std::string salt = stmt.getString(2);
            std::string hash = stmt.getString(3);
            std::string createdAt = stmt.getString(4);
            std::string email = stmt.getString(5);
            std::string lastLoginAt = stmt.getString(6);

            return User(uid, username, salt, hash, createdAt, email, lastLoginAt);
        }
    } catch (...) {
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<User> UserRepository::findByUsername(const std::string& username) {
    if (!db.isOpen() || username.empty()) return std::nullopt;

    try {
        Statement stmt(db.getRawHandle(),
            "SELECT id, username, password_salt, password_hash, created_at, email, last_login_at "
            "FROM users WHERE username = ? AND deleted_at IS NULL;");
        stmt.bindString(1, username);

        if (stmt.step()) {
            int uid = stmt.getInt(0);
            std::string uname = stmt.getString(1);
            std::string salt = stmt.getString(2);
            std::string hash = stmt.getString(3);
            std::string createdAt = stmt.getString(4);
            std::string email = stmt.getString(5);
            std::string lastLoginAt = stmt.getString(6);

            return User(uid, uname, salt, hash, createdAt, email, lastLoginAt);
        }
    } catch (...) {
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<User> UserRepository::findByEmail(const std::string& email) {
    if (!db.isOpen() || email.empty()) return std::nullopt;

    try {
        Statement stmt(db.getRawHandle(),
            "SELECT id, username, password_salt, password_hash, created_at, email, last_login_at "
            "FROM users WHERE email = ? AND deleted_at IS NULL;");
        stmt.bindString(1, email);

        if (stmt.step()) {
            int uid = stmt.getInt(0);
            std::string uname = stmt.getString(1);
            std::string salt = stmt.getString(2);
            std::string hash = stmt.getString(3);
            std::string createdAt = stmt.getString(4);
            std::string uemail = stmt.getString(5);
            std::string lastLoginAt = stmt.getString(6);

            return User(uid, uname, salt, hash, createdAt, uemail, lastLoginAt);
        }
    } catch (...) {
        return std::nullopt;
    }

    return std::nullopt;
}

int UserRepository::createUser(const std::string& username, const std::string& passwordSalt,
                               const std::string& passwordHash, const std::string& email) {
    if (!db.isOpen()) return -1;

    try {
        Statement stmt(db.getRawHandle(),
            "INSERT INTO users (username, password_salt, password_hash, email) "
            "VALUES (?, ?, ?, NULLIF(?, ''));");
        stmt.bindString(1, username);
        stmt.bindString(2, passwordSalt);
        stmt.bindString(3, passwordHash);
        stmt.bindString(4, email);
        stmt.execute();

        int newId = static_cast<int>(sqlite3_last_insert_rowid(db.getRawHandle()));

        // Tự động tạo hồ sơ mặc định trong player_profiles
        Statement profileStmt(db.getRawHandle(),
            "INSERT OR IGNORE INTO player_profiles (user_id, display_name, coins) VALUES (?, ?, 100);");
        profileStmt.bindInt(1, newId);
        profileStmt.bindString(2, username);
        profileStmt.execute();

        return newId;
    } catch (...) {
        return -1;
    }
}

bool UserRepository::updatePassword(int id, const std::string& newSalt, const std::string& newHash) {
    if (!db.isOpen()) return false;

    try {
        Statement stmt(db.getRawHandle(),
            "UPDATE users SET password_salt = ?, password_hash = ? WHERE id = ? AND deleted_at IS NULL;");
        stmt.bindString(1, newSalt);
        stmt.bindString(2, newHash);
        stmt.bindInt(3, id);
        stmt.execute();
        return sqlite3_changes(db.getRawHandle()) > 0;
    } catch (...) {
        return false;
    }
}

bool UserRepository::updateLastLogin(int id, const std::string& timestamp) {
    if (!db.isOpen()) return false;

    try {
        Statement stmt(db.getRawHandle(),
            "UPDATE users SET last_login_at = ? WHERE id = ? AND deleted_at IS NULL;");
        stmt.bindString(1, timestamp);
        stmt.bindInt(2, id);
        stmt.execute();
        return sqlite3_changes(db.getRawHandle()) > 0;
    } catch (...) {
        return false;
    }
}

bool UserRepository::softDelete(int id) {
    if (!db.isOpen()) return false;

    try {
        Statement stmt(db.getRawHandle(),
            "UPDATE users SET deleted_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), "
            "username = NULL, email = NULL, password_salt = NULL, password_hash = NULL "
            "WHERE id = ? AND deleted_at IS NULL;");
        stmt.bindInt(1, id);
        stmt.execute();
        return sqlite3_changes(db.getRawHandle()) > 0;
    } catch (...) {
        return false;
    }
}

int UserRepository::getCoins(int userId) {
    if (!db.isOpen()) return 0;

    try {
        Statement stmt(db.getRawHandle(),
            "SELECT coins FROM player_profiles WHERE user_id = ?;");
        stmt.bindInt(1, userId);
        if (stmt.step()) {
            return stmt.getInt(0);
        }
    } catch (...) {
        return 0;
    }
    return 0;
}

bool UserRepository::setCoins(int userId, int coins) {
    if (!db.isOpen() || coins < 0) return false;

    try {
        Statement stmt(db.getRawHandle(),
            "INSERT INTO player_profiles (user_id, coins) VALUES (?, ?) "
            "ON CONFLICT(user_id) DO UPDATE SET coins = excluded.coins;");
        stmt.bindInt(1, userId);
        stmt.bindInt(2, coins);
        stmt.execute();
        return true;
    } catch (...) {
        return false;
    }
}

bool UserRepository::addCoins(int userId, int amount) {
    if (!db.isOpen()) return false;

    try {
        Statement stmt(db.getRawHandle(),
            "UPDATE player_profiles SET coins = coins + ? WHERE user_id = ? AND coins + ? >= 0;");
        stmt.bindInt(1, amount);
        stmt.bindInt(2, userId);
        stmt.bindInt(3, amount);
        stmt.execute();
        return sqlite3_changes(db.getRawHandle()) > 0;
    } catch (...) {
        return false;
    }
}
