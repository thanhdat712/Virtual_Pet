#include "User.h"
#include "Pet.h"
#include "Food.h"
#include <stdexcept>
#include <utility>

namespace {
bool isAsciiAlphaNumericOrUnderscore(char character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z')
        || (character >= '0' && character <= '9')
        || character == '_';
}

bool hasAtLeastEightUtf8Characters(const std::string& value) {
    std::size_t count = 0;

    for (std::size_t i = 0; i < value.size();) {
        const unsigned char first = static_cast<unsigned char>(value[i]);
        std::size_t length = 0;

        if (first <= 0x7F) length = 1;
        else if (first >= 0xC2 && first <= 0xDF) length = 2;
        else if (first >= 0xE0 && first <= 0xEF) length = 3;
        else if (first >= 0xF0 && first <= 0xF4) length = 4;
        else return false;

        if (i + length > value.size()) return false;

        for (std::size_t j = 1; j < length; ++j) {
            unsigned char continuation = static_cast<unsigned char>(value[i + j]);
            if ((continuation & 0xC0) != 0x80) return false;

            if (j == 1 &&
                ((first == 0xE0 && continuation < 0xA0) ||
                 (first == 0xED && continuation >= 0xA0) ||
                 (first == 0xF0 && continuation < 0x90) ||
                 (first == 0xF4 && continuation >= 0x90))) {
                return false;
            }
        }

        ++count;
        i += length;
    }

    return count >= 8;
}
}

User::User(int id,
           const std::string& username,
           const std::string& passwordSalt,
           const std::string& passwordHash,
           const std::string& createdAt,
           const std::string& email,
           const std::string& lastLoginAt)
    : id(id),
      username(username),
      email(email),
      passwordSalt(passwordSalt),
      passwordHash(passwordHash),
      createdAt(createdAt),
      lastLoginAt(lastLoginAt) {
    if (id <= 0) {
        throw std::invalid_argument("User id must be positive.");
    }

    if (!isValidUsername(username)) {
        throw std::invalid_argument("Invalid username.");
    }

    if (!email.empty() && !isValidEmail(email)) {
        throw std::invalid_argument("Invalid email.");
    }

    if (passwordSalt.empty() || passwordHash.empty()) {
        throw std::invalid_argument("Password salt and hash must not be empty.");
    }

    if (createdAt.empty()) {
        throw std::invalid_argument("Creation timestamp must not be empty.");
    }
}

int User::getId() const { return id; }
const std::string& User::getUsername() const { return username; }
const std::string& User::getEmail() const { return email; }
const std::string& User::getPasswordSalt() const { return passwordSalt; }
const std::string& User::getPasswordHash() const { return passwordHash; }
const std::string& User::getCreatedAt() const { return createdAt; }
const std::string& User::getLastLoginAt() const { return lastLoginAt; }

Inventory& User::getInventory() { return inventory; }
const Inventory& User::getInventory() const { return inventory; }

bool User::isValidUsername(const std::string& username) {
    if (username.size() < 3 || username.size() > 20) {
        return false;
    }

    for (char character : username) {
        if (!isAsciiAlphaNumericOrUnderscore(character)) {
            return false;
        }
    }

    return true;
}

bool User::isValidPassword(const std::string& password) {
    return password.size() <= 512 && hasAtLeastEightUtf8Characters(password);
}

bool User::isValidEmail(const std::string& email) {
    if (email.empty() || email.find_first_of(" \t\r\n") != std::string::npos) {
        return false;
    }

    std::size_t at = email.find('@');
    if (at == std::string::npos || at == 0 || at != email.rfind('@') || at + 1 >= email.size()) {
        return false;
    }

    std::size_t dot = email.find('.', at + 2);
    return dot != std::string::npos && dot + 1 < email.size();
}

void User::updatePassword(const std::string& passwordSalt, const std::string& passwordHash) {
    if (passwordSalt.empty() || passwordHash.empty()) {
        throw std::invalid_argument("Password salt and hash must not be empty.");
    }

    this->passwordSalt = passwordSalt;
    this->passwordHash = passwordHash;
}

void User::setLastLoginAt(const std::string& lastLoginAt) {
    if (lastLoginAt.empty()) {
        throw std::invalid_argument("Last login timestamp must not be empty.");
    }

    this->lastLoginAt = lastLoginAt;
}

void User::setPet(std::shared_ptr<Pet> ownedPet) {
    pet = std::move(ownedPet);
}

std::shared_ptr<Pet> User::getPet() {
    return pet;
}

std::shared_ptr<const Pet> User::getPet() const {
    return pet;
}

bool User::feedPet(int foodItemId) {
    if (!pet || inventory.getQuantity(foodItemId) <= 0) {
        return false;
    }

    const auto& slots = inventory.getSlots();
    for (const auto& slot : slots) {
        if (!slot.item || slot.item->getId() != foodItemId) {
            continue;
        }

        auto food = std::dynamic_pointer_cast<Food>(slot.item);
        if (!food) {
            return false;
        }

        ActionResult result = pet->feed(*food);
        if (!result.ok()) {
            return false;
        }

        return inventory.removeItem(foodItemId, 1);
    }

    return false;
}
