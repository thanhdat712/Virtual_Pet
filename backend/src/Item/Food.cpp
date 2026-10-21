#include "Food.h"
#include <algorithm> // dùng std::clamp để kẹp giá trị trong khoảng 0-100

Food::Food(int id, const std::string& name, const std::string& description, int price,
           const std::string& imagePath, int hungerRestore, int happinessRestore)
    : Item(id, name, description, price, imagePath),
      // mức hồi phục là thang %, nên kẹp lại trong 0-100 để không bao giờ vượt ngưỡng
      hungerRestore(std::clamp(hungerRestore, 0, 100)),
      happinessRestore(std::clamp(happinessRestore, 0, 100)) {}

int Food::getHungerRestore() const {
    return hungerRestore;
}

int Food::getHappinessRestore() const {
    return happinessRestore;
}

ItemType Food::getType() const {
    return ItemType::FOOD;
}
