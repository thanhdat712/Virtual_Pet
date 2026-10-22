#include "User.h"
#include "../Pet/Pet.h"
#include "../Item/Food.h"
#include <memory>

User::User(int id, const std::string& name, int coins)
    : id(id), name(name), coins(coins < 0 ? 0 : coins) {}

int User::getId() const { return id; }
std::string User::getName() const { return name; }
int User::getCoins() const { return coins; }
void User::addCoins(int amount) { if (amount > 0) coins += amount; }
bool User::spendCoins(int amount) {
    if (amount < 0 || coins < amount) return false;
    coins -= amount;
    return true;
}
Inventory& User::getInventory() { return inventory; }
const Inventory& User::getInventory() const { return inventory; }

void User::setPet(std::shared_ptr<Pet> ownedPet) { pet = std::move(ownedPet); }
std::shared_ptr<Pet> User::getPet() { return pet; }
std::shared_ptr<const Pet> User::getPet() const { return pet; }

bool User::feedPet(int foodItemId) {
    if (!pet || inventory.getQuantity(foodItemId) <= 0) return false;
    const auto& slots = inventory.getSlots();
    for (const auto& slot : slots) {
        if (slot.item && slot.item->getId() == foodItemId) {
            auto food = std::dynamic_pointer_cast<Food>(slot.item);
            if (!food) return false;
            // Thuc hien an truoc, chi tru kho khi an thanh cong.
            if (!pet->eat(*food)) return false;
            return inventory.removeItem(foodItemId, 1);
        }
    }
    return false;
}
