#include "DatabaseConnection.h"
#include "Statement.h"
#include "Transaction.h"

#include <sqlite3.h>
#include <stdexcept>
#include <iostream>
#include <utility>

// -------------------------------------------------------------
// Triển khai Statement (Prepared Statement RAII)
// -------------------------------------------------------------

Statement::Statement(sqlite3* db, const std::string& sql)
    : dbHandle(db) {
    if (!db) {
        throw std::runtime_error("Database connection is null.");
    }
    if (sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to prepare statement: ") + sqlite3_errmsg(db));
    }
}

Statement::~Statement() {
    if (stmt) {
        sqlite3_finalize(stmt);
        stmt = nullptr;
    }
}

Statement::Statement(Statement&& other) noexcept
    : stmt(other.stmt), dbHandle(other.dbHandle) {
    other.stmt = nullptr;
    other.dbHandle = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (stmt) sqlite3_finalize(stmt);
        stmt = other.stmt;
        dbHandle = other.dbHandle;
        other.stmt = nullptr;
        other.dbHandle = nullptr;
    }
    return *this;
}

void Statement::bindInt(int index, int value) {
    if (sqlite3_bind_int(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error(std::string("Bind int failed: ") + sqlite3_errmsg(dbHandle));
    }
}

void Statement::bindInt64(int index, std::int64_t value) {
    if (sqlite3_bind_int64(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error(std::string("Bind int64 failed: ") + sqlite3_errmsg(dbHandle));
    }
}

void Statement::bindDouble(int index, double value) {
    if (sqlite3_bind_double(stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error(std::string("Bind double failed: ") + sqlite3_errmsg(dbHandle));
    }
}

void Statement::bindString(int index, const std::string& value) {
    if (sqlite3_bind_text(stmt, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        throw std::runtime_error(std::string("Bind string failed: ") + sqlite3_errmsg(dbHandle));
    }
}

void Statement::bindNull(int index) {
    if (sqlite3_bind_null(stmt, index) != SQLITE_OK) {
        throw std::runtime_error(std::string("Bind null failed: ") + sqlite3_errmsg(dbHandle));
    }
}

bool Statement::step() {
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;
    throw std::runtime_error(std::string("Step failed: ") + sqlite3_errmsg(dbHandle));
}

void Statement::execute() {
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error(std::string("Execute statement failed: ") + sqlite3_errmsg(dbHandle));
    }
}

int Statement::getInt(int column) const {
    return sqlite3_column_int(stmt, column);
}

std::int64_t Statement::getInt64(int column) const {
    return sqlite3_column_int64(stmt, column);
}

double Statement::getDouble(int column) const {
    return sqlite3_column_double(stmt, column);
}

std::string Statement::getString(int column) const {
    const auto* text = sqlite3_column_text(stmt, column);
    if (!text) return "";
    return std::string(reinterpret_cast<const char*>(text), sqlite3_column_bytes(stmt, column));
}

bool Statement::isNull(int column) const {
    return sqlite3_column_type(stmt, column) == SQLITE_NULL;
}

void Statement::reset() {
    sqlite3_reset(stmt);
}

void Statement::clearBindings() {
    sqlite3_clear_bindings(stmt);
}

// -------------------------------------------------------------
// Triển khai Transaction (Giao dịch RAII)
// -------------------------------------------------------------

Transaction::Transaction(sqlite3* db, bool immediate)
    : db(db) {
    if (!db) {
        throw std::runtime_error("Database connection is null for transaction.");
    }
    const char* sql = immediate ? "BEGIN IMMEDIATE;" : "BEGIN;";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "Failed to begin transaction";
        sqlite3_free(errMsg);
        throw std::runtime_error(err);
    }
}

Transaction::~Transaction() {
    if (!committed && !rolledBack && db) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
}

Transaction::Transaction(Transaction&& other) noexcept
    : db(other.db), committed(other.committed), rolledBack(other.rolledBack) {
    other.db = nullptr;
    other.committed = true;
    other.rolledBack = true;
}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (!committed && !rolledBack && db) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
        db = other.db;
        committed = other.committed;
        rolledBack = other.rolledBack;
        other.db = nullptr;
        other.committed = true;
        other.rolledBack = true;
    }
    return *this;
}

void Transaction::commit() {
    if (committed || rolledBack) {
        throw std::runtime_error("Transaction already finished.");
    }
    char* errMsg = nullptr;
    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "Failed to commit transaction";
        sqlite3_free(errMsg);
        throw std::runtime_error(err);
    }
    committed = true;
}

void Transaction::rollback() {
    if (committed || rolledBack) {
        throw std::runtime_error("Transaction already finished.");
    }
    char* errMsg = nullptr;
    if (sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "Failed to rollback transaction";
        sqlite3_free(errMsg);
        throw std::runtime_error(err);
    }
    rolledBack = true;
}

