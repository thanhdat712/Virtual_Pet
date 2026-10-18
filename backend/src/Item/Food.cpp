#include "Food.h"

Food::Food(int id, const std::string& name, const std::string& description, int price,
           const std::string& imagePath, int hungerRestore, int happinessRestore)
    : Item(id, name, description, price, imagePath),
      hungerRestore(hungerRestore), happinessRestore(happinessRestore) {}

int Food::getHungerRestore() const {
    return hungerRestore;
}

int Food::getHappinessRestore() const {
    return happinessRestore;
}

ItemType Food::getType() const {
    return ItemType::FOOD;
}
