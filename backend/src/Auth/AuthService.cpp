#include "AuthService.h"

#include "User.h"
#include <argon2.h>
#include <sqlite3.h>

#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

namespace {
constexpr std::uint32_t ARGON2_ITERATIONS = 3;
constexpr std::uint32_t ARGON2_MEMORY_KIB = 64 * 1024;
constexpr std::uint32_t ARGON2_PARALLELISM = 1;
constexpr std::size_t SALT_SIZE = 16;
constexpr std::size_t HASH_SIZE = 32;
constexpr std::size_t SESSION_TOKEN_SIZE = 32;
constexpr std::int64_t SESSION_LIFETIME_SECONDS = 7 * 24 * 60 * 60;
constexpr int MAX_LOGIN_FAILURES = 5;
constexpr std::int64_t LOGIN_LOCK_SECONDS = 5 * 60;
constexpr unsigned char SESSION_HASH_SALT[] = "VirtualPetSessionHash";

const char* const AUTH_SCHEMA = R"sql(
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
)sql";

class Statement {
private:
    sqlite3_stmt* value = nullptr;
public:
    Statement(sqlite3* database, const char* sql) {
        if (sqlite3_prepare_v2(database, sql, -1, &value, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
    }

    ~Statement() {
        sqlite3_finalize(value);
    }

    sqlite3_stmt* get() const { return value; }
};

void wipeString(std::string& value) {
    volatile char* bytes = value.empty() ? nullptr : &value[0];
    for (std::size_t index = 0; index < value.size(); ++index) {
        bytes[index] = 0;
    }
    value.clear();
}

class PasswordWiper {
private:
    std::string& value;
public:
    explicit PasswordWiper(std::string& value) : value(value) {}
    ~PasswordWiper() { wipeString(value); }
};

bool fillRandomBytes(unsigned char* destination, std::size_t size) {
#ifdef _WIN32
    return BCryptGenRandom(
        nullptr,
        destination,
        static_cast<ULONG>(size),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
    try {
        std::random_device source;
        for (std::size_t index = 0; index < size; ++index) {
            destination[index] = static_cast<unsigned char>(source());
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
#endif
}

std::string toHex(const unsigned char* bytes, std::size_t size) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2);
    for (std::size_t index = 0; index < size; ++index) {
        result.push_back(digits[bytes[index] >> 4]);
        result.push_back(digits[bytes[index] & 0x0f]);
    }
    return result;
}

bool execute(sqlite3* database, const char* sql) {
    return sqlite3_exec(database, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

std::int64_t currentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool hashSessionToken(const std::string& token, std::string& hash) {
    if (token.size() != SESSION_TOKEN_SIZE * 2) {
        return false;
    }
    for (unsigned char character : token) {
        if (!std::isxdigit(character)) {
            return false;
        }
    }

    std::array<unsigned char, HASH_SIZE> digest{};
    if (argon2id_hash_raw(
            1,
            8,
            1,
            token.data(),
            token.size(),
            SESSION_HASH_SALT,
            sizeof(SESSION_HASH_SALT) - 1,
            digest.data(),
            digest.size()) != ARGON2_OK) {
        return false;
    }
    hash = toHex(digest.data(), digest.size());
    return true;
}
}

AuthService::AuthService(const std::string& databasePath) : database(nullptr) {
    const std::filesystem::path path(databasePath);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    if (sqlite3_open_v2(
            databasePath.c_str(),
            &database,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
            nullptr) != SQLITE_OK) {
        const std::string error = database ? sqlite3_errmsg(database) : "SQLite allocation failed.";
        if (database) {
            sqlite3_close(database);
            database = nullptr;
        }
        throw std::runtime_error("Could not open account database: " + error);
    }

    sqlite3_busy_timeout(database, 5000);
    try {
        initializeSchema();
    } catch (const std::exception&) {
        sqlite3_close(database);
        database = nullptr;
        throw;
    }
}

AuthService::~AuthService() {
    if (database) {
        sqlite3_close(database);
    }
}

void AuthService::initializeSchema() {
    char* error = nullptr;
    if (sqlite3_exec(database, AUTH_SCHEMA, nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error ? error : "Unknown SQLite error.";
        sqlite3_free(error);
        throw std::runtime_error("Could not initialize account schema: " + message);
    }
}

RegisterResult AuthService::registerUser(std::string username, std::string password) {
    return registerUser(std::move(username), {}, std::move(password));
}

RegisterResult AuthService::registerUser(std::string username, std::string email, std::string password) {
    PasswordWiper passwordWiper(password);
    if (!User::isValidUsername(username)) {
        return {RegisterStatus::INVALID_USERNAME, 0};
    }
    if (email.size() > 254) {
        return {RegisterStatus::INVALID_EMAIL, 0};
    }
    if (!User::isValidPassword(password)) {
        return {RegisterStatus::INVALID_PASSWORD, 0};
    }

    {
        std::lock_guard<std::mutex> lock(databaseMutex);
        try {
            Statement existingUser(database,
                "SELECT 1 FROM users WHERE username = ? COLLATE NOCASE AND deleted_at IS NULL LIMIT 1;");
            sqlite3_bind_text(existingUser.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
            const int lookupStatus = sqlite3_step(existingUser.get());
            if (lookupStatus == SQLITE_ROW) {
                return {RegisterStatus::USERNAME_TAKEN, 0};
            }
            if (lookupStatus != SQLITE_DONE) {
                return {RegisterStatus::DATABASE_ERROR, 0, sqlite3_errmsg(database)};
            }
            if (!email.empty()) {
                Statement existingEmail(database,
                    "SELECT 1 FROM users WHERE email = ? COLLATE NOCASE AND deleted_at IS NULL LIMIT 1;");
                sqlite3_bind_text(existingEmail.get(), 1, email.c_str(), -1, SQLITE_TRANSIENT);
                const int emailStatus = sqlite3_step(existingEmail.get());
                if (emailStatus == SQLITE_ROW) return {RegisterStatus::EMAIL_TAKEN, 0};
                if (emailStatus != SQLITE_DONE) return {RegisterStatus::DATABASE_ERROR, 0, sqlite3_errmsg(database)};
            }
        } catch (const std::exception& error) {
            return {RegisterStatus::DATABASE_ERROR, 0, error.what()};
        }
    }

    std::array<unsigned char, SALT_SIZE> salt{};
    if (!fillRandomBytes(salt.data(), salt.size())) {
        return {RegisterStatus::HASH_ERROR, 0, "Secure random salt generation failed."};
    }

    const std::size_t encodedSize = argon2_encodedlen(
        ARGON2_ITERATIONS,
        ARGON2_MEMORY_KIB,
        ARGON2_PARALLELISM,
        static_cast<std::uint32_t>(salt.size()),
        static_cast<std::uint32_t>(HASH_SIZE),
        Argon2_id);
    std::string encodedHash(encodedSize, '\0');
    const int hashStatus = argon2id_hash_encoded(
        ARGON2_ITERATIONS,
        ARGON2_MEMORY_KIB,
        ARGON2_PARALLELISM,
        password.data(),
        password.size(),
        salt.data(),
        salt.size(),
        HASH_SIZE,
        encodedHash.data(),
        encodedHash.size());
    if (hashStatus != ARGON2_OK) {
        wipeString(encodedHash);
        return {RegisterStatus::HASH_ERROR, 0, argon2_error_message(hashStatus)};
    }
    encodedHash.resize(std::char_traits<char>::length(encodedHash.c_str()));
    const std::string saltHex = toHex(salt.data(), salt.size());

    std::lock_guard<std::mutex> lock(databaseMutex);
    if (!execute(database, "BEGIN IMMEDIATE;")) {
        wipeString(encodedHash);
        return {RegisterStatus::DATABASE_ERROR, 0, sqlite3_errmsg(database)};
    }

    try {
        Statement insertUser(database,
            "INSERT INTO users (username, email, password_salt, password_hash) VALUES (?, ?, ?, ?);");
        sqlite3_bind_text(insertUser.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (email.empty()) sqlite3_bind_null(insertUser.get(), 2);
        else sqlite3_bind_text(insertUser.get(), 2, email.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertUser.get(), 3, saltHex.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertUser.get(), 4, encodedHash.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insertUser.get()) != SQLITE_DONE) {
            const int error = sqlite3_extended_errcode(database);
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            if (error == SQLITE_CONSTRAINT_UNIQUE || error == SQLITE_CONSTRAINT_PRIMARYKEY) {
                return {RegisterStatus::USERNAME_TAKEN, 0};
            }
            return {RegisterStatus::DATABASE_ERROR, 0, detail};
        }

        const sqlite3_int64 userId = sqlite3_last_insert_rowid(database);
        Statement insertProfile(database,
            "INSERT INTO player_profiles (user_id, display_name, coins) VALUES (?, ?, 100);");
        sqlite3_bind_int64(insertProfile.get(), 1, userId);
        sqlite3_bind_text(insertProfile.get(), 2, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insertProfile.get()) != SQLITE_DONE) {
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            return {RegisterStatus::DATABASE_ERROR, 0, detail};
        }

        Statement insertPet(database, "INSERT INTO pets (user_id, name) VALUES (?, ?);");
        sqlite3_bind_int64(insertPet.get(), 1, userId);
        sqlite3_bind_text(insertPet.get(), 2, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insertPet.get()) != SQLITE_DONE) {
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            return {RegisterStatus::DATABASE_ERROR, 0, detail};
        }

        Statement clearAttempts(database, "DELETE FROM login_attempts WHERE username = ? COLLATE NOCASE;");
        sqlite3_bind_text(clearAttempts.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(clearAttempts.get()) != SQLITE_DONE) {
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            return {RegisterStatus::DATABASE_ERROR, 0, detail};
        }

        if (!execute(database, "COMMIT;")) {
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            return {RegisterStatus::DATABASE_ERROR, 0, detail};
        }

        wipeString(encodedHash);
        return {RegisterStatus::SUCCESS, static_cast<int>(userId)};
    } catch (const std::exception& error) {
        execute(database, "ROLLBACK;");
        wipeString(encodedHash);
        return {RegisterStatus::DATABASE_ERROR, 0, error.what()};
    }
}

LoginResult AuthService::login(std::string username, std::string password) {
    return loginWithIdentity(std::move(username), std::move(password), false);
}

LoginResult AuthService::loginByEmail(std::string email, std::string password) {
    return loginWithIdentity(std::move(email), std::move(password), true);
}

LoginResult AuthService::loginWithIdentity(std::string username, std::string password, bool useEmail) {
    PasswordWiper passwordWiper(password);
    if (username.empty() || username.size() > (useEmail ? 254u : 256u) || password.size() > 512) {
        return {LoginStatus::INVALID_CREDENTIALS};
    }

    std::lock_guard<std::mutex> lock(databaseMutex);
    const std::int64_t now = currentTimestamp();
    int failures = 0;
    std::int64_t lockedUntil = 0;

    try {
        {
            Statement attempt(database,
                "SELECT failed_attempts, locked_until FROM login_attempts WHERE username = ? COLLATE NOCASE;");
            sqlite3_bind_text(attempt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
            const int attemptStatus = sqlite3_step(attempt.get());
            if (attemptStatus == SQLITE_ROW) {
                failures = sqlite3_column_int(attempt.get(), 0);
                lockedUntil = sqlite3_column_int64(attempt.get(), 1);
            } else if (attemptStatus != SQLITE_DONE) {
                return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, sqlite3_errmsg(database)};
            }
        }
        const bool isLocked = lockedUntil > now;

        bool userExists = false;
        int userId = 0;
        std::string canonicalUsername;
        std::string storedHash;
        {
            const std::string userQuery = std::string("SELECT id, username, password_hash FROM users WHERE ")
                + (useEmail ? "email" : "username")
                + " = ? COLLATE NOCASE AND deleted_at IS NULL LIMIT 1;";
            Statement user(database, userQuery.c_str());
            sqlite3_bind_text(user.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
            const int userStatus = sqlite3_step(user.get());
            if (userStatus != SQLITE_ROW && userStatus != SQLITE_DONE) {
                return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, sqlite3_errmsg(database)};
            }
            userExists = userStatus == SQLITE_ROW;
            if (userExists) {
                userId = sqlite3_column_int(user.get(), 0);
                const auto* usernameText = reinterpret_cast<const char*>(sqlite3_column_text(user.get(), 1));
                const auto* hashText = reinterpret_cast<const char*>(sqlite3_column_text(user.get(), 2));
                canonicalUsername = usernameText ? usernameText : "";
                storedHash = hashText ? hashText : "";
            }
        }

        bool passwordMatches = false;
        if (userExists && !storedHash.empty()) {
            passwordMatches = argon2id_verify(storedHash.c_str(), password.data(), password.size()) == ARGON2_OK;
        } else {
            std::array<unsigned char, SALT_SIZE> dummySalt{};
            const std::size_t encodedSize = argon2_encodedlen(
                ARGON2_ITERATIONS,
                ARGON2_MEMORY_KIB,
                ARGON2_PARALLELISM,
                static_cast<std::uint32_t>(dummySalt.size()),
                static_cast<std::uint32_t>(HASH_SIZE),
                Argon2_id);
            std::string dummyHash(encodedSize, '\0');
            if (!fillRandomBytes(dummySalt.data(), dummySalt.size())) {
                return {LoginStatus::HASH_ERROR, 0, {}, {}, 0, "Secure random salt generation failed."};
            }
            const int hashStatus = argon2id_hash_encoded(
                ARGON2_ITERATIONS,
                ARGON2_MEMORY_KIB,
                ARGON2_PARALLELISM,
                password.data(),
                password.size(),
                dummySalt.data(),
                dummySalt.size(),
                HASH_SIZE,
                dummyHash.data(),
                dummyHash.size());
            if (hashStatus != ARGON2_OK) {
                wipeString(dummyHash);
                return {LoginStatus::HASH_ERROR, 0, {}, {}, 0, argon2_error_message(hashStatus)};
            }
            wipeString(dummyHash);
        }

        if (isLocked) {
            return {LoginStatus::RATE_LIMITED};
        }

        if (!userExists || !passwordMatches) {
            if (!useEmail && !User::isValidUsername(username)) {
                return {LoginStatus::INVALID_CREDENTIALS};
            }
            if (lockedUntil > 0) {
                failures = 0;
            }
            ++failures;
            const std::int64_t newLockedUntil =
                failures >= MAX_LOGIN_FAILURES ? now + LOGIN_LOCK_SECONDS : 0;
            Statement saveAttempt(database,
                "INSERT INTO login_attempts (username, failed_attempts, locked_until) VALUES (?, ?, ?) "
                "ON CONFLICT(username) DO UPDATE SET "
                "failed_attempts = excluded.failed_attempts, locked_until = excluded.locked_until;");
            sqlite3_bind_text(saveAttempt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(saveAttempt.get(), 2, failures);
            sqlite3_bind_int64(saveAttempt.get(), 3, newLockedUntil);
            if (sqlite3_step(saveAttempt.get()) != SQLITE_DONE) {
                return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, sqlite3_errmsg(database)};
            }
            return {failures >= MAX_LOGIN_FAILURES
                ? LoginStatus::RATE_LIMITED
                : LoginStatus::INVALID_CREDENTIALS};
        }

        std::array<unsigned char, SESSION_TOKEN_SIZE> tokenBytes{};
        if (!fillRandomBytes(tokenBytes.data(), tokenBytes.size())) {
            return {LoginStatus::HASH_ERROR, 0, {}, {}, 0, "Secure random session token generation failed."};
        }
        const std::string token = toHex(tokenBytes.data(), tokenBytes.size());
        std::string tokenHash;
        if (!hashSessionToken(token, tokenHash)) {
            return {LoginStatus::HASH_ERROR, 0, {}, {}, 0, "Session token hashing failed."};
        }

        const std::int64_t expiresAt = now + SESSION_LIFETIME_SECONDS;
        if (!execute(database, "BEGIN IMMEDIATE;")) {
            return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, sqlite3_errmsg(database)};
        }

        Statement insertSession(database,
            "INSERT INTO sessions (token_hash, user_id, expires_at) VALUES (?, ?, ?);");
        sqlite3_bind_text(insertSession.get(), 1, tokenHash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insertSession.get(), 2, userId);
        sqlite3_bind_int64(insertSession.get(), 3, expiresAt);
        if (sqlite3_step(insertSession.get()) != SQLITE_DONE) {
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, detail};
        }

        Statement nameDefaultPet(database,
            "UPDATE pets SET name = ? WHERE user_id = ? AND name = 'UIA';");
        sqlite3_bind_text(nameDefaultPet.get(), 1, canonicalUsername.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(nameDefaultPet.get(), 2, userId);
        if (sqlite3_step(nameDefaultPet.get()) != SQLITE_DONE) {
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, detail};
        }

        Statement updateLogin(database,
            "UPDATE users SET last_login_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now') WHERE id = ?;");
        sqlite3_bind_int(updateLogin.get(), 1, userId);
        if (sqlite3_step(updateLogin.get()) != SQLITE_DONE) {
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, detail};
        }

        Statement clearAttempts(database, "DELETE FROM login_attempts WHERE username = ? COLLATE NOCASE;");
        sqlite3_bind_text(clearAttempts.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(clearAttempts.get()) != SQLITE_DONE || !execute(database, "COMMIT;")) {
            const std::string detail = sqlite3_errmsg(database);
            execute(database, "ROLLBACK;");
            return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, detail};
        }

        return {LoginStatus::SUCCESS, userId, canonicalUsername, token, expiresAt};
    } catch (const std::exception& error) {
        execute(database, "ROLLBACK;");
        return {LoginStatus::DATABASE_ERROR, 0, {}, {}, 0, error.what()};
    }
}