// -------------------------------------------------------------
// Triển khai DatabaseConnection
// -------------------------------------------------------------

DatabaseConnection::DatabaseConnection() = default;

DatabaseConnection::~DatabaseConnection() {
    close();
}

DatabaseConnection::DatabaseConnection(DatabaseConnection&& other) noexcept
    : db(other.db), dbPath(std::move(other.dbPath)), lastErrorMessage(std::move(other.lastErrorMessage)) {
    other.db = nullptr;
}

DatabaseConnection& DatabaseConnection::operator=(DatabaseConnection&& other) noexcept {
    if (this != &other) {
        close();
        db = other.db;
        dbPath = std::move(other.dbPath);
        lastErrorMessage = std::move(other.lastErrorMessage);
        other.db = nullptr;
    }
    return *this;
}

bool DatabaseConnection::open(const std::string& path) {
    close();
    dbPath = path;

    int rc = sqlite3_open(path.c_str(), &db);
    if (rc != SQLITE_OK) {
        lastErrorMessage = db ? sqlite3_errmsg(db) : "Failed to open SQLite database.";
        close();
        return false;
    }

    sqlite3_busy_timeout(db, 5000);

    // Kích hoạt ràng buộc khóa ngoại (Foreign Keys)
    if (!execute("PRAGMA foreign_keys = ON;")) {
        close();
        return false;
    }

    return true;
}

void DatabaseConnection::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool DatabaseConnection::isOpen() const {
    return db != nullptr;
}

sqlite3* DatabaseConnection::getRawHandle() const {
    return db;
}

const std::string& DatabaseConnection::getPath() const {
    return dbPath;
}

const std::string& DatabaseConnection::getLastError() const {
    return lastErrorMessage;
}

bool DatabaseConnection::execute(const std::string& sql) {
    if (!db) {
        lastErrorMessage = "Database is not open.";
        return false;
    }
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        lastErrorMessage = errMsg ? errMsg : "Unknown SQL error";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseConnection::initializeSchema() {
    const char* schema = R"sql(
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT COLLATE NOCASE UNIQUE,
    email TEXT COLLATE NOCASE UNIQUE,
    password_salt TEXT,
    password_hash TEXT,
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    last_login_at TEXT,
    deleted_at TEXT,
    CHECK (
        (deleted_at IS NULL
            AND username IS NOT NULL
            AND length(username) BETWEEN 3 AND 20
            AND username NOT GLOB '*[^A-Za-z0-9_]*'
            AND password_salt IS NOT NULL
            AND length(password_salt) > 0
            AND password_hash IS NOT NULL
            AND length(password_hash) > 0)
        OR
        (deleted_at IS NOT NULL
            AND username IS NULL
            AND email IS NULL
            AND password_salt IS NULL
            AND password_hash IS NULL)
    )
);

CREATE TABLE IF NOT EXISTS sessions (
    token_hash TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    expires_at INTEGER NOT NULL,
    revoked_at TEXT
);

CREATE INDEX IF NOT EXISTS sessions_user_id_idx ON sessions(user_id);

CREATE TABLE IF NOT EXISTS login_attempts (
    username TEXT PRIMARY KEY COLLATE NOCASE,
    failed_attempts INTEGER NOT NULL DEFAULT 0 CHECK (failed_attempts >= 0),
    locked_until INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS player_profiles (
    user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE RESTRICT,
    display_name TEXT NOT NULL DEFAULT 'Player',
    coins INTEGER NOT NULL DEFAULT 100 CHECK (coins >= 0)
);

CREATE TABLE IF NOT EXISTS pets (
    user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE RESTRICT,
    name TEXT NOT NULL DEFAULT 'UIA',
    level INTEGER NOT NULL DEFAULT 1 CHECK (level > 0),
    experience INTEGER NOT NULL DEFAULT 0 CHECK (experience >= 0),
    happiness INTEGER NOT NULL DEFAULT 85 CHECK (happiness BETWEEN 0 AND 100),
    hunger INTEGER NOT NULL DEFAULT 30 CHECK (hunger BETWEEN 0 AND 100),
    energy INTEGER NOT NULL DEFAULT 70 CHECK (energy BETWEEN 0 AND 100)
);

CREATE TABLE IF NOT EXISTS inventory (
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    item_id INTEGER NOT NULL CHECK (item_id BETWEEN 1 AND 13),
    quantity INTEGER NOT NULL CHECK (quantity BETWEEN 1 AND 1000000),
    PRIMARY KEY (user_id, item_id),
    CHECK (item_id BETWEEN 1 AND 5 OR quantity = 1)
);

CREATE TABLE IF NOT EXISTS equipped_decorations (
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    slot INTEGER NOT NULL,
    item_id INTEGER NOT NULL,
    PRIMARY KEY (user_id, slot)
);
)sql";

    return execute(schema);
}
