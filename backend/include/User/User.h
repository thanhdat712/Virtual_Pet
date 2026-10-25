#ifndef USER_H
#define USER_H

#include <string>
#include <memory>
#include "../Inventory/Inventory.h"

class Pet;

class User {
private:
    // Thông tin tài khoản
    int id;
    std::string username;
    std::string email;
    std::string passwordSalt;
    std::string passwordHash;
    std::string createdAt;
    std::string lastLoginAt;

    // Dữ liệu người chơi
    std::string name;
    int coins;
    Inventory inventory;
    std::shared_ptr<Pet> pet;

public:
    User(int id, const std::string& username, const std::string& passwordSalt,
         const std::string& passwordHash, const std::string& createdAt,
         const std::string& email = "", const std::string& lastLoginAt = "");

    // Thông tin người chơi
    int getId() const;
    std::string getName() const;
    int getCoins() const;
    void addCoins(int amount);
    bool spendCoins(int amount);

    // Kho đồ
    Inventory& getInventory();
    const Inventory& getInventory() const;

    // Thú cưng
    void setPet(std::shared_ptr<Pet> ownedPet);
    std::shared_ptr<Pet> getPet();
    std::shared_ptr<const Pet> getPet() const;

    // Chỉ trừ đồ ăn nếu pet ăn thành công
    bool feedPet(int foodItemId);

    // Kiểm tra thông tin tài khoản
    static bool isValidUsername(const std::string& username);
    static bool isValidPassword(const std::string& password);
    static bool isValidEmail(const std::string& email);

    // Lấy thông tin tài khoản
    const std::string& getUsername() const;
    const std::string& getEmail() const;
    const std::string& getPasswordSalt() const;
    const std::string& getPasswordHash() const;
    const std::string& getCreatedAt() const;
    const std::string& getLastLoginAt() const;

    // Cập nhật thông tin tài khoản
    void updatePassword(
        const std::string& passwordSalt,
        const std::string& passwordHash
    );
    void setLastLoginAt(const std::string& lastLoginAt);
};

#endif
