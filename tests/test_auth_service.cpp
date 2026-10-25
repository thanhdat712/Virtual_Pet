#include "AuthService.h"

#include <sqlite3.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
bool check(const char* name, bool condition) {
    std::cout << (condition ? "[PASS] " : "[FAIL] ") << name << '\n';
    return condition;
}
}

int main() {
    const auto uniqueId = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path databasePath =
        std::filesystem::temp_directory_path() / ("virtual-pet-auth-" + std::to_string(uniqueId) + ".db");
    bool passed = true;

    try {
        {
            AuthService service(databasePath.string());
            const RegisterResult registered = service.registerUser("Mimi_01", "a secure pet password");
            passed &= check("valid registration succeeds", registered.status == RegisterStatus::SUCCESS);
            passed &= check("registration returns a user id", registered.userId > 0);

            const RegisterResult duplicate = service.registerUser("mImI_01", "another secure password");
            passed &= check("duplicate usernames ignore case", duplicate.status == RegisterStatus::USERNAME_TAKEN);
            passed &= check("invalid username is rejected", service.registerUser("bad-name", "a secure pet password").status == RegisterStatus::INVALID_USERNAME);
            passed &= check("short password is rejected", service.registerUser("valid_name", "short").status == RegisterStatus::INVALID_PASSWORD);

            const LoginResult login = service.login("mImI_01", "a secure pet password");
            passed &= check("login accepts username without case sensitivity", login.status == LoginStatus::SUCCESS);
            passed &= check("login issues a random session token", login.sessionToken.size() == 64);
            const SessionResult activeSession = service.currentSession(login.sessionToken);
            passed &= check("session token identifies the signed-in user",
                activeSession.authenticated && activeSession.userId == registered.userId
                    && activeSession.username == "Mimi_01");
            passed &= check("logout revokes the current session",
                service.logout(login.sessionToken) == LogoutStatus::SUCCESS
                    && !service.currentSession(login.sessionToken).authenticated);
            passed &= check("unknown and incorrect credentials share a generic result",
                service.login("missing_user", "a secure pet password").status == LoginStatus::INVALID_CREDENTIALS
                    && service.login("Mimi_01", "wrong secure password").status == LoginStatus::INVALID_CREDENTIALS);
            for (int attempt = 0; attempt < 3; ++attempt) {
                passed &= check("failed login is rejected",
                    service.login("Mimi_01", "wrong secure password").status == LoginStatus::INVALID_CREDENTIALS);
            }
            passed &= check("fifth failed login triggers temporary lockout",
                service.login("Mimi_01", "wrong secure password").status == LoginStatus::RATE_LIMITED);
            passed &= check("lockout blocks even correct credentials until expiry",
                service.login("Mimi_01", "a secure pet password").status == LoginStatus::RATE_LIMITED);

            sqlite3* database = nullptr;
            if (sqlite3_open(databasePath.string().c_str(), &database) != SQLITE_OK) {
                passed &= check("test can open database", false);
            } else {
                sqlite3_stmt* statement = nullptr;
                const char* query =
                    "SELECT u.password_salt, u.password_hash, p.coins, pet.name, "
                    "(SELECT count(*) FROM inventory i WHERE i.user_id = u.id) "
                    "FROM users u "
                    "JOIN player_profiles p ON p.user_id = u.id "
                    "JOIN pets pet ON pet.user_id = u.id "
                    "WHERE u.id = ?;";
                const bool prepared = sqlite3_prepare_v2(database, query, -1, &statement, nullptr) == SQLITE_OK;
                if (prepared) {
                    sqlite3_bind_int(statement, 1, registered.userId);
                }
                const bool found = prepared && sqlite3_step(statement) == SQLITE_ROW;
                passed &= check("new user gets all game data in one registration", found);
                if (found) {
                    const auto* salt = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
                    const auto* hash = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
                    const auto* petName = reinterpret_cast<const char*>(sqlite3_column_text(statement, 3));
                    passed &= check("password is stored as salted Argon2id hash",
                        salt && std::string(salt).size() == 32
                            && hash && std::string(hash).find("$argon2id$") == 0
                            && std::string(hash).find("a secure pet password") == std::string::npos);
                    passed &= check("new player receives 100 coins", sqlite3_column_int(statement, 2) == 100);
                    passed &= check("new player gets the default pet", petName && std::string(petName) == "UIA");
                    passed &= check("new player's inventory starts empty", sqlite3_column_int(statement, 4) == 0);
                }
                sqlite3_finalize(statement);
                statement = nullptr;

                const bool sessionHashPrepared = sqlite3_prepare_v2(
                    database,
                    "SELECT token_hash FROM sessions WHERE user_id = ? LIMIT 1;",
                    -1,
                    &statement,
                    nullptr) == SQLITE_OK;
                if (sessionHashPrepared) {
                    sqlite3_bind_int(statement, 1, registered.userId);
                }
                const bool sessionHashFound = sessionHashPrepared && sqlite3_step(statement) == SQLITE_ROW;
                const auto* storedTokenHash = sessionHashFound
                    ? reinterpret_cast<const char*>(sqlite3_column_text(statement, 0))
                    : nullptr;
                passed &= check("database stores only a session token hash",
                    storedTokenHash && std::string(storedTokenHash).size() == 64
                        && std::string(storedTokenHash) != login.sessionToken);
                sqlite3_finalize(statement);
                statement = nullptr;

                const bool lockoutPrepared = sqlite3_prepare_v2(
                    database,
                    "SELECT locked_until FROM login_attempts WHERE username = 'MIMI_01';",
                    -1,
                    &statement,
                    nullptr) == SQLITE_OK;
                const bool lockoutFound = lockoutPrepared && sqlite3_step(statement) == SQLITE_ROW;
                const auto lockExpiresAt = lockoutFound
                    ? sqlite3_column_int64(statement, 0)
                    : 0;
                const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                passed &= check("failed login lock lasts five minutes",
                    lockExpiresAt >= now + 295 && lockExpiresAt <= now + 305);
                sqlite3_finalize(statement);
                sqlite3_close(database);
            }

            sqlite3* triggerDatabase = nullptr;
            if (sqlite3_open(databasePath.string().c_str(), &triggerDatabase) == SQLITE_OK) {
                const int triggerStatus = sqlite3_exec(
                    triggerDatabase,
                    "CREATE TRIGGER reject_pet BEFORE INSERT ON pets BEGIN SELECT RAISE(FAIL, 'test failure'); END;",
                    nullptr,
                    nullptr,
                    nullptr);
                sqlite3_close(triggerDatabase);
                if (triggerStatus == SQLITE_OK) {
                    const RegisterResult failedPet = service.registerUser("rollback_user", "another secure password");
                    passed &= check("initial game data creation reports failure",
                        failedPet.status == RegisterStatus::DATABASE_ERROR);

                    sqlite3* verificationDatabase = nullptr;
                    if (sqlite3_open(databasePath.string().c_str(), &verificationDatabase) == SQLITE_OK) {
                        sqlite3_stmt* countStatement = nullptr;
                        const bool countPrepared = sqlite3_prepare_v2(
                            verificationDatabase,
                            "SELECT count(*) FROM users WHERE username = 'rollback_user';",
                            -1,
                            &countStatement,
                            nullptr) == SQLITE_OK;
                        const bool rolledBack = countPrepared
                            && sqlite3_step(countStatement) == SQLITE_ROW
                            && sqlite3_column_int(countStatement, 0) == 0;
                        passed &= check("failed registration rolls back account and profile", rolledBack);
                        sqlite3_finalize(countStatement);
                        sqlite3_close(verificationDatabase);
                    } else {
                        passed &= check("failed registration rolls back account and profile", false);
                    }
                } else {
                    passed &= check("test can inject initialization failure", false);
                }
            } else {
                passed &= check("test can open registration database", false);
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Auth service test raised an exception: " << error.what() << '\n';
        passed = false;
    }

    std::error_code cleanupError;
    std::filesystem::remove(databasePath, cleanupError);
    if (cleanupError) {
        std::cerr << "Could not remove test database: " << cleanupError.message() << '\n';
        passed = false;
    }
    std::filesystem::remove(databasePath.string() + "-journal", cleanupError);
    std::filesystem::remove(databasePath.string() + "-wal", cleanupError);
    std::filesystem::remove(databasePath.string() + "-shm", cleanupError);

    return passed ? 0 : 1;
}
