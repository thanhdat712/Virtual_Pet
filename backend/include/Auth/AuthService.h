#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <string>
#include <mutex>
#include <cstdint>

struct sqlite3;

enum class RegisterStatus {
    SUCCESS,
    USERNAME_TAKEN,
    INVALID_USERNAME,
    INVALID_PASSWORD,
    DATABASE_ERROR,
    HASH_ERROR
};

struct RegisterResult {
    RegisterStatus status;
    int userId = 0;
    std::string detail;
};

enum class LoginStatus {
    SUCCESS,
    INVALID_CREDENTIALS,
    RATE_LIMITED,
    DATABASE_ERROR,
    HASH_ERROR
};

struct LoginResult {
    LoginStatus status;
    int userId = 0;
    std::string username;
    std::string sessionToken;
    std::int64_t expiresAt = 0;
    std::string detail;
};

struct SessionResult {
    bool authenticated = false;
    int userId = 0;
    std::string username;
    std::string detail;
};

enum class LogoutStatus {
    SUCCESS,
    DATABASE_ERROR
};

class AuthService {
private:
    sqlite3* database;
    std::mutex databaseMutex;

    void initializeSchema();

public:
    explicit AuthService(const std::string& databasePath);
    ~AuthService();

    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;

    RegisterResult registerUser(std::string username, std::string password);
    LoginResult login(std::string username, std::string password);
    SessionResult currentSession(const std::string& sessionToken);
    LogoutStatus logout(const std::string& sessionToken, std::string* detail = nullptr);
};

#endif
