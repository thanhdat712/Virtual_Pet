#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include "IUserRepository.h"
#include "DatabaseConnection.h"

// Triển khai IUserRepository sử dụng SQLite và Prepared Statements
class UserRepository : public IUserRepository {
private:
    DatabaseConnection& db;

public:
    explicit UserRepository(DatabaseConnection& dbConnection);

    std::optional<User> findById(int id) override;
    std::optional<User> findByUsername(const std::string& username) override;
    std::optional<User> findByEmail(const std::string& email) override;

    int createUser(const std::string& username, const std::string& passwordSalt,
                   const std::string& passwordHash, const std::string& email = "") override;

    bool updatePassword(int id, const std::string& newSalt, const std::string& newHash) override;
    bool updateLastLogin(int id, const std::string& timestamp) override;
    bool softDelete(int id) override;

    int getCoins(int userId) override;
    bool setCoins(int userId, int coins) override;
    bool addCoins(int userId, int amount) override;
};

#endif
