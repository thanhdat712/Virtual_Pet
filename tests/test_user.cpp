#include <iostream>
#include <stdexcept>
#include "PlayerGameData.h"
#include "User.h"

static int passed = 0;
static int failed = 0;

static void check(const char* name, bool condition) {
    if (condition) {
        std::cout << "[PASS] " << name << "\n";
        ++passed;
    } else {
        std::cout << "[FAIL] " << name << "\n";
        ++failed;
    }
}

int main() {
    check("username minimum length", User::isValidUsername("abc"));
    check("username maximum length", User::isValidUsername("abcdefghijklmnopqrst"));
    check("username rejects short value", !User::isValidUsername("ab"));
    check("username rejects punctuation", !User::isValidUsername("bad-name"));
    check("password accepts eight characters", User::isValidPassword("12345678"));
    check("password rejects seven characters", !User::isValidPassword("1234567"));
    check("password counts UTF-8 characters", User::isValidPassword("\xC3\xA0\xC3\xA0\xC3\xA0\xC3\xA0\xC3\xA0\xC3\xA0\xC3\xA0\xC3\xA0"));
    check("password rejects malformed UTF-8", !User::isValidPassword("\xC0\xAF" "123456"));
    check("email is optional", User::isValidEmail("student@example.com"));
    check("email rejects invalid value", !User::isValidEmail("not-an-email"));

    User account(1, "student_1", "salt", "hash", "2026-10-22T00:00:00Z");
    PlayerData gameData(1, "Player", 100);
    check("account metadata is retained", account.getUsername() == "student_1" && account.getEmail().empty());
    check("last login starts unset", account.getLastLoginAt().empty());
    account.updatePassword("new-salt", "new-hash");
    account.setLastLoginAt("2026-10-22T01:00:00Z");
    check("credential updates do not alter account/game identity",
          account.getPasswordHash() == "new-hash"
              && account.getCreatedAt() == "2026-10-22T00:00:00Z"
              && account.getLastLoginAt() == "2026-10-22T01:00:00Z"
              && gameData.getUserId() == account.getId()
              && gameData.getCoins() == 100
              && gameData.getInventory().isEmpty());

    bool rejectedInvalidUsername = false;
    try {
        User invalid(2, "x!", "salt", "hash", "2026-10-22T00:00:00Z");
    } catch (const std::invalid_argument&) {
        rejectedInvalidUsername = true;
    }
    check("constructor rejects invalid username", rejectedInvalidUsername);

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
