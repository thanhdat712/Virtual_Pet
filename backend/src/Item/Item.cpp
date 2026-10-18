#include "Item.h"

Item::Item(int id, const std::string& name, const std::string& description, int price, const std::string& imagePath)
    : id(id), name(name), description(description), price(price), imagePath(imagePath) {}

Item::~Item() {}

int Item::getId() const {
    return id;
}

std::string Item::getName() const {
    return name;
}

std::string Item::getDescription() const {
    return description;
}

int Item::getPrice() const {
    return price;
}

std::string Item::getImagePath() const {
    return imagePath;
}
