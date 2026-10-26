#include "PlayerGameData.h"
#include "Pet.h"
#include "Food.h"
#include <utility>

PlayerData::PlayerData(int userId, const std::string& name, int coins)
    : userId(userId), name(name), coins(coins) {}

int PlayerData::getId() const { return userId; }
int PlayerData::getUserId() const { return userId; }
const std::string& PlayerData::getName() const { return name; }
int PlayerData::getCoins() const { return coins; }

void PlayerData::addCoins(int amount) {
    if (amount > 0) {
        coins += amount;
    }
}

bool PlayerData::spendCoins(int amount) {
    if (amount < 0 || coins < amount) {
        return false;
    }
    coins -= amount;
    return true;
}

Inventory& PlayerData::getInventory() { return inventory; }
const Inventory& PlayerData::getInventory() const { return inventory; }

void PlayerData::setPet(std::shared_ptr<Pet> ownedPet) {
    pet = std::move(ownedPet);
}

std::shared_ptr<Pet> PlayerData::getPet() {
    return pet;
}

std::shared_ptr<const Pet> PlayerData::getPet() const {
    return pet;
}

bool PlayerData::feedPet(int foodItemId) {
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
