#include "User.h"

#include <stdexcept>

namespace {
bool isAsciiAlphaNumericOrUnderscore(char character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z')
        || (character >= '0' && character <= '9')
        || character == '_';
}

bool hasAtLeastEightUtf8Characters(const std::string& value) {
    std::size_t characterCount = 0;

    for (std::size_t index = 0; index < value.size();) {
        const unsigned char first = static_cast<unsigned char>(value[index]);
        std::size_t sequenceLength = 0;

        if (first <= 0x7F) {
            sequenceLength = 1;
        } else if (first >= 0xC2 && first <= 0xDF) {
            sequenceLength = 2;
        } else if (first >= 0xE0 && first <= 0xEF) {
            sequenceLength = 3;
        } else if (first >= 0xF0 && first <= 0xF4) {
            sequenceLength = 4;
        } else {
            return false;
        }

        if (index + sequenceLength > value.size()) {
            return false;
        }

        for (std::size_t offset = 1; offset < sequenceLength; ++offset) {
            const unsigned char continuation = static_cast<unsigned char>(value[index + offset]);
            if ((continuation & 0xC0) != 0x80) {
                return false;
            }
            if (offset == 1
                && ((first == 0xE0 && continuation < 0xA0)
                    || (first == 0xED && continuation >= 0xA0)
                    || (first == 0xF0 && continuation < 0x90)
                    || (first == 0xF4 && continuation >= 0x90))) {
                return false;
            }
        }

        ++characterCount;
        index += sequenceLength;
    }

    return characterCount >= 8;
}
}

User::User(int id, const std::string& username, const std::string& passwordSalt,
           const std::string& passwordHash, const std::string& createdAt,
           const std::string& email, const std::string& lastLoginAt)
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
        throw std::invalid_argument("Username must contain 3-20 ASCII letters, digits, or underscores.");
    }
    if (!email.empty() && !isValidEmail(email)) {
        throw std::invalid_argument("Email address is invalid.");
    }
    if (passwordSalt.empty() || passwordHash.empty()) {
        throw std::invalid_argument("Password salt and hash must not be empty.");
    }
    if (createdAt.empty()) {
        throw std::invalid_argument("Account creation timestamp must not be empty.");
    }
}

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
    const std::size_t at = email.find('@');
    if (at == std::string::npos || at == 0 || at != email.rfind('@') || at + 1 >= email.size()) {
        return false;
    }
    const std::size_t dot = email.find('.', at + 2);
    return dot != std::string::npos && dot + 1 < email.size();
}

int User::getId() const { return id; }
const std::string& User::getUsername() const { return username; }
const std::string& User::getEmail() const { return email; }
const std::string& User::getPasswordSalt() const { return passwordSalt; }
const std::string& User::getPasswordHash() const { return passwordHash; }
const std::string& User::getCreatedAt() const { return createdAt; }
const std::string& User::getLastLoginAt() const { return lastLoginAt; }

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
