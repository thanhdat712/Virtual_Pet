#ifndef I_USER_REPOSITORY_H
#define I_USER_REPOSITORY_H

#include <string>
#include <optional>
#include <vector>
#include "../User/User.h"

// Interface định nghĩa các thao tác lưu trữ tài khoản người dùng theo Mẫu Repository
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual std::optional<User> findById(int id) = 0;
    virtual std::optional<User> findByUsername(const std::string& username) = 0;
    virtual std::optional<User> findByEmail(const std::string& email) = 0;

    virtual int createUser(const std::string& username, const std::string& passwordSalt,
                           const std::string& passwordHash, const std::string& email = "") = 0;

    virtual bool updatePassword(int id, const std::string& newSalt, const std::string& newHash) = 0;
    virtual bool updateLastLogin(int id, const std::string& timestamp) = 0;
    virtual bool softDelete(int id) = 0;

    virtual int getCoins(int userId) = 0;
    virtual bool setCoins(int userId, int coins) = 0;
    virtual bool addCoins(int userId, int amount) = 0;
};

#endif
