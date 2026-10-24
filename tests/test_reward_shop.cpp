#include <iostream>
#include "Reward.h"
#include "Shop.h"
#include "User.h"
#include "Food.h"

static int passed = 0;
static int failed = 0;

static void check(const std::string& title, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << title << "\n";
        passed++;
    } else {
        std::cout << "  [FAIL] " << title << "\n";
        failed++;
    }
}

int main() {
    std::cout << "=== TEST REWARD + SHOP ===\n";

    User user(7, "Ada", 100);
    Shop shop;

    auto food = std::make_shared<Food>(99, "Bánh ngọt", "Ngon và bổ dưỡng", 12, "assets/items/cake.png", 25, 10);
    Reward reward(20);
    reward.addItem(food);

    check("Reward xu ban dau = 20", reward.getCoins() == 20);
    check("Reward item duoc them", reward.getItems().size() == 1);

    reward.grantTo(user);
    check("GrantTo tang xu", user.getCoins() == 120);
    check("GrantTo them item vao inventory", user.getInventory().hasItem(99));

    auto target = shop.getItemById(1);
    check("Shop co mon co id 1", target != nullptr);
    check("Mua mon 1 thanh cong", shop.buyItem(user, 1));
    check("Sau mua xu giam", user.getCoins() == 115);
    check("Inventory da co item 1", user.getInventory().hasItem(1));

    std::cout << "Ket qua: " << passed << " dat, " << failed << " that bai\n";
    return failed == 0 ? 0 : 1;
}