SessionResult AuthService::currentSession(const std::string& sessionToken) {
    std::string tokenHash;
    if (!hashSessionToken(sessionToken, tokenHash)) {
        return {};
    }

    std::lock_guard<std::mutex> lock(databaseMutex);
    try {
        Statement session(database,
            "SELECT u.id, u.username FROM sessions s "
            "JOIN users u ON u.id = s.user_id "
            "WHERE s.token_hash = ? AND s.revoked_at IS NULL AND s.expires_at > ? "
            "AND u.deleted_at IS NULL LIMIT 1;");
        sqlite3_bind_text(session.get(), 1, tokenHash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(session.get(), 2, currentTimestamp());
        const int status = sqlite3_step(session.get());
        if (status == SQLITE_DONE) {
            return {};
        }
        if (status != SQLITE_ROW) {
            return {false, 0, {}, sqlite3_errmsg(database)};
        }
        const auto* username = reinterpret_cast<const char*>(sqlite3_column_text(session.get(), 1));
        return {
            true,
            sqlite3_column_int(session.get(), 0),
            username ? username : ""
        };
    } catch (const std::exception& error) {
        return {false, 0, {}, error.what()};
    }
}

LogoutStatus AuthService::logout(const std::string& sessionToken, std::string* detail) {
    if (sessionToken.empty()) {
        return LogoutStatus::SUCCESS;
    }

    std::string tokenHash;
    if (!hashSessionToken(sessionToken, tokenHash)) {
        return LogoutStatus::SUCCESS;
    }

    std::lock_guard<std::mutex> lock(databaseMutex);
    try {
        Statement revokeSession(database,
            "UPDATE sessions SET revoked_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now') "
            "WHERE token_hash = ? AND revoked_at IS NULL;");
        sqlite3_bind_text(revokeSession.get(), 1, tokenHash.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(revokeSession.get()) != SQLITE_DONE) {
            if (detail) {
                *detail = sqlite3_errmsg(database);
            }
            return LogoutStatus::DATABASE_ERROR;
        }
        return LogoutStatus::SUCCESS;
    } catch (const std::exception& error) {
        if (detail) {
            *detail = error.what();
        }
        return LogoutStatus::DATABASE_ERROR;
    }
}
