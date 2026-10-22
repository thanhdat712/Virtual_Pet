#ifndef USER_H
#define USER_H

#include <string>

class User {
private:
    int id;
    std::string username;
    std::string email;
    std::string passwordSalt;
    std::string passwordHash;
    std::string createdAt;
    std::string lastLoginAt;

public:
    User(int id, const std::string& username, const std::string& passwordSalt,
         const std::string& passwordHash, const std::string& createdAt,
         const std::string& email = "", const std::string& lastLoginAt = "");

    static bool isValidUsername(const std::string& username);
    static bool isValidPassword(const std::string& password);
    static bool isValidEmail(const std::string& email);

    int getId() const;
    const std::string& getUsername() const;
    const std::string& getEmail() const;
    const std::string& getPasswordSalt() const;
    const std::string& getPasswordHash() const;
    const std::string& getCreatedAt() const;
    const std::string& getLastLoginAt() const;

    void updatePassword(const std::string& passwordSalt, const std::string& passwordHash);
    void setLastLoginAt(const std::string& lastLoginAt);
};

#endif
